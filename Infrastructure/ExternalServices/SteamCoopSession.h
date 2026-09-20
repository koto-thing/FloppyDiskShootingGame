#pragma once
#pragma warning(push)
#pragma warning(disable: 4996 4828)
#include <steam/steam_api.h>
#include <steam/isteamnetworkingmessages.h>
#include <steam/isteamnetworkingutils.h>
#pragma warning(pop)
#include <chrono>
#include <string>
#include "../../Application/UseCases/CooperativeFrames.h"

/** @brief Steamの招待ロビーと2人分の固定更新入力を管理する */
class SteamCoopSession {
public:
    /** @brief Steam APIを終了する @return なし */
    ~SteamCoopSession();
    /** @brief Steamを初期化し起動時の招待を読み取る @param commandLine 起動引数 @return なし */
    void Initialize(const wchar_t* commandLine);
    /** @brief コールバックと受信を処理する @return なし */
    void Poll();
    /** @brief ロビーを作るか保留中の招待へ参加する @return なし */
    void OpenLobby();
    /** @brief 招待による画面遷移要求を取得する @return 要求があればtrue */
    bool TakeLobbyRequest();
    /** @brief Steamのフレンド招待画面を開く @return なし */
    void Invite();
    /** @brief ロビーと通信を終了する @return なし */
    void Leave();
    /** @brief 自分の機体を変更する @return なし */
    void CycleShot();
    /** @brief ホストが難易度を変更する @return なし */
    void CycleDifficulty();
    /** @brief 自分の準備完了状態を切り替える @return なし */
    void ToggleReady();
    /** @brief ホストが両者へ開始設定を送る @return なし */
    void Start();
    /** @brief 固定更新用入力を送り同期済み入力を取得する @param local 自分の入力（送信済み押下を消費） @param inputs 出力先 @return 更新可能ならtrue */
    bool Step(CooperativeInput& local, std::array<CooperativeInput, 2>& inputs);
    /** @brief API利用可否を取得する @return 利用可能ならtrue */
    bool Available() const { return m_initialized; }
    /** @brief ホスト判定 @return 自分が作成者ならtrue */
    bool IsHost() const;
    /** @brief ロビー参加状態を取得する @return 参加中ならtrue */
    bool InLobby() const { return m_lobby.IsValid(); }
    /** @brief 指定プレイヤーの参加状態を取得する @param player ホスト0、ゲスト1 @return 参加済みならtrue */
    bool HasPlayer(int player) const { return Member(player).IsValid(); }
    /** @brief 開始状態を取得する @return 開始済みならtrue */
    bool InGame() const { return m_inGame; }
    /** @brief 通信エラーを取得する @return 失敗した場合true */
    bool Failed() const { return m_failed; }
    /** @brief オーバーレイの表示状態を取得する @return 表示中ならtrue */
    bool OverlayActive() const { return m_overlayActive; }
    /** @brief 開始条件を取得する @return 両者が同じビルドで準備済みならtrue */
    bool CanStart() const;
    /** @brief 表示用状態を取得する @return 状態文字列 */
    const char* Status() const { return m_status.c_str(); }
    /** @brief 現在の難易度を取得する @return 0から2 */
    int Difficulty() const;
    /** @brief 指定プレイヤーの機体を取得する @param player ホスト0、ゲスト1 @return 0から2 */
    int Shot(int player) const;
    /** @brief 指定プレイヤーの準備状態を取得する @param player ホスト0、ゲスト1 @return 準備済みならtrue */
    bool Ready(int player) const;
    /** @brief 同期開始用乱数を取得する @return 非ゼロの乱数種 */
    unsigned Seed() const { return m_seed; }
private:
    friend struct SteamCoopSessionTests;
    /** @brief ロビー作成結果を処理する @param result 作成結果 @param failure 通信失敗 @return なし */
    void OnCreated(LobbyCreated_t* result, bool failure);
    /** @brief ロビー参加結果を処理する @param result 参加結果 @param failure 通信失敗 @return なし */
    void OnEntered(LobbyEnter_t* result, bool failure);
    /** @brief 招待を保留する @param event Steamの招待 @return なし */
    STEAM_CALLBACK_MANUAL(SteamCoopSession, OnInvite, GameLobbyJoinRequested_t, m_inviteCallback);
    /** @brief ロビーメンバー以外の接続を拒否する @param event 接続要求 @return なし */
    STEAM_CALLBACK_MANUAL(SteamCoopSession, OnSessionRequest, SteamNetworkingMessagesSessionRequest_t, m_sessionRequestCallback);
    /** @brief 通信失敗を記録する @param event 切断情報 @return なし */
    STEAM_CALLBACK_MANUAL(SteamCoopSession, OnSessionFailed, SteamNetworkingMessagesSessionFailed_t, m_sessionFailedCallback);
    /** @brief Steamオーバーレイの操作中はゲームを止める @param event 表示状態 @return なし */
    STEAM_CALLBACK_MANUAL(SteamCoopSession, OnOverlay, GameOverlayActivated_t, m_overlayCallback);
    /** @brief メンバーを取得する @param player ホスト0、ゲスト1 @return Steam ID */
    CSteamID Member(int player) const;
    /** @brief 失敗を記録し更新を止める @param message 表示文 @return なし */
    void Fail(const char* message);
    /** @brief 固定長メッセージを送る @param packet 8ワードの通信データ @return 送信成功ならtrue */
    bool Send(const std::array<std::uint32_t, 8>& packet);
    /** @brief ゲーム開始を確定する @param seed 乱数種 @return なし */
    void Begin(unsigned seed);
    CCallResult<SteamCoopSession, LobbyCreated_t> m_created;
    CCallResult<SteamCoopSession, LobbyEnter_t> m_entered;
    CSteamID m_lobby, m_peer, m_owner, m_pendingLobby;
    bool m_initialized = false, m_inGame = false, m_failed = false, m_lobbyRequest = false;
    bool m_opening = false;
    bool m_overlayActive = false;
    std::string m_status = "STEAM IS NOT RUNNING";
    CooperativeFrames m_frames;
    unsigned m_seed = 1;
    int m_difficulty = 0;
    std::array<int, 2> m_shots {};
    std::chrono::steady_clock::time_point m_lastReceive, m_openStarted;
};
