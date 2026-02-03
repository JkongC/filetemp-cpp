#pragma once

#include <format>
#include <memory>
#include <source_location>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ft
{
constinit inline std::shared_ptr<spdlog::logger> stdoutLogger{};
constinit inline std::shared_ptr<spdlog::logger> stderrLogger{};

inline void validate_stdout_logger()
{
    if (!stdoutLogger)
    {
        stdoutLogger = spdlog::stdout_color_mt("info");
        stdoutLogger->set_level(spdlog::level::info);
        stdoutLogger->set_pattern("%v");
    }
}

inline void validate_stderr_logger()
{
    if (!stderrLogger)
    {
        stderrLogger = spdlog::stdout_color_mt("err");
        stderrLogger->set_level(spdlog::level::warn);
        stderrLogger->set_pattern("%^[%l]%$ %v");
    }
}

template <typename... Args>
inline void log_info(spdlog::format_string_t<Args...> fmt, Args &&...args)
{
    validate_stdout_logger();

    stdoutLogger->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_info(std::source_location loc, spdlog::format_string_t<Args...> fmt, Args &&...args)
{
    validate_stdout_logger();

    stdoutLogger->info(std::format("\"{}\"({}:{}): {}\n---(In function: {})",
                                   loc.file_name(),
                                   loc.line(),
                                   loc.column(),
                                   std::format(fmt, std::forward<Args>(args)...),
                                   loc.function_name()));
}

template <typename... Args>
inline void log_err(spdlog::format_string_t<Args...> fmt, Args &&...args)
{
    validate_stderr_logger();

    stderrLogger->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_err(std::source_location loc, spdlog::format_string_t<Args...> fmt, Args &&...args)
{
    validate_stderr_logger();

    stderrLogger->info(std::format("\"{}\"({}:{}): {}\n---(In function: {})",
                                   loc.file_name(),
                                   loc.line(),
                                   loc.column(),
                                   std::format(fmt, std::forward<Args>(args)...),
                                   loc.function_name()));
}

#ifdef FT_DEBUG
struct WithSourceLocation
{
    std::source_location loc;

    WithSourceLocation(std::source_location l = std::source_location::current())
        : loc(l)
    {
    }

    template <typename... Args>
    void log_info(this const WithSourceLocation &self, spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        ::ft::log_info(self.loc, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_err(this const WithSourceLocation &self, spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        ::ft::log_err(self.loc, fmt, std::forward<Args>(args)...);
    }
};
#else
struct WithSourceLocation
{
    template <typename... Args>
    void log_info(this const WithSourceLocation &self, spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        ::ft::log_info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_err(this const WithSourceLocation &self, spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        ::ft::log_err(fmt, std::forward<Args>(args)...);
    }
};
#endif
} // namespace ft