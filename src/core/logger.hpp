#pragma once

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace il2cpp_base::log {

enum class Level : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

struct Line {
    Level level;
    std::string text;
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void init(std::wstring_view log_path, bool attach_console);
    void shutdown();

    void set_level(Level level) { level_ = level; }
    Level level() const { return level_; }

    void log(Level level, std::string_view tag, const char* fmt, ...);
    void logv(Level level, std::string_view tag, const char* fmt, va_list args);

    std::vector<Line> history() const;
    void clear_history();
    // Changes every time a line is added or the history is cleared.
    std::uint64_t version() const { return version_.load(std::memory_order_acquire); }

private:
    Logger() = default;

    static constexpr std::size_t kHistoryLimit = 500;

    static const char* level_name(Level level);
    static WORD console_color(Level level);
    void write_console(Level level, std::string_view line);
    void write_file(std::string_view line);

    mutable std::mutex mutex_;
    std::ofstream file_;
    Level level_ = Level::Debug;
    bool console_attached_ = false;
    std::deque<Line> history_;
    std::atomic<std::uint64_t> version_{ 0 };
};

inline void Logger::init(std::wstring_view log_path, bool attach_console) {
    std::lock_guard lock(mutex_);

    if (!file_.is_open()) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(log_path).parent_path(), ec);
        file_.open(std::filesystem::path(log_path), std::ios::out | std::ios::trunc);
    }

    if (attach_console && !console_attached_ && GetConsoleWindow() == nullptr && AllocConsole()) {
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);
        SetConsoleTitleW(L"Il2cppBase log");
        console_attached_ = true;
    }
}

inline void Logger::shutdown() {
    std::lock_guard lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

inline const char* Logger::level_name(Level level) {
    switch (level) {
    case Level::Trace: return "TRACE";
    case Level::Debug: return "DEBUG";
    case Level::Info:  return "INFO ";
    case Level::Warn:  return "WARN ";
    case Level::Error: return "ERROR";
    case Level::Fatal: return "FATAL";
    }
    return "?????";
}

inline WORD Logger::console_color(Level level) {
    switch (level) {
    case Level::Trace: return FOREGROUND_INTENSITY;
    case Level::Debug: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case Level::Info:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case Level::Warn:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case Level::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY;
    case Level::Fatal: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

inline void Logger::write_console(Level level, std::string_view line) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!out || out == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    const bool has_info = GetConsoleScreenBufferInfo(out, &info) != FALSE;
    if (has_info) {
        SetConsoleTextAttribute(out, console_color(level));
    }

    fwrite(line.data(), 1, line.size(), stdout);
    fputc('\n', stdout);
    fflush(stdout);

    if (has_info) {
        SetConsoleTextAttribute(out, info.wAttributes);
    }
}

inline void Logger::write_file(std::string_view line) {
    if (!file_.is_open()) {
        return;
    }
    file_.write(line.data(), static_cast<std::streamsize>(line.size()));
    file_.put('\n');
    file_.flush();
}

inline void Logger::logv(Level level, std::string_view tag, const char* fmt, va_list args) {
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    char message[2048]{};
    vsnprintf(message, sizeof(message), fmt, args);

    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &tt);

    char line[2304]{};
    snprintf(line, sizeof(line), "[%02d:%02d:%02d.%03d] [%s] [%.*s] %s",
        local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(ms.count()),
        level_name(level), static_cast<int>(tag.size()), tag.data(), message);

    std::lock_guard lock(mutex_);
    write_file(line);
    write_console(level, line);

    history_.push_back(Line{ level, line });
    if (history_.size() > kHistoryLimit) {
        history_.pop_front();
    }
    version_.fetch_add(1, std::memory_order_release);
}

inline void Logger::log(Level level, std::string_view tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logv(level, tag, fmt, args);
    va_end(args);
}

inline std::vector<Line> Logger::history() const {
    std::lock_guard lock(mutex_);
    return { history_.begin(), history_.end() };
}

inline void Logger::clear_history() {
    std::lock_guard lock(mutex_);
    history_.clear();
    version_.fetch_add(1, std::memory_order_release);
}

#define LOG_TRACE(tag, ...) ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...) ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Debug, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)  ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Info,  tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)  ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Warn,  tag, __VA_ARGS__)
#define LOG_ERROR(tag, ...) ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Error, tag, __VA_ARGS__)
#define LOG_FATAL(tag, ...) ::il2cpp_base::log::Logger::instance().log(::il2cpp_base::log::Level::Fatal, tag, __VA_ARGS__)

} // namespace il2cpp_base::log
