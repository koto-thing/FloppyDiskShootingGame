#include "Debug.h"

#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace {
/**
 * @brief デバッグ状態を表す構造体
 */
struct DebugState {
std::mutex mutex;
std::ofstream file;
bool initialized = false;
};

/**
 * @brief デバッグ状態を取得する
 * @return DebugState& デバッグ状態の参照
 */
DebugState& GetState() {
    static DebugState state;
    return state;
}

/**
 * @brief ログレベルを文字列に変換する
 * @param level ログレベル
 * @return const char* ログレベルの文字列
 */
const char* ToString(LogLevel level) {
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

/**
 * @brief 実行可能ファイルのディレクトリを取得する
 * @return std::filesystem::path 実行可能ファイルのディレクトリ
 */
std::filesystem::path GetExecutableDirectory() {
    // Windows API を使用して実行可能ファイルのパスを取得し、ディレクトリ部分を返す
    std::wstring path(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) {
        return std::filesystem::current_path();
    }
    
    // 文字列の長さを正確に設定して、余分な null 文字を削除
    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

/**
 * @brief 現在のタイムスタンプを文字列として取得する
 * @return std::string 現在のタイムスタンプの文字列
 */
std::string BuildTimestamp() {
    // 現在の時刻を取得し、ミリ秒まで含めたフォーマットで文字列に変換する
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds 
        = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    // Windows環境でのローカルタイム変換
    std::tm localTime{};
    localtime_s(&localTime, &time);

    // 文字列ストリームを使用してフォーマットされたタイムスタンプを作成
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    return stream.str();
}

/**
 * @brief UTF-8文字列をワイド文字列に変換する
 * @param text UTF-8文字列
 * @return std::wstring ワイド文字列
 */
std::wstring Utf8ToWide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    
    // UTF-8からワイド文字列への変換に必要なバッファサイズを取得
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    
    // 変換後のワイド文字列を格納するためのバッファを作成し、変換を実行
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}
} // namespace

/**
 * @brief デバッグシステムを初期化する
 * @return true: 初期化成功, false: 初期化失敗
 */
bool Debug::Initialize() {
    // デバッグ状態を取得し、初期化済みかどうかを確認する
    DebugState& state = GetState();
    std::scoped_lock lock(state.mutex);
    if (state.initialized) {
        return state.file.is_open();
    }

    // ディレクトリを作成し、ログファイルを開く
    state.initialized = true;
    std::error_code error;
    const std::filesystem::path logDirectory = GetExecutableDirectory() / L"Logs";
    std::filesystem::create_directories(logDirectory, error);
    if (!error) {
        state.file.open(logDirectory / L"latest.log", std::ios::out | std::ios::trunc | std::ios::binary);
    }
    
    // デバッグ出力に初期化メッセージを出力する
    return state.file.is_open();
}

/**
 * @brief デバッグシステムをシャットダウンする
 */
void Debug::Shutdown() {
    // デバッグ状態を取得し、ログファイルを閉じる
    DebugState& state = GetState();
    std::scoped_lock lock(state.mutex);
    if (state.file.is_open()) {
        state.file.flush();
        state.file.close();
    }
    
    // 初期化フラグをリセットする
    state.initialized = false;
}

/**
 * @brief デバッグメッセージをログに出力する
 * @param message ログメッセージ
 * @param location ソースコードの位置情報
 */
void Debug::Log(std::string_view message, const std::source_location& location) {
#if defined(_DEBUG)
    Write(LogLevel::Info, message, location);
#else
    (void)message;
    (void)location;
#endif
}

/**
 * @brief 警告メッセージをログに出力する
 * @param message 警告メッセージ
 * @param location ソースコードの位置情報
 */
void Debug::LogWarning(std::string_view message, const std::source_location& location) {
    Write(LogLevel::Warning, message, location);
}

/**
 * @brief エラーメッセージをログに出力する
 * @param message エラーメッセージ
 * @param location ソースコードの位置情報
 */
void Debug::LogError(std::string_view message, const std::source_location& location) {
    Write(LogLevel::Error, message, location);
}

/**
 * @brief HRESULTエラーコードをログに出力する
 * @param message エラーメッセージ
 * @param result HRESULTエラーコード
 * @param location ソースコードの位置情報
 */
void Debug::LogHResult(
    std::string_view message,
    long result,
    const std::source_location& location) {
    
    // HRESULTコードを16進数形式でフォーマットしてログメッセージを作成
    std::ostringstream stream;
    stream << message << " (HRESULT: 0x"
           << std::uppercase << std::hex << std::setfill('0') << std::setw(8)
           << static_cast<unsigned long>(result) << ')';
    Write(LogLevel::Error, stream.str(), location);
}

/**
 * @brief デバッグメッセージをログに出力する内部関数
 * @param level ログレベル
 * @param message ログメッセージ
 * @param location ソースコードの位置情報
 */
void Debug::Write(
    LogLevel level,
    std::string_view message,
    const std::source_location& location) {
    
    // ログメッセージのフォーマットを作成する
    const std::filesystem::path sourcePath(location.file_name());
    std::ostringstream stream;
    stream << '[' << BuildTimestamp() << "] [" << ToString(level) << "] ";
    if (level != LogLevel::Info) {
        stream << sourcePath.filename().string() << ':' << location.line() << ' ';
    }
    
    // メッセージを追加して改行する
    stream << message << '\n';
    const std::string line = stream.str();

    // デバッグ出力にメッセージを出力する (UTF-8 -> ワイド文字列変換)
    const std::wstring wideLine = Utf8ToWide(line);
    if (!wideLine.empty()) {
        OutputDebugStringW(wideLine.c_str());
    } else {
        OutputDebugStringA(line.c_str());
    }

    // ログファイルにメッセージを書き込む
    DebugState& state = GetState();
    std::scoped_lock lock(state.mutex);
    if (state.file.is_open()) {
        state.file.write(line.data(), static_cast<std::streamsize>(line.size()));
        state.file.flush();
    }
}
