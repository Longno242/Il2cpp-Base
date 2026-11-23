#include "core/crash.hpp"

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <exception>

#include "config.hpp"
#include "core/logger.hpp"

#pragma comment(lib, "dbghelp.lib")

#ifndef STATUS_HEAP_CORRUPTION
#define STATUS_HEAP_CORRUPTION 0xC0000374L
#endif
#ifndef STATUS_STACK_BUFFER_OVERRUN
#define STATUS_STACK_BUFFER_OVERRUN 0xC0000409L
#endif

static constexpr DWORD kMsvcCppException = 0xE06D7363u;

namespace il2cpp_base::crash {
namespace {

std::atomic<bool> g_installed{ false };
std::atomic<bool> g_shutting_down{ false };
std::atomic<bool> g_reporting{ false };
LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = nullptr;
PVOID g_vectored = nullptr;

const char* exception_name(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
    case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
    case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_INVALID_DISPOSITION:      return "INVALID_DISPOSITION";
    case EXCEPTION_GUARD_PAGE:               return "GUARD_PAGE";
    case STATUS_HEAP_CORRUPTION:             return "HEAP_CORRUPTION";
    case STATUS_STACK_BUFFER_OVERRUN:        return "STACK_BUFFER_OVERRUN";
    case kMsvcCppException:                 return "MSVC_CXX_EXCEPTION";
    default:                                 return "EXCEPTION";
    }
}

bool should_ignore(EXCEPTION_POINTERS* info) {
    if (g_shutting_down.load() || !info || !info->ExceptionRecord) {
        return true;
    }
    if (info->ExceptionRecord->ExceptionCode == kMsvcCppException) {
        return true;
    }
    return false;
}

void print_line(const char* fmt, ...) {
    char buf[1024]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fputs(buf, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    fputs(buf, stderr);
    fputc('\n', stderr);
    fflush(stderr);
    LOG_FATAL("Crash", "%s", buf);
}

void print_module(void* address) {
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCSTR>(address), &module) || !module) {
        return;
    }

    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(module, path, MAX_PATH)) {
        return;
    }

    const char* name = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            name = p + 1;
        }
    }

    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto addr = reinterpret_cast<std::uintptr_t>(address);
    print_line("  Module: %s + 0x%llX", name, static_cast<unsigned long long>(addr - base));
}

void print_stack(CONTEXT* context_in) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(process, nullptr, TRUE)) {
        print_line("  (no symbols)");
        return;
    }

    CONTEXT context = *context_in;
    STACKFRAME64 frame{};
    DWORD machine = IMAGE_FILE_MACHINE_AMD64;
#if defined(_M_X64) || defined(__x86_64__)
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif

    print_line("Call stack:");
    for (int i = 0; i < 32; ++i) {
        if (!StackWalk64(machine, process, thread, &frame, &context,
                nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
            break;
        }
        if (frame.AddrPC.Offset == 0) {
            break;
        }

        char symbol_buf[sizeof(SYMBOL_INFO) + 256]{};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buf);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
            IMAGEHLP_LINE64 line{};
            line.SizeOfStruct = sizeof(line);
            DWORD line_disp = 0;
            if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_disp, &line)) {
                print_line("  #%02d %s + 0x%llX  (%s:%lu)",
                    i, symbol->Name,
                    static_cast<unsigned long long>(displacement),
                    line.FileName, line.LineNumber);
            } else {
                print_line("  #%02d %s + 0x%llX  [0x%p]",
                    i, symbol->Name,
                    static_cast<unsigned long long>(displacement),
                    reinterpret_cast<void*>(frame.AddrPC.Offset));
            }
        } else {
            print_line("  #%02d 0x%p", i, reinterpret_cast<void*>(frame.AddrPC.Offset));
            print_module(reinterpret_cast<void*>(frame.AddrPC.Offset));
        }
    }

    SymCleanup(process);
}

void report(EXCEPTION_POINTERS* info) {
    bool expected = false;
    if (!g_reporting.compare_exchange_strong(expected, true)) {
        return;
    }

    if (!info || !info->ExceptionRecord) {
        print_line("CRASH: unknown");
        return;
    }

    const EXCEPTION_RECORD& rec = *info->ExceptionRecord;
    const void* address = rec.ExceptionAddress;

    print_line("============================================================");
    print_line("CRASH");
    print_line("============================================================");
    print_line("Code:    0x%08X (%s)", rec.ExceptionCode, exception_name(rec.ExceptionCode));
    print_line("Address: 0x%p", address);
    print_module(const_cast<void*>(address));

    if (rec.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec.NumberParameters >= 2) {
        const ULONG_PTR op = rec.ExceptionInformation[0];
        const ULONG_PTR addr = rec.ExceptionInformation[1];
        print_line("Access:  %s at 0x%p",
            op == 0 ? "read" : (op == 1 ? "write" : "execute"),
            reinterpret_cast<void*>(addr));
    }

    if (info->ContextRecord) {
#if defined(_M_X64) || defined(__x86_64__)
        const CONTEXT& ctx = *info->ContextRecord;
        print_line("RIP=%p  RSP=%p  RBP=%p",
            reinterpret_cast<void*>(ctx.Rip),
            reinterpret_cast<void*>(ctx.Rsp),
            reinterpret_cast<void*>(ctx.Rbp));
        print_line("RAX=%p  RBX=%p  RCX=%p  RDX=%p",
            reinterpret_cast<void*>(ctx.Rax),
            reinterpret_cast<void*>(ctx.Rbx),
            reinterpret_cast<void*>(ctx.Rcx),
            reinterpret_cast<void*>(ctx.Rdx));
#endif
        print_stack(info->ContextRecord);
    }

    print_line("============================================================");
    print_line("holding console 60s");
    print_line("also in %s", config::kLogFileName);
    print_line("============================================================");
    fflush(stdout);
    fflush(stderr);
    Sleep(60000);
}

LONG WINAPI unhandled_filter(EXCEPTION_POINTERS* info) {
    if (!should_ignore(info)) {
        report(info);
    }
    if (g_previous_filter) {
        return g_previous_filter(info);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK vectored_continue(EXCEPTION_POINTERS* info) {
    if (should_ignore(info)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (info && info->ExceptionRecord &&
        (info->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)) {
        report(info);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT ||
        ctrl_type == CTRL_BREAK_EVENT ||
        ctrl_type == CTRL_CLOSE_EVENT) {
        LOG_INFO("Crash", "console closed, game still running");
        FreeConsole();
        return TRUE;
    }
    return FALSE;
}

void on_terminate() {
    if (g_shutting_down.load()) {
        return;
    }
    print_line("CRASH: std::terminate()");
    fflush(stdout);
    Sleep(60000);
    std::abort();
}

void on_purecall() {
    print_line("CRASH: purecall");
    fflush(stdout);
    Sleep(60000);
    std::abort();
}

void on_invalid_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned, uintptr_t) {
    if (g_shutting_down.load()) {
        return;
    }
    print_line("CRASH: invalid parameter");
    fflush(stdout);
    Sleep(60000);
}

} // namespace

void install() {
    if (g_installed.exchange(true)) {
        return;
    }

    g_shutting_down.store(false);
    SetConsoleTitleW(L"Il2cppBase log");
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    g_previous_filter = SetUnhandledExceptionFilter(unhandled_filter);
    g_vectored = AddVectoredContinueHandler(1, vectored_continue);
    std::set_terminate(on_terminate);
    _set_purecall_handler(on_purecall);
    _set_invalid_parameter_handler(on_invalid_parameter);
    _set_abort_behavior(_WRITE_ABORT_MSG | _CALL_REPORTFAULT, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    LOG_INFO("Crash", "crash handler on");
}

void shutdown() {
    g_shutting_down.store(true);
    if (!g_installed.exchange(false)) {
        return;
    }
    SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
    if (g_vectored) {
        RemoveVectoredContinueHandler(g_vectored);
        g_vectored = nullptr;
    }
    SetUnhandledExceptionFilter(g_previous_filter);
    g_previous_filter = nullptr;
}

} // namespace il2cpp_base::crash
