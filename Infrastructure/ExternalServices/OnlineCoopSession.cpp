#include "OnlineCoopSession.h"
#include "BuildVersion.h"
#include "../Repositories/UserDataPath.h"
#include <fstream>
#include <windows.h>
#include <winhttp.h>
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <sstream>

namespace {
// 揺らぎを吸収できている間も必要な先行入力を保持する
constexpr auto InputRecoveryHold = std::chrono::seconds(5);
/** @brief 空白区切りの非負整数を厳密に読み取る @param input 入力 @param value 出力 @return 正常ならtrue */
bool Number(std::istream& input, unsigned& value) {
    std::string word;
    if (!(input >> word)) return false;
    const auto result = std::from_chars(word.data(), word.data() + word.size(), value);
    return result.ec == std::errc {} && result.ptr == word.data() + word.size();
}
/** @brief 末尾に余分な値がないか調べる @param input 入力 @return 末尾ならtrue */
bool End(std::istream& input) { input >> std::ws; return input.eof(); }
/** @brief 部屋コードを検証する @param code 入力 @return 6桁ならtrue */
bool RoomCodeValid(const std::string& code) {
    return code.size() == 6 && code.find_first_not_of("0123456789") == std::string::npos;
}
/** @brief 配布先の検証とHTTP接続を共通化する @param session セッション出力 @param secure TLS使用の出力 @param asynchronous 非同期接続か @return 接続ハンドル */
HINTERNET Connect(HINTERNET& session, bool& secure, bool asynchronous = false) {
    // 接続先は配布時に設定でき、平文HTTPはローカル開発だけ許可する
    wchar_t configured[2048] {};
    const DWORD length = GetEnvironmentVariableW(L"SPACEYAKUZA_SERVER_URL", configured, 2048);
    const std::wstring url = length == 0 ? L"https://game.koto-thing.com" : length < 2048 ? configured : L"";
    URL_COMPONENTS parts {};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength =
        parts.dwUserNameLength = parts.dwPasswordLength = static_cast<DWORD>(-1);
    const bool parsed = WinHttpCrackUrl(url.c_str(), 0, 0, &parts) != FALSE;
    const std::wstring host = parsed ? std::wstring(parts.lpszHostName, parts.dwHostNameLength) : L"";
    secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
    const bool valid = parsed && (secure || (parts.nScheme == INTERNET_SCHEME_HTTP &&
        (host == L"127.0.0.1" || host == L"localhost" || host == L"[::1]"))) &&
        !parts.dwUserNameLength && !parts.dwPasswordLength && !parts.dwExtraInfoLength &&
        (parts.dwUrlPathLength == 0 || (parts.dwUrlPathLength == 1 && *parts.lpszUrlPath == L'/'));

    // 接続先が安全な形式ならHTTPセッションを初期化する
    session = valid ? WinHttpOpen(L"SpaceYakuza/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, asynchronous ? WINHTTP_FLAG_ASYNC : 0) : nullptr;
    if (session) {
        WinHttpSetTimeouts(session, 3000, 3000, 3000, 3000);
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    }
    return session ? WinHttpConnect(session, host.c_str(), parts.nPort, 0) : nullptr;
}

}

/** @brief ワーカーを開始する @return なし */
OnlineCoopSession::OnlineCoopSession() : m_worker(&OnlineCoopSession::Work, this) {}

/** @brief 通信終了を待つ @return なし */
OnlineCoopSession::~OnlineCoopSession() {
    LogNetwork();
    StopStream();
    // ワーカーへ停止を通知する
    {
        std::lock_guard lock(m_mutex);
        m_stopping = true;
        m_requests.clear();
    }

    // ワーカースレッドの終了を待つ
    m_wake.notify_one();
    m_worker.join();
}

/** @brief 通信要求を追加する @param request 送信内容 @return なし */
void OnlineCoopSession::Queue(Request request) {
    std::lock_guard lock(m_mutex);
    m_requests.push_back(std::move(request));
    m_wake.notify_one();
}

/** @brief HTTP処理を描画スレッドから分離する @return なし */
void OnlineCoopSession::Work() {
    HINTERNET session = nullptr;
    bool secure = false;
    HINTERNET connection = Connect(session, secure);
    for (;;) {
        // 次の通信要求を取り出す
        Request job;
        {
            std::unique_lock lock(m_mutex);
            m_wake.wait(lock, [this] { return m_stopping || !m_requests.empty(); });
            if (m_stopping) break;
            job = std::move(m_requests.front());
            m_requests.pop_front();
        }

        // HTTP要求を実行して応答を収集する
        Response result {job, {}, false};
        const wchar_t* method = job.operation == Operation::Ranking ? L"GET" : L"POST";
        HINTERNET request = connection ? WinHttpOpenRequest(connection, method, job.path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0) : nullptr;
        if (request) {
            std::wstring headers = L"Content-Type: text/plain; charset=utf-8\r\n";
            if (!job.token.empty()) headers += L"Authorization: Bearer " + std::wstring(job.token.begin(), job.token.end()) + L"\r\n";
            if (WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()),
                job.body.empty() ? nullptr : job.body.data(), static_cast<DWORD>(job.body.size()),
                static_cast<DWORD>(job.body.size()), 0) && WinHttpReceiveResponse(request, nullptr)) {
                DWORD status = 0, size = sizeof(status);
                if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status == 200) {
                    // 応答サイズと総読取時間を制限し、切れた応答は適用しない
                    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                    char buffer[4096];
                    DWORD count = 0;
                    while (std::chrono::steady_clock::now() < deadline &&
                        WinHttpReadData(request, buffer, sizeof(buffer), &count)) {
                        if (!count) { result.success = true; break; }
                        if (result.body.size() + count > 32768) break;
                        result.body.append(buffer, count);
                    }
                }
            }
            WinHttpCloseHandle(request);
        }

        // 描画スレッドへ応答を渡す
        {
            std::lock_guard lock(m_mutex);
            if (!m_stopping) m_responses.push_back(std::move(result));
        }
    }
    if (connection) WinHttpCloseHandle(connection);
    if (session) WinHttpCloseHandle(session);
}

/** @brief 常時接続を終了する @return なし */
void OnlineCoopSession::StopStream() {
    // 通信スレッドが保留操作をキャンセルしてから回収する
    {
        std::lock_guard lock(m_mutex);
        m_streamStop = true;
        m_streamOutgoing.clear();
    }
    m_streamWake.notify_one();
    if (m_streamWorker.joinable()) m_streamWorker.join();
}

/** @brief 常時接続の送信と受信を分離する @param token 認証トークン @param generation 接続世代 @return なし */
void OnlineCoopSession::Stream(std::string token, unsigned generation) {
    // 完了通知だけをコールバックで受け取り、ハンドル操作はこのスレッドへ集約する
    struct Completion {
        OnlineCoopSession* owner;
        bool requestSent = false, headers = false, received = false, written = false, failed = false;
        HINTERNET closed = nullptr;
        DWORD count = 0;
        std::chrono::steady_clock::time_point receivedAt {}, writtenAt {};
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type {};
        /** @brief 非同期完了を送信スレッドへ通知する @param handle 接続 @param context 状態 @param status 通知 @param info 結果 @param length 結果長 @return なし */
        static void CALLBACK Notify(HINTERNET handle, DWORD_PTR context, DWORD status, void* info, DWORD length) {
            if (!context) return;
            auto& state = *reinterpret_cast<Completion*>(context);
            {
                std::lock_guard lock(state.owner->m_mutex);
                switch (status) {
                case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE: state.requestSent = true; break;
                case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: state.headers = true; break;
                case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
                    if (length != sizeof(WINHTTP_WEB_SOCKET_STATUS)) { state.failed = true; break; }
                    state.count = static_cast<WINHTTP_WEB_SOCKET_STATUS*>(info)->dwBytesTransferred;
                    state.type = static_cast<WINHTTP_WEB_SOCKET_STATUS*>(info)->eBufferType;
                    state.received = true;
                    state.receivedAt = std::chrono::steady_clock::now();
                    break;
                case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
                    state.written = true;
                    state.writtenAt = std::chrono::steady_clock::now();
                    break;
                case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: state.failed = true; break;
                case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING: state.closed = handle; break;
                }
                state.owner->m_streamWake.notify_one();
            }
        }
    } completion {this};
    // 非同期要求は取り消した後も最後の通知まで状態とバッファを保持する
    const auto close = [&](HINTERNET handle) {
        if (handle && WinHttpCloseHandle(handle)) {
            std::unique_lock lock(m_mutex);
            m_streamWake.wait(lock, [&] { return completion.closed == handle; });
        }
    };
    const auto wait = [&](bool& flag) {
        std::unique_lock lock(m_mutex);
        return m_streamWake.wait_for(lock, std::chrono::seconds(5), [&] { return flag || completion.failed || m_streamStop; }) &&
            flag && !completion.failed && !m_streamStop;
    };

    // Windows標準のWebSocketへ認証済みHTTP接続を昇格する
    HINTERNET session = nullptr;
    bool secure = false;
    HINTERNET connection = Connect(session, secure, true);
    HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", L"/v2/stream", nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0) : nullptr;
    HINTERNET socket = nullptr;
    if (request) {
        auto context = reinterpret_cast<DWORD_PTR>(&completion);
        const std::wstring headers = L"Authorization: Bearer " + std::wstring(token.begin(), token.end()) + L"\r\n";
        DWORD status = 0, size = sizeof(status);
        // 失敗時もHANDLE_CLOSINGを受け取れるよう要求開始前にコンテキストを設定する
        const bool callbackReady = WinHttpSetOption(request, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context)) &&
            WinHttpSetStatusCallback(request, Completion::Notify,
                WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE | WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE |
                WINHTTP_CALLBACK_FLAG_READ_COMPLETE | WINHTTP_CALLBACK_FLAG_WRITE_COMPLETE |
                WINHTTP_CALLBACK_FLAG_REQUEST_ERROR | WINHTTP_CALLBACK_FLAG_HANDLES, 0) != WINHTTP_INVALID_STATUS_CALLBACK;
        if (callbackReady && WinHttpSetOption(request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0) &&
            WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), nullptr, 0, 0, context) &&
            wait(completion.requestSent) && WinHttpReceiveResponse(request, nullptr) && wait(completion.headers) &&
            WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status == 101)
            socket = WinHttpWebSocketCompleteUpgrade(request, context);
        if (callbackReady) close(request);
        else WinHttpCloseHandle(request);
    }
    {
        std::lock_guard lock(m_mutex);
        m_responses.push_back({{Operation::StreamOpen, generation, {}, {}, {}}, {}, socket != nullptr});
    }
    if (socket) {
        // 受信バッファと送信文字列を各完了通知まで保持し、送受信を並行する
        char buffer[4096];
        std::string body, outgoing;
        std::chrono::steady_clock::time_point pingSent {}, sendStarted {};
        bool writing = false;
        bool healthy = WinHttpWebSocketReceive(socket, buffer, sizeof(buffer), nullptr, nullptr) == NO_ERROR;
        while (healthy) {
            bool received = false, send = false;
            DWORD count = 0;
            std::chrono::steady_clock::time_point receivedAt {};
            WINHTTP_WEB_SOCKET_BUFFER_TYPE type {};
            {
                std::unique_lock lock(m_mutex);
                m_streamWake.wait(lock, [&] { return m_streamStop || completion.failed || completion.received ||
                    completion.written || (!writing && !m_streamOutgoing.empty()); });
                if (m_streamStop || completion.failed) break;
                received = completion.received;
                count = completion.count;
                receivedAt = completion.receivedAt;
                type = completion.type;
                completion.received = false;
                if (completion.written) {
                    m_sendMaxMs = (std::max)(m_sendMaxMs, static_cast<unsigned>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(completion.writtenAt - sendStarted).count()));
                    completion.written = false; writing = false; outgoing.clear();
                }
                if (!writing && !m_streamOutgoing.empty()) {
                    m_sendQueueMaxMs = (std::max)(m_sendQueueMaxMs, static_cast<unsigned>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_streamOutgoing.front().queuedAt).count()));
                    outgoing = std::move(m_streamOutgoing.front().body);
                    m_streamOutgoing.pop_front();
                    writing = send = true;
                }
            }
            if (received) {
                // 分割メッセージも上限内で復元し、描画停止中の蓄積も制限する
                if ((type != WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE &&
                    type != WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) || body.size() + count > 32768) break;
                body.append(buffer, count);
                if (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                    std::lock_guard lock(m_mutex);
                    if (m_responses.size() >= 512) break;
                    const unsigned rtt = body.starts_with("PONG ") ? static_cast<unsigned>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(receivedAt - pingSent).count()) : 0;
                    m_responses.push_back({{Operation::StreamData, generation, {}, {}, {}}, std::move(body), true, receivedAt, rtt});
                    body.clear();
                }
                healthy = WinHttpWebSocketReceive(socket, buffer, sizeof(buffer), nullptr, nullptr) == NO_ERROR;
            }
            if (healthy && send) {
                // RTTは実際の送信開始から受信完了までとし、描画待ちを含めない
                sendStarted = std::chrono::steady_clock::now();
                if (outgoing.starts_with("PING ")) pingSent = sendStarted;
                healthy = WinHttpWebSocketSend(socket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                    outgoing.data(), static_cast<DWORD>(outgoing.size())) == NO_ERROR;
            }
        }
        // 非同期操作の取り消しと完了を待ってからバッファを解放する
        close(socket);
        std::lock_guard lock(m_mutex);
        m_responses.push_back({{Operation::StreamData, generation, {}, {}, {}}, {}, false});
    }
    if (connection) WinHttpCloseHandle(connection);
    if (session) WinHttpCloseHandle(session);
}

/** @brief 個人情報を含まない試合ごとの通信統計を保存する @return なし */
void OnlineCoopSession::LogNetwork() {
    if (!m_inGame) return;
    // ログは1MiBを超えた時に切り替え、ディスク障害でゲームを止めない
    const auto path = UserDataPath() / L"online-network.log";
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return;
    const auto size = std::filesystem::file_size(path, error);
    std::ofstream output(path, !error && size > 1048576 ? std::ios::trunc : std::ios::app);
    // 通信スレッドの計測値だけをロック内で読み取り、ファイル書き込みはロック外で行う
    unsigned sendQueueMaxMs = 0, sendMaxMs = 0;
    {
        std::lock_guard lock(m_mutex);
        sendQueueMaxMs = m_sendQueueMaxMs;
        sendMaxMs = m_sendMaxMs;
    }
    const auto waiting = m_waitStarted == std::chrono::steady_clock::time_point{} ? 0u :
        static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_waitStarted).count());
    // 複数試行の追記を日時とビルドで区別する
    SYSTEMTIME utc {};
    GetSystemTime(&utc);
    char timestamp[32] {};
    sprintf_s(timestamp, "%04u-%02u-%02uT%02u:%02u:%02uZ", utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond);
    output << "logged_at=" << timestamp << " build=\"" << SPACEYAKUZA_BUILD_LABEL << "\" transport=websocket delay_frames=" << m_inputDelay << " rtt_ms=" << m_rttMs << " rtt_max_ms=" << m_rttMaxMs
        << " main_thread_lag_max_ms=" << m_mainLagMaxMs << " input_gap_max_ms=" << m_frameGapMs
        << " send_queue_max_ms=" << sendQueueMaxMs << " send_max_ms=" << sendMaxMs << " waits=" << m_waitCount
        << " send_ahead_frames=" << m_sendAhead << " send_ahead_max_frames=" << m_sendAheadMax
        << " wait_total_ms=" << m_waitMs + waiting << " wait_max_ms=" << (std::max)(m_waitMaxMs, waiting)
        << " failure_reason=" << m_failureReason << " status=" << m_status << '\n';
}

/** @brief 部屋の接続処理を開始する @param automatic 自動検索するか @param code 参加コード @return なし */
void OnlineCoopSession::OpenLobby(bool automatic, const std::string& code) {
    // 入力された部屋コードを検証する
    if (!code.empty() && !RoomCodeValid(code)) { m_status = "ENTER A SIX DIGIT ROOM CODE"; return; }

    // 前回の接続を終了して新しい接続を準備する
    Leave();
    m_active = true;
    m_status = "CONNECTING TO SERVER";
    m_openCommand = automatic ? "MATCH\n" : code.empty() ? "CREATE\n" : "JOIN " + code + "\n";

    // 実行環境を含むビルド識別子を作る
    std::string build = SPACEYAKUZA_BUILD_LABEL;
    std::replace(build.begin(), build.end(), ' ', '_');
#if defined(_WIN64)
    build += "-x64";
#else
    build += "-x86";
#endif
#ifdef _DEBUG
    build += "-debug";
#else
    build += "-release";
#endif

    // セッション作成要求を送る
    Queue({Operation::Open, m_generation, L"/v1/session", "2 " + build + "\n", {}});
}

/** @brief 待機と部屋を終了する @return なし */
void OnlineCoopSession::Leave() {
    LogNetwork();
    StopStream();
    // キャンセル済みの未送信要求は新しい接続を待たせない
    {
        std::lock_guard lock(m_mutex);
        std::erase_if(m_requests, [](const Request& request) {
            return request.operation == Operation::Open;
        });
    }
    if (!m_token.empty()) Queue({Operation::Leave, m_generation, L"/v1/leave", {}, m_token});

    // 接続状態とロビー状態を初期化する
    ++m_generation;
    m_token.clear(); m_room.clear();
    m_active = m_failed = m_inGame = false;
    m_failureReason = "none";
    m_ready = {}; m_shots = {};
    m_streamReady = m_calibrated = false;
    m_inputDelay = CooperativeFrames::Delay;
    m_sendAhead = m_sendAheadMax = m_inputDelay;
    m_nextAheadReduction = {};
    m_pingPending = m_rttMs = m_rttMaxMs = m_waitCount = m_waitMs = m_waitMaxMs = m_frameGapMs = 0;
    m_mainLagMaxMs = 0;
    m_sendQueueMaxMs = m_sendMaxMs = 0;
    m_nextPing = m_waitStarted = {};

    m_members = m_player = m_difficulty = 0;
    m_status = "SELECT MATCHMAKING OR A PRIVATE ROOM";
}

/** @brief 進行を停止する @param message 表示内容 @param reason ログ用の原因識別子 @return なし */
void OnlineCoopSession::Fail(const char* message, const char* reason) {
    // 最初の失敗を保持し、後続の無通信判定で原因を上書きしない
    if (m_failed) return;
    // 失敗後にタイトルへ戻るまでの時間を入力待ちへ加算しない
    if (m_waitStarted != std::chrono::steady_clock::time_point{}) {
        const auto ms = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_waitStarted).count());
        m_waitMs += ms;
        m_waitMaxMs = (std::max)(m_waitMaxMs, ms);
        m_waitStarted = {};
    }
    m_failed = true;
    m_status = message;
    m_failureReason = reason;
}

/** @brief 操作を送信バッファへ追加する @param command 操作行 @return なし */
void OnlineCoopSession::Command(const std::string& command) {
    if (!m_active || m_failed || !m_streamReady) return;
    // 次回のPollを待たず、入力を取得した固定更新で送信を開始する
    {
        std::lock_guard lock(m_mutex);
        if (command.size() > 8192 || m_streamOutgoing.size() >= 128) { Fail("SEND BUFFER FULL - RETURN TO TITLE"); return; }
        m_streamOutgoing.push_back({command, std::chrono::steady_clock::now()});
    }
    m_streamWake.notify_one();
}

/** @brief 機体を変更する @return なし */
void OnlineCoopSession::CycleShot() {
    if (InLobby() && !m_inGame) Command("SHOT " + std::to_string((Shot(m_player) + 1) % 3) + "\n");
}
/** @brief 難易度を変更する @return なし */
void OnlineCoopSession::CycleDifficulty() {
    if (InLobby() && IsHost() && !m_inGame && !Ready(0) && !Ready(1))
        Command("DIFFICULTY " + std::to_string((m_difficulty + 1) % 3) + "\n");
}
/** @brief 準備状態を切り替える @return なし */
void OnlineCoopSession::ToggleReady() {
    if (InLobby() && !m_inGame) Command(Ready(m_player) ? "READY 0\n" : "READY 1\n");
}
/** @brief 開始要求を送る @return なし */
void OnlineCoopSession::Start() { if (IsHost() && CanStart()) Command("START\n"); }

/** @brief 状態と入力行を検証して取り込む @param body 応答 @return 正常ならtrue */
bool OnlineCoopSession::Receive(const std::string& body) {
    // 応答を行単位で解析する
    std::istringstream lines(body);
    std::string line, kind;
    bool stateSeen = false;
    while (std::getline(lines, line)) {
        std::istringstream input(line);
        if (!(input >> kind)) return false;

        // サーバーエラーをゲーム状態へ反映する
        if (kind == "ERROR") {
            const char* message = "SERVER REJECTED REQUEST - RETURN AND RETRY";
            if (line == "ERROR ROOM_UNAVAILABLE") message = "ROOM NOT FOUND, FULL, OR DIFFERENT BUILD";
            if (line == "ERROR PEER_LEFT") message = "PARTNER DISCONNECTED - RETURN TO TITLE";
            Fail(message, "server_error");
            return true;
        }

        // ロビー状態を検証して更新する
        if (kind == "STATE" && !stateSeen) {
            std::string room;
            unsigned v[11] {};
            if (!(input >> room) || !RoomCodeValid(room)) return false;
            for (auto& value : v) if (!Number(input, value)) return false;
            if (!End(input) || v[0] > 1 || v[1] < 1 || v[1] > 2 || v[0] >= v[1] ||
                v[2] > 2 || v[3] > 2 || v[4] > 2 || v[5] > 1 || v[6] > 1 || v[7] > 1 || !v[8] ||
                v[9] < CooperativeFrames::Delay || v[9] > 30 || v[10] > 1) return false;
            if (m_inGame && (room != m_room || v[0] != static_cast<unsigned>(m_player) || v[1] != 2 ||
                v[2] != static_cast<unsigned>(m_difficulty) || v[3] != static_cast<unsigned>(m_shots[0]) ||
                v[4] != static_cast<unsigned>(m_shots[1]) || !v[7] || v[8] != m_seed || v[9] != m_inputDelay)) return false;
            if (v[7] && (v[1] != 2 || !v[5] || !v[6] || !v[10])) return false;
            m_room = room; m_player = static_cast<int>(v[0]); m_members = static_cast<int>(v[1]);
            m_difficulty = static_cast<int>(v[2]); m_shots = {static_cast<int>(v[3]), static_cast<int>(v[4])};
            m_ready = {v[5] != 0, v[6] != 0}; m_seed = v[8];
            m_inputDelay = v[9]; m_calibrated = v[10] != 0;
            if (v[7] && !m_inGame) {
                m_frames.Reset(m_inputDelay);
                m_lastFrame = std::chrono::steady_clock::now();
                m_sendAhead = m_sendAheadMax = m_inputDelay;
                m_nextAheadReduction = m_lastFrame + InputRecoveryHold;
            }
            m_inGame = v[7] != 0;
            m_status = m_inGame ? "CONNECTED" : m_members == 2 ? "SELECT SHOT AND PRESS READY" : "WAITING FOR A PARTNER";
            stateSeen = true;
        } else if (kind == "FRAME" && stateSeen && m_inGame) {
            // 相手の入力フレームを検証して保存する
            unsigned v[4] {};
            for (auto& value : v) if (!Number(input, value)) return false;
            if (!End(input) || v[0] != static_cast<unsigned>(1 - m_player) ||
                v[2] > CooperativeInput::ValidButtons || v[3] > CooperativeInput::ValidButtons ||
                !m_frames.Push(static_cast<int>(v[0]), v[1], {static_cast<std::uint16_t>(v[2]), static_cast<std::uint16_t>(v[3])})) return false;
            const auto now = std::chrono::steady_clock::now();
            const auto gapMs = static_cast<unsigned>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrame).count());
            m_frameGapMs = (std::max)(m_frameGapMs, gapMs);
            // 3フレーム分を超える到着間隔は、待機に至らなくても回線の揺らぎとして扱う
            if (gapMs > 50) m_nextAheadReduction = now + InputRecoveryHold;
            m_lastFrame = now;
        } else return false;
    }
    return stateSeen;
}

/** @brief 完了通信を反映し入力を交換する @return なし */
void OnlineCoopSession::Poll() {
    // ワーカーが返した応答を描画スレッドへ取り込む
    std::deque<Response> replies;
    {
        std::lock_guard lock(m_mutex);
        replies.swap(m_responses);
    }
    for (const auto& reply : replies) {
        const auto& request = reply.request;

        // 退出要求の応答は状態更新から除外する
        if (request.operation == Operation::Leave) continue;

        // ランキング応答を検証して保存する
        if (request.operation == Operation::Ranking) {
            const int mode = request.cooperative ? 1 : 0;
            m_rankPending[mode] = false;
            ScoreRepository::Rankings rankings {};
            std::istringstream input(reply.body);
            std::string tag;
            bool valid = reply.success && (input >> tag) && tag == "RANKS";
            for (auto& scores : rankings) for (auto& score : scores) {
                unsigned value = 0;
                valid = Number(input, value) && value <= 999999999 && valid;
                score = static_cast<int>(value);
            }
            for (const auto& scores : rankings)
                valid = std::is_sorted(scores.begin(), scores.end(), std::greater<int>()) && valid;
            if (valid && End(input)) { m_rankings[mode] = rankings; m_rankStatus[mode] = "GLOBAL RANKING - CLIENT REPORTED SCORES"; }
            else m_rankStatus[mode] = "RANKING UNAVAILABLE - REOPEN TO RETRY";
            continue;
        }

        // スコア送信結果を表示状態へ反映する
        if (request.operation == Operation::Score) {
            m_scoreStatus = reply.success && reply.body == "OK\n" ? "SCORE UPLOADED" : "UPLOAD FAILED - LOCAL SCORE IS SAVED";
            continue;
        }

        // 古い世代の接続結果を破棄する
        if (request.generation != m_generation) {
            // 戻る操作の後に発行されたセッションも退出させる
            if (request.operation == Operation::Open && reply.success && reply.body.size() == 65 &&
                reply.body.back() == '\n' && reply.body.substr(0, 64).find_first_not_of("0123456789abcdef") == std::string::npos)
                Queue({Operation::Leave, request.generation, L"/v1/leave", {}, reply.body.substr(0, 64)});
            continue;
        }

        // 現在の接続結果を状態へ反映する
        if (m_failed) continue;
        if (!reply.success) { Fail("SERVER UNAVAILABLE - CHECK SERVER URL AND RETRY", "transport_error"); continue; }
        if (request.operation == Operation::Open) {
            if (reply.body.size() != 65 || reply.body.back() != '\n' ||
                reply.body.substr(0, 64).find_first_not_of("0123456789abcdef") != std::string::npos) {
                Fail("INVALID SERVER SESSION"); continue;
            }
            m_token = reply.body.substr(0, 64);
            // 入力専用接続はランキング通信と別のスレッドで進める
            {
                std::lock_guard lock(m_mutex);
                m_streamStop = false;
            }
            m_streamWorker = std::thread(&OnlineCoopSession::Stream, this, m_token, m_generation);
        } else if (request.operation == Operation::StreamOpen) {
            m_streamReady = true;
            m_lastReceived = std::chrono::steady_clock::now();
            Command(m_openCommand);
        } else {
            m_lastReceived = std::chrono::steady_clock::now();
            m_mainLagMaxMs = (std::max)(m_mainLagMaxMs, static_cast<unsigned>(
                std::chrono::duration_cast<std::chrono::milliseconds>(m_lastReceived - reply.receivedAt).count()));
            if (reply.body.starts_with("PONG ")) {
                std::istringstream input(reply.body.substr(5));
                unsigned id = 0;
                if (!Number(input, id) || !End(input) || !m_pingPending || id != m_pingPending) {
                    Fail("INVALID SERVER DATA - RETURN TO TITLE"); continue;
                }
                m_rttMs = reply.roundTripMs;
                m_rttMaxMs = (std::max)(m_rttMaxMs, m_rttMs);
                m_pingPending = 0;
                if (m_rttMs > 10000) { Fail("PARTNER TIMED OUT - RETURN TO TITLE", "rtt_timeout"); continue; }
                Command("LATENCY " + std::to_string(m_rttMs) + "\n");
            } else if (!Receive(reply.body)) Fail("INVALID SERVER DATA - RETURN TO TITLE");
        }
    }
    const auto now = std::chrono::steady_clock::now();
    if (!m_active || m_failed || !m_streamReady) return;
    if (now - m_lastReceived > std::chrono::seconds(10) ||
        (m_pingPending && now - m_pingSent > std::chrono::seconds(10))) {
        Fail("PARTNER TIMED OUT - RETURN TO TITLE", now - m_lastReceived > std::chrono::seconds(10) ? "receive_timeout" : "ping_timeout"); return;
    }

    // 応答を待たずに入力を送り、独立したPINGで往復時間を計測する
    {
        std::lock_guard lock(m_mutex);
        if (m_streamOutgoing.size() >= 128) { Fail("SEND BUFFER FULL - RETURN TO TITLE"); return; }
        if (!m_pingPending && now >= m_nextPing) {
            if (++m_pingId == 0) ++m_pingId;
            m_pingPending = m_pingId;
            m_pingSent = now;
            m_nextPing = now + std::chrono::milliseconds(m_inGame ? 1000 : 250);
            m_streamOutgoing.push_back({"PING " + std::to_string(m_pingId) + "\n", now});
        }
    }
    m_streamWake.notify_one();
}

/** @brief 入力を送信して同期フレームを取り出す @param local 自分の入力 @param inputs 出力先 @return 更新可能ならtrue */
bool OnlineCoopSession::Step(CooperativeInput& local, std::array<CooperativeInput, 2>& inputs) {
    if (!m_inGame || m_failed) return false;
    const auto now = std::chrono::steady_clock::now();

    // 到着の揺らぎも5秒間収まったら、先行量を0.5秒ごとに1フレームずつ戻す
    if (m_sendAhead > m_inputDelay && m_waitStarted == std::chrono::steady_clock::time_point{} && now >= m_nextAheadReduction) {
        --m_sendAhead;
        m_nextAheadReduction = now + std::chrono::milliseconds(500);
    }

    // 自分の入力を送信バッファへ追加する
    if (m_frames.CanSubmit(m_player, m_sendAhead)) {
        const auto frame = m_frames.Next(m_player);
        Command("FRAME " + std::to_string(frame) + " " + std::to_string(local.held) + " " + std::to_string(local.pressed) + "\n");
        if (m_failed || !m_frames.Push(m_player, frame, local)) { Fail("INVALID LOCAL INPUT"); return false; }
        local.pressed = 0;
    }

    // 両者の入力が揃ったら固定更新へ渡す
    const bool advanced = m_frames.Pop(inputs);
    // 入力を必要とするプレイ中だけ監視し、クリア後のエンディングをタイムアウト扱いにしない
    if (!advanced && now - m_lastFrame > std::chrono::seconds(30)) {
        Fail("PARTNER TIMED OUT - RETURN TO TITLE", "input_timeout");
        return false;
    }
    if (!advanced) {
        // ponytail: 追加先行は100msか合計30フレームまで、それ以上の遅延は待機し、解消にはロールバック等が必要
        if (!m_frames.CanSubmit(m_player, m_sendAhead) && m_sendAhead < (std::min)(30u, m_inputDelay + 6)) {
            ++m_sendAhead;
            m_sendAheadMax = (std::max)(m_sendAheadMax, m_sendAhead);
        }
        m_nextAheadReduction = now + InputRecoveryHold;
    }
    if (!advanced && m_waitStarted == std::chrono::steady_clock::time_point{}) {
        m_waitStarted = now;
        ++m_waitCount;
    } else if (advanced && m_waitStarted != std::chrono::steady_clock::time_point{}) {
        const auto ms = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_waitStarted).count());
        m_waitMs += ms;
        m_waitMaxMs = (std::max)(m_waitMaxMs, ms);
        m_waitStarted = {};
    }
    return advanced;
}

/** @brief 共有ランキングを取得する @param cooperative 協力用か @return なし */
void OnlineCoopSession::FetchRankings(bool cooperative) {
    const int mode = cooperative ? 1 : 0;
    if (m_rankPending[mode]) return;
    m_rankPending[mode] = true;
    m_rankStatus[mode] = "LOADING GLOBAL RANKING";
    Queue({Operation::Ranking, 0, cooperative ? L"/v1/rankings?coop=1" : L"/v1/rankings?coop=0", {}, {}, cooperative});
}

/** @brief スコアを送信する @param difficulty 難易度 @param score 得点 @param cooperative 協力用か @return なし */
void OnlineCoopSession::SubmitScore(int difficulty, int score, bool cooperative) {
    if (difficulty < 0 || difficulty > 2 || score <= 0 || score > 999999999) return;
    m_scoreStatus = "UPLOADING SCORE";
    Queue({Operation::Score, 0, L"/v1/scores", std::to_string(difficulty) + " " +
        (cooperative ? "1 " : "0 ") + std::to_string(score) + "\n", {}, cooperative});
}
