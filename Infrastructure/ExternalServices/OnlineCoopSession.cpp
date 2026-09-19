#include "OnlineCoopSession.h"
#include "BuildVersion.h"
#include <windows.h>
#include <winhttp.h>
#include <algorithm>
#include <charconv>
#include <sstream>

namespace {
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
}

/** @brief ワーカーを開始する @return なし */
OnlineCoopSession::OnlineCoopSession() : m_worker(&OnlineCoopSession::Work, this) {}

/** @brief 通信終了を待つ @return なし */
OnlineCoopSession::~OnlineCoopSession() {
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
    const bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
    const bool valid = parsed && (secure || (parts.nScheme == INTERNET_SCHEME_HTTP &&
        (host == L"127.0.0.1" || host == L"localhost" || host == L"[::1]"))) &&
        !parts.dwUserNameLength && !parts.dwPasswordLength && !parts.dwExtraInfoLength &&
        (parts.dwUrlPathLength == 0 || (parts.dwUrlPathLength == 1 && *parts.lpszUrlPath == L'/'));

    // 接続先が安全な形式ならHTTPセッションを初期化する
    HINTERNET session = valid ? WinHttpOpen(L"SpaceYakuza/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0) : nullptr;
    if (session) {
        WinHttpSetTimeouts(session, 3000, 3000, 3000, 3000);
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    }
    HINTERNET connection = session ? WinHttpConnect(session, host.c_str(), parts.nPort, 0) : nullptr;
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

/** @brief 部屋の接続処理を開始する @param automatic 自動検索するか @param code 参加コード @return なし */
void OnlineCoopSession::OpenLobby(bool automatic, const std::string& code) {
    // 入力された部屋コードを検証する
    if (!code.empty() && !RoomCodeValid(code)) { m_status = "ENTER A SIX DIGIT ROOM CODE"; return; }

    // 前回の接続を終了して新しい接続を準備する
    Leave();
    m_active = m_pending = true;
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
    Queue({Operation::Open, m_generation, L"/v1/session", "1 " + build + "\n", {}});
}

/** @brief 待機と部屋を終了する @return なし */
void OnlineCoopSession::Leave() {
    // キャンセル済みの未送信要求は新しい接続を待たせない
    {
        std::lock_guard lock(m_mutex);
        std::erase_if(m_requests, [](const Request& request) {
            return request.operation == Operation::Open || request.operation == Operation::Exchange;
        });
    }
    if (!m_token.empty()) Queue({Operation::Leave, m_generation, L"/v1/leave", {}, m_token});

    // 接続状態とロビー状態を初期化する
    ++m_generation;
    m_token.clear(); m_room.clear(); m_commands.clear();
    m_active = m_pending = m_failed = m_inGame = false;
    m_ready = {}; m_shots = {};
    m_members = m_player = m_difficulty = 0;
    m_status = "SELECT MATCHMAKING OR A PRIVATE ROOM";
}

/** @brief 進行を停止する @param message 表示内容 @return なし */
void OnlineCoopSession::Fail(const char* message) { m_failed = true; m_status = message; }

/** @brief 操作を送信バッファへ追加する @param command 操作行 @return なし */
void OnlineCoopSession::Command(const std::string& command) {
    if (!m_active || m_failed) return;
    if (m_commands.size() + command.size() > 8192) { Fail("SEND BUFFER FULL - RETURN TO TITLE"); return; }
    m_commands += command;
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
            m_status = "SERVER REJECTED REQUEST - RETURN AND RETRY";
            if (line == "ERROR ROOM_UNAVAILABLE") m_status = "ROOM NOT FOUND, FULL, OR DIFFERENT BUILD";
            if (line == "ERROR PEER_LEFT") m_status = "PARTNER DISCONNECTED - RETURN TO TITLE";
            m_failed = true;
            return true;
        }

        // ロビー状態を検証して更新する
        if (kind == "STATE" && !stateSeen) {
            std::string room;
            unsigned v[9] {};
            if (!(input >> room) || !RoomCodeValid(room)) return false;
            for (auto& value : v) if (!Number(input, value)) return false;
            if (!End(input) || v[0] > 1 || v[1] < 1 || v[1] > 2 || v[0] >= v[1] ||
                v[2] > 2 || v[3] > 2 || v[4] > 2 || v[5] > 1 || v[6] > 1 || v[7] > 1 || !v[8]) return false;
            if (m_inGame && (room != m_room || v[0] != static_cast<unsigned>(m_player) || v[1] != 2 ||
                v[2] != static_cast<unsigned>(m_difficulty) || v[3] != static_cast<unsigned>(m_shots[0]) ||
                v[4] != static_cast<unsigned>(m_shots[1]) || !v[7] || v[8] != m_seed)) return false;
            if (v[7] && (v[1] != 2 || !v[5] || !v[6])) return false;
            m_room = room; m_player = static_cast<int>(v[0]); m_members = static_cast<int>(v[1]);
            m_difficulty = static_cast<int>(v[2]); m_shots = {static_cast<int>(v[3]), static_cast<int>(v[4])};
            m_ready = {v[5] != 0, v[6] != 0}; m_seed = v[8];
            if (v[7] && !m_inGame) { m_frames.Reset(); m_lastFrame = std::chrono::steady_clock::now(); }
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
            m_lastFrame = std::chrono::steady_clock::now();
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
        m_pending = false;
        if (!reply.success) { Fail("SERVER UNAVAILABLE - CHECK SERVER URL AND RETRY"); continue; }
        if (request.operation == Operation::Open) {
            if (reply.body.size() != 65 || reply.body.back() != '\n' ||
                reply.body.substr(0, 64).find_first_not_of("0123456789abcdef") != std::string::npos) {
                Fail("INVALID SERVER SESSION"); continue;
            }
            m_token = reply.body.substr(0, 64);
            Command(m_openCommand);
        } else if (!Receive(reply.body)) Fail("INVALID SERVER DATA - RETURN TO TITLE");
    }
    const auto now = std::chrono::steady_clock::now();
    if (m_inGame && now - m_lastFrame > std::chrono::seconds(30)) Fail("PARTNER TIMED OUT - RETURN TO TITLE");
    if (!m_active || m_failed || m_pending || m_token.empty() || now < m_nextPoll) return;

    // 次回の入力交換を予約する
    // ponytail: HTTP交換は少人数向けで往復遅延に制限される、大規模化時はWebSocketへ置換する
    Queue({Operation::Exchange, m_generation, L"/v1/exchange", std::move(m_commands), m_token});
    m_commands.clear();
    m_pending = true;
    m_nextPoll = now + std::chrono::milliseconds(m_inGame ? 8 : 150);
}

/** @brief 入力を送信して同期フレームを取り出す @param local 自分の入力 @param inputs 出力先 @return 更新可能ならtrue */
bool OnlineCoopSession::Step(CooperativeInput& local, std::array<CooperativeInput, 2>& inputs) {
    if (!m_inGame || m_failed) return false;

    // 自分の入力を送信バッファへ追加する
    if (m_frames.CanSubmit(m_player)) {
        const auto frame = m_frames.Next(m_player);
        Command("FRAME " + std::to_string(frame) + " " + std::to_string(local.held) + " " + std::to_string(local.pressed) + "\n");
        if (m_failed || !m_frames.Push(m_player, frame, local)) { Fail("INVALID LOCAL INPUT"); return false; }
        local.pressed = 0;
    }

    // 両者の入力が揃ったら固定更新へ渡す
    return m_frames.Pop(inputs);
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
