#pragma once

#include <source_location>
#include <string_view>

/** @brief デバッグログの重要度 */
enum class LogLevel {
    Info,
    Warning,
    Error,
};

class Debug final {
public:
    Debug() = delete;

    /** @brief デバッグログを初期化する */
    static bool Initialize();
    /** @brief デバッグログを終了する */
    static void Shutdown();

    /**
     * @brief 情報ログを出力する
     * @param message 出力するメッセージ
     * @param location 呼び出し元のソース位置
     */
    static void Log(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    /** @brief 配布版でも情報ログを出力する @param message メッセージ @param location 呼び出し元 @return なし */
    static void LogInfo(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    /**
     * @brief 警告ログを出力する
     * @param message 出力するメッセージ
     * @param location 呼び出し元のソース位置
     */
    static void LogWarning(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    /**
     * @brief エラーログを出力する
     * @param message 出力するメッセージ
     * @param location 呼び出し元のソース位置
     */
    static void LogError(
        std::string_view message,
        const std::source_location& location = std::source_location::current());
    /**
     * @brief HRESULTを含むエラーログを出力する
     * @param message 出力するメッセージ
     * @param result 出力するHRESULT
     * @param location 呼び出し元のソース位置
     */
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
