#include "ui/overlay.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <mutex>

#include "config.hpp"
#include "core/hooks.hpp"
#include "core/logger.hpp"
#include "core/main_thread.hpp"
#include "features/features.hpp"
#include "menu.hpp"
#include "ui/cursor.hpp"
#include "ui/style.hpp"
#include "ui/widgets.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wparam, LPARAM lparam);

namespace il2cpp_base::ui {

Options& options() {
    static Options opts;
    return opts;
}

} // namespace il2cpp_base::ui

namespace il2cpp_base::ui::overlay {
namespace {

using present_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using resize_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

struct State {
    present_fn present = nullptr;
    resize_fn resize = nullptr;
    WNDPROC wndproc = nullptr;

    HWND hwnd = nullptr;
    IDXGISwapChain* swap = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    UINT tick_msg = 0;

    // The game's window procedure (main thread) and Present (render thread) both touch ImGui's input queue.
    std::recursive_mutex input_mutex;

    std::atomic<bool> installed{ false };
    std::atomic<bool> imgui_ready{ false };
    std::atomic<bool> menu_open{ false };
    std::atomic<bool> tick_pending{ false };
    bool key_was_down = false;
    float menu_alpha = 0.0f;
};

State g;

bool swapchain_functions(void*& present, void*& resize) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"Il2cppBaseDummy";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
        nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    bool ok = false;
    for (const D3D_DRIVER_TYPE driver : { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP }) {
        IDXGISwapChain* swap = nullptr;
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                &desc, &swap, &device, nullptr, &context))) {
            void** vtable = *reinterpret_cast<void***>(swap);
            present = vtable[8];
            resize = vtable[13];
            swap->Release();
            device->Release();
            context->Release();
            ok = true;
            break;
        }
    }

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

DXGI_FORMAT view_format(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default: return format;
    }
}

void release_rtv() {
    if (g.rtv) {
        g.rtv->Release();
        g.rtv = nullptr;
    }
}

bool ensure_rtv(IDXGISwapChain* swap) {
    if (g.rtv) {
        return true;
    }
    ID3D11Texture2D* back = nullptr;
    if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back)))) {
        return false;
    }
    D3D11_TEXTURE2D_DESC tex{};
    back->GetDesc(&tex);

    D3D11_RENDER_TARGET_VIEW_DESC view{};
    view.Format = view_format(tex.Format);
    view.ViewDimension = tex.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;

    const HRESULT hr = g.device->CreateRenderTargetView(back, &view, &g.rtv);
    back->Release();
    if (FAILED(hr)) {
        LOG_ERROR("Overlay", "CreateRenderTargetView failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

bool should_block(UINT msg) {
    const ImGuiIO& io = ImGui::GetIO();
    if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_INPUT) {
        return io.WantCaptureMouse;
    }
    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR) {
        return io.WantTextInput;
    }
    return false;
}

LRESULT CALLBACK hk_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (g.tick_msg != 0 && msg == g.tick_msg) {
        g.tick_pending.store(false);
        main_thread::run_pending();
        cursor::update();
        features::update();
        return 0;
    }

    if (g.imgui_ready.load()) {
        const bool open = g.menu_open.load();
        if (open || msg != WM_SETCURSOR) {
            LRESULT handled = 0;
            {
                std::lock_guard lock(g.input_mutex);
                handled = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
            }
            if (open && handled) {
                return handled;
            }
            if (open && options().block_input && should_block(msg)) {
                return msg == WM_INPUT ? DefWindowProcW(hwnd, msg, wparam, lparam) : 0;
            }
        }
    }

    return CallWindowProcW(g.wndproc, hwnd, msg, wparam, lparam);
}

bool setup(IDXGISwapChain* swap) {
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swap->GetDesc(&desc)) ||
        FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g.device)))) {
        return false;
    }
    g.device->GetImmediateContext(&g.context);
    g.hwnd = desc.OutputWindow;
    g.swap = swap;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    style::load_fonts();
    const float dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(g.hwnd);
    style::set_scale(dpi > 1.0f ? dpi : 1.0f);
    style::apply_pending();

    ImGui_ImplWin32_Init(g.hwnd);
    ImGui_ImplDX11_Init(g.device, g.context);

    g.imgui_ready.store(true);
    g.wndproc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g.hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&hk_wndproc)));

    LOG_INFO("Overlay", "ImGui ready on HWND %p", g.hwnd);
    return true;
}

// Returns false when `swap` belongs to some other window or device.
bool adopt_swapchain(IDXGISwapChain* swap) {
    if (swap == g.swap) {
        return true;
    }
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swap->GetDesc(&desc)) || desc.OutputWindow != g.hwnd) {
        return false;
    }
    ID3D11Device* device = nullptr;
    if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)))) {
        return false;
    }
    device->Release();
    if (device != g.device) {
        return false;
    }
    release_rtv();
    g.swap = swap;
    return true;
}

void poll_menu_key() {
    const bool down = (GetAsyncKeyState(config::kMenuKey) & 0x8000) != 0;
    if (down && !g.key_was_down && GetForegroundWindow() == g.hwnd) {
        g.menu_open.store(!g.menu_open.load());
    }
    g.key_was_down = down;
}

void request_tick() {
    bool expected = false;
    if (g.tick_pending.compare_exchange_strong(expected, true) && !PostMessageW(g.hwnd, g.tick_msg, 0, 0)) {
        g.tick_pending.store(false);
    }
}

void draw_watermark() {
    char text[128]{};
    snprintf(text, sizeof(text), "%s   %.0f fps", config::kName, ImGui::GetIO().Framerate);

    ImFont* font = style::semibold();
    const float size = ImGui::GetFontSize();
    const ImVec2 text_size = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    const ImVec2 pad(style::px(10.0f), style::px(6.0f));
    const ImVec2 min(style::px(12.0f), style::px(12.0f));
    const ImVec2 max(min.x + text_size.x + pad.x * 2.0f, min.y + text_size.y + pad.y * 2.0f);
    const float rounding = style::px(6.0f);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(min, max, IM_COL32(11, 11, 12, 225), rounding);
    dl->AddRect(min, max, style::color::kBorder, rounding);
    dl->AddText(font, size, ImVec2(min.x + pad.x, min.y + pad.y), style::color::kText, text);
}

void render(IDXGISwapChain* swap) {
    if (!g.imgui_ready.load() && !setup(swap)) {
        return;
    }
    if (!adopt_swapchain(swap) || !ensure_rtv(swap)) {
        return;
    }

    poll_menu_key();
    request_tick();
    style::apply_pending();

    const bool open = g.menu_open.load();
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = open && options().software_cursor;
    if (open) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }

    ImGui_ImplDX11_NewFrame();
    {
        std::lock_guard lock(g.input_mutex);
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    features::draw();
    if (options().watermark) {
        draw_watermark();
    }

    g.menu_alpha += ((open ? 1.0f : 0.0f) - g.menu_alpha) * (1.0f - std::exp(-18.0f * io.DeltaTime));
    if (g.menu_alpha > 0.01f) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g.menu_alpha);
        menu::draw();
        ImGui::PopStyleVar();
    }
    draw_notifications();

    ImGui::Render();

    ID3D11RenderTargetView* prev_rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    ID3D11DepthStencilView* prev_dsv = nullptr;
    g.context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, prev_rtv, &prev_dsv);
    g.context->OMSetRenderTargets(1, &g.rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g.context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, prev_rtv, prev_dsv);
    for (ID3D11RenderTargetView* view : prev_rtv) {
        if (view) {
            view->Release();
        }
    }
    if (prev_dsv) {
        prev_dsv->Release();
    }
}

HRESULT WINAPI hk_present(IDXGISwapChain* swap, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) {
        render(swap);
    }
    return g.present(swap, sync, flags);
}

HRESULT WINAPI hk_resize(IDXGISwapChain* swap, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags) {
    if (swap == g.swap) {
        release_rtv();
    }
    return g.resize(swap, count, width, height, format, flags);
}

} // namespace

bool install() {
    if (g.installed.load()) {
        return true;
    }
    if (!hooks::init()) {
        return false;
    }

    void* present = nullptr;
    void* resize = nullptr;
    if (!swapchain_functions(present, resize)) {
        LOG_ERROR("Overlay", "could not create a D3D11 device to find IDXGISwapChain");
        return false;
    }

    g.tick_msg = RegisterWindowMessageW(L"Il2cppBase.Tick");
    if (!hooks::install("DXGI.Present", present, &hk_present, &g.present)) {
        return false;
    }
    if (!hooks::install("DXGI.ResizeBuffers", resize, &hk_resize, &g.resize)) {
        hooks::remove("DXGI.Present");
        return false;
    }

    g.installed.store(true);
    return true;
}

void uninstall() {
    if (!g.installed.exchange(false)) {
        return;
    }
    hooks::remove("DXGI.Present");
    hooks::remove("DXGI.ResizeBuffers");

    if (g.imgui_ready.exchange(false)) {
        if (g.hwnd && g.wndproc) {
            SetWindowLongPtrW(g.hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g.wndproc));
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        release_rtv();
        if (g.context) {
            g.context->Release();
            g.context = nullptr;
        }
        if (g.device) {
            g.device->Release();
            g.device = nullptr;
        }
    }
}

bool menu_open() {
    return g.menu_open.load();
}

void set_menu_open(bool open) {
    g.menu_open.store(open);
}

const char* menu_key_name() {
    const ImGuiKey key = ImGui_ImplWin32_KeyEventToImGuiKey(static_cast<WPARAM>(config::kMenuKey), 0);
    return key != ImGuiKey_None ? ImGui::GetKeyName(key) : "Menu key";
}

} // namespace il2cpp_base::ui::overlay
