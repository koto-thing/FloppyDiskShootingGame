#pragma once

#include <source_location>
#include <string_view>

enum class LogLevel {
    Info,
    Warning,
    Error,
};

class Debug final {
public:
    Debug() = delete;

    static bool Initialize();
    static void Shutdown();

    static void Log(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    static void LogWarning(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    static void LogError(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    static void LogHResult(
        std::string_view message,
        long result,
        const std::source_location& location = std::source_location::current());

private:
    static void Write(
        LogLevel level,
        std::string_view message,
        const std::source_location& location);
};
