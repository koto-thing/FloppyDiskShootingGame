#pragma once
#include "../../Application/UseCases/CooperativeFrames.h"
#include "../Repositories/ScoreRepository.h"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

/** @brief GoサーバーとのWebSocket入力同期とHTTP共有ランキング */
class OnlineCoopSession {
public:
    /** @brief 通信専用スレッドを開始する @return なし */
    OnlineCoopSession();
    /** @brief 保留通信を終了してスレッドを回収する @return なし */
    ~OnlineCoopSession();
    /** @brief 受信結果と定期通信を処理する @return なし */
    void Poll();
    /** @brief 新しい匿名セッションを開始する @param automatic 自動マッチングするか @param code 空なら部屋作成、6桁なら参加 @return なし */
    void OpenLobby(bool automatic, const std::string& code = {});
    /** @brief 待機と部屋を退出する @return なし */
    void Leave();
    /** @brief 自分の機体を変更する @return なし */
    void CycleShot();
    /** @brief ホストが難易度を変更する @return なし */
    void CycleDifficulty();
    /** @brief 自分の準備状態を切り替える @return なし */
    void ToggleReady();
    /** @brief ホストがゲーム開始を要求する @return なし */
    void Start();
    /** @brief 同期済み入力を取得する @param local 自分の入力 @param inputs 出力先 @return 更新可能ならtrue */
    bool Step(CooperativeInput& local, std::array<CooperativeInput, 2>& inputs);
    /** @brief ロビー参加状態を取得する @return 参加中ならtrue */
    bool InLobby() const { return !m_room.empty(); }
    /** @brief 接続待機状態を取得する @return 接続処理中ならtrue */
    bool Active() const { return m_active; }
    /** @brief ホスト判定 @return 自分がホストならtrue */
    bool IsHost() const { return m_player == 0; }
    /** @brief 開始状態を取得する @return 開始済みならtrue */
    bool InGame() const { return m_inGame; }
    /** @brief 接続失敗を取得する @return 失敗ならtrue */
    bool Failed() const { return m_failed; }
    /** @brief 共通ゲーム入力用オーバーレイ状態 @return 常にfalse */
    bool OverlayActive() const { return false; }
    /** @brief 開始条件を取得する @return 2人とも準備と回線計測が完了したらtrue */
    bool CanStart() const { return InLobby() && !m_failed && !m_inGame && m_calibrated && m_members == 2 && m_ready[0] && m_ready[1]; }
    /** @brief 合意済みの入力遅延を取得する @return フレーム数 */
    unsigned InputDelay() const { return m_inputDelay; }
    /** @brief 現在の入力送信先行量を取得する @return フレーム数 */
    unsigned InputLookahead() const { return m_sendAhead; }
    /** @brief 直近の往復遅延を取得する @return ミリ秒 */
    unsigned RoundTripMs() const { return m_rttMs; }
    /** @brief 表示状態を取得する @return 状態文字列 */
    const char* Status() const { return m_status.c_str(); }
    /** @brief 部屋コードを取得する @return 6桁のコード */
    const std::string& RoomCode() const { return m_room; }
    /** @brief 難易度を取得する @return 0から2 */
    int Difficulty() const { return m_difficulty; }
    /** @brief 機体を取得する @param player プレイヤー番号 @return 0から2 */
    int Shot(int player) const { return m_shots[player]; }
    /** @brief 準備状態を取得する @param player プレイヤー番号 @return 準備済みならtrue */
    bool Ready(int player) const { return m_ready[player]; }
    /** @brief 同期用乱数種を取得する @return 非ゼロの乱数種 */
    unsigned Seed() const { return m_seed; }
    /** @brief ランキングを非同期取得する @param cooperative 協力用か @return なし */
    void FetchRankings(bool cooperative);
    /** @brief スコアを非同期送信する @param difficulty 難易度 @param score 得点 @param cooperative 協力用か @return なし */
    void SubmitScore(int difficulty, int score, bool cooperative);
    /** @brief 取得済みランキングを返す @param cooperative 協力用か @return 難易度別上位5件 */
    const ScoreRepository::Rankings& Rankings(bool cooperative) const { return m_rankings[cooperative ? 1 : 0]; }
    /** @brief ランキング通信状態を返す @param cooperative 協力用か @return 表示用状態 */
    const char* RankingStatus(bool cooperative) const { return m_rankStatus[cooperative ? 1 : 0].c_str(); }
    /** @brief スコア送信状態を返す @return 表示用状態 */
    const char* ScoreStatus() const { return m_scoreStatus.c_str(); }
private:
    enum class Operation { Open, StreamOpen, StreamData, Leave, Ranking, Score };
    struct Request {
        Operation operation;
        unsigned generation;
        std::wstring path;
        std::string body, token;
        bool cooperative = false;
    };
    struct Response {
        Request request;
        std::string body;
        bool success = false;
        std::chrono::steady_clock::time_point receivedAt {};
        unsigned roundTripMs = 0;
    };
    /** @brief 通信をキューへ追加する @param request 送信内容 @return なし */
    void Queue(Request request);
    /** @brief ワーカースレッドでHTTPを実行する @return なし */
    void Work();
    /** @brief 常時接続で送信と受信を並行する @param token 認証トークン @param generation 接続世代 @return なし */
    void Stream(std::string token, unsigned generation);
    /** @brief 常時接続を終了してスレッドを回収する @return なし */
    void StopStream();
    /** @brief 通信統計を保存する @return なし */
    void LogNetwork();
    /** @brief 操作を通信スレッドへ直ちに渡す @param command プロトコル行 @return なし */
    void Command(const std::string& command);
    /** @brief 応答を検証して適用する @param body 応答本文 @return 正常ならtrue */
    bool Receive(const std::string& body);
    /** @brief 通信異常で進行を停止する @param message 表示内容 @param reason ログ用の原因識別子 @return なし */
    void Fail(const char* message, const char* reason = "local_error");
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<Request> m_requests;
    std::deque<Response> m_responses;
    bool m_stopping = false;
    std::thread m_worker;
    std::condition_variable m_streamWake;
    std::thread m_streamWorker;
    bool m_streamStop = false, m_streamReady = false, m_calibrated = false;
    struct Outgoing { std::string body; std::chrono::steady_clock::time_point queuedAt; };
    std::deque<Outgoing> m_streamOutgoing;
    unsigned m_sendQueueMaxMs = 0, m_sendMaxMs = 0;
    unsigned m_generation = 0;
    bool m_active = false, m_failed = false, m_inGame = false;
    int m_player = 0, m_members = 0, m_difficulty = 0;
    unsigned m_seed = 1;
    std::array<int, 2> m_shots {};
    std::array<bool, 2> m_ready {};
    std::string m_token, m_room, m_openCommand;
    std::string m_status = "SELECT MATCHMAKING OR A PRIVATE ROOM";
    const char* m_failureReason = "none";
    CooperativeFrames m_frames;
    std::chrono::steady_clock::time_point m_nextPing {}, m_pingSent {}, m_lastReceived {}, m_lastFrame {}, m_waitStarted {};
    std::chrono::steady_clock::time_point m_nextAheadReduction {};
    unsigned m_inputDelay = CooperativeFrames::Delay, m_rttMs = 0, m_rttMaxMs = 0, m_pingId = 0, m_pingPending = 0;
    unsigned m_sendAhead = CooperativeFrames::Delay, m_sendAheadMax = CooperativeFrames::Delay;
    unsigned m_waitCount = 0, m_waitMs = 0, m_waitMaxMs = 0, m_frameGapMs = 0;
    unsigned m_mainLagMaxMs = 0;
    std::array<ScoreRepository::Rankings, 2> m_rankings {};
    std::array<bool, 2> m_rankPending {};
    std::array<std::string, 2> m_rankStatus {"", ""};
    std::string m_scoreStatus;
};
