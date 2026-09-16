#pragma once
#include "../../Application/UseCases/CooperativeFrames.h"
#include "../Repositories/ScoreRepository.h"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

/** @brief Go HTTPサーバー経由のロビー、入力同期、共有ランキング */
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
    /** @brief 開始条件を取得する @return 2人とも準備済みならtrue */
    bool CanStart() const { return InLobby() && !m_failed && !m_inGame && m_members == 2 && m_ready[0] && m_ready[1]; }
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
    enum class Operation { Open, Exchange, Leave, Ranking, Score };
    struct Request {
        Operation operation;
        unsigned generation;
        std::wstring path;
        std::string body, token;
        bool cooperative = false;
    };
    struct Response { Request request; std::string body; bool success = false; };
    /** @brief 通信をキューへ追加する @param request 送信内容 @return なし */
    void Queue(Request request);
    /** @brief ワーカースレッドでHTTPを実行する @return なし */
    void Work();
    /** @brief 操作を次回通信へ追加する @param command プロトコル行 @return なし */
    void Command(const std::string& command);
    /** @brief 応答を検証して適用する @param body 応答本文 @return 正常ならtrue */
    bool Receive(const std::string& body);
    /** @brief 通信異常で進行を停止する @param message 表示内容 @return なし */
    void Fail(const char* message);
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<Request> m_requests;
    std::deque<Response> m_responses;
    bool m_stopping = false;
    std::thread m_worker;
    unsigned m_generation = 0;
    bool m_active = false, m_pending = false, m_failed = false, m_inGame = false;
    int m_player = 0, m_members = 0, m_difficulty = 0;
    unsigned m_seed = 1;
    std::array<int, 2> m_shots {};
    std::array<bool, 2> m_ready {};
    std::string m_token, m_room, m_commands, m_openCommand;
    std::string m_status = "SELECT MATCHMAKING OR A PRIVATE ROOM";
    CooperativeFrames m_frames;
    std::chrono::steady_clock::time_point m_nextPoll {}, m_lastFrame {};
    std::array<ScoreRepository::Rankings, 2> m_rankings {};
    std::array<bool, 2> m_rankPending {};
    std::array<std::string, 2> m_rankStatus {"", ""};
    std::string m_scoreStatus;
};
