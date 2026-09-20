#include "SteamCoopSession.h"
#include "BuildVersion.h"
#include "../../Engine/Diagnostics/Debug.h"
#include <cstring>
#include <sstream>
#include <cstdlib>

namespace {
constexpr std::uint32_t Magic = 0x53594350, Protocol = 1;
constexpr int Channel = 27;
constexpr const char* Game = "space-yakuza-coop-1";
// ponytail: 同一ビルド・同一CPU構成の入力同期に限定し、異機種対応時は状態スナップショット同期へ移行する
#if defined(_WIN64)
constexpr const char* Build = SPACEYAKUZA_BUILD_LABEL "-x64";
#else
constexpr const char* Build = SPACEYAKUZA_BUILD_LABEL "-x86";
#endif
/** @brief メタデータの選択値を検証する @param text 数値文字列 @return 有効なら0から2、無効なら-1 */
int Choice(const char* text) { return text && text[0] >= '0' && text[0] <= '2' && text[1] == '\0' ? text[0] - '0' : -1; }
}

/** @brief Steam APIを終了する @return なし */
SteamCoopSession::~SteamCoopSession() {
    Leave();
    // Steam終了前に通知と非同期結果の登録を解除する
    m_inviteCallback.Unregister();
    m_sessionRequestCallback.Unregister();
    m_sessionFailedCallback.Unregister();
    m_overlayCallback.Unregister();
    m_created.Cancel();
    m_entered.Cancel();
    if (m_initialized) SteamAPI_Shutdown();
}

/** @brief Steamを初期化する @param commandLine 起動引数 @return なし */
void SteamCoopSession::Initialize(const wchar_t* commandLine) {
    if (m_initialized) return;
    // Steamをレンダラーより先に初期化してオーバーレイを利用する
    _putenv_s("SteamAppId", std::to_string(SPACEYAKUZA_STEAM_APP_ID).c_str());
    SteamErrMsg error {};
    m_initialized = SteamAPI_InitEx(&error) == k_ESteamAPIInitResult_OK;
    if (!m_initialized) {
        Debug::LogError(std::string("Steam initialization failed: ") + error);
        m_status = "START STEAM AND CHECK steam_appid.txt"; return;
    }

    // 初期化済みのSteamへ通知を登録する
    m_inviteCallback.Register(this, &SteamCoopSession::OnInvite);
    m_sessionRequestCallback.Register(this, &SteamCoopSession::OnSessionRequest);
    m_sessionFailedCallback.Register(this, &SteamCoopSession::OnSessionFailed);
    m_overlayCallback.Register(this, &SteamCoopSession::OnOverlay);
    Debug::LogInfo(std::string("Steam initialized: app=") + std::to_string(SteamUtils()->GetAppID()) + " build=" + Build);

    // リレー通信を初期化する
    SteamNetworkingUtils()->InitRelayNetworkAccess();
    m_status = "READY TO CREATE A FRIENDS LOBBY";

    // 未起動時に受けた招待のロビーIDを読み取る
    std::wistringstream arguments(commandLine ? commandLine : L"");
    std::wstring token;
    while (arguments >> token) {
        if (token != L"+connect_lobby") continue;
        std::uint64_t id = 0;
        if (arguments >> id) {
            m_pendingLobby = CSteamID(id);
            m_lobbyRequest = m_pendingLobby.IsLobby();
        }
        break;
    }
}

/** @brief 招待による画面遷移要求を消費する @return 要求があればtrue */
bool SteamCoopSession::TakeLobbyRequest() {
    // 非同期接続の結果が届くまで招待を保持し、進行中の要求との競合を避ける
    if (m_opening || m_created.IsActive() || m_entered.IsActive()) return false;
    const bool requested = m_lobbyRequest;
    m_lobbyRequest = false;
    return requested;
}

/** @brief ロビーを作成するか招待へ参加する @return なし */
void SteamCoopSession::OpenLobby() {
    // Steam APIと直前の非同期要求を確認する
    if (!m_initialized || m_opening) return;
    if (m_created.IsActive() || m_entered.IsActive()) {
        m_status = "PREVIOUS REQUEST PENDING - RETURN AND RETRY";
        return;
    }
    const auto invited = m_pendingLobby;

    // 新しいロビー接続を開始する
    Leave();
    m_pendingLobby.Clear();
    m_opening = true;
    m_openStarted = std::chrono::steady_clock::now();
    m_status = "CONNECTING TO STEAM LOBBY";

    // 保留中の招待があれば参加し、なければロビーを作成する
    if (invited.IsValid()) {
        Debug::LogInfo("Steam JoinLobby: " + std::to_string(invited.ConvertToUint64()));
        m_entered.Set(SteamMatchmaking()->JoinLobby(invited), this, &SteamCoopSession::OnEntered);
    } else {
        Debug::LogInfo("Steam CreateLobby");
        m_created.Set(SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 2), this, &SteamCoopSession::OnCreated);
    }
}

/** @brief ロビー作成結果を処理する @param result 結果 @param failure 通信失敗 @return なし */
void SteamCoopSession::OnCreated(LobbyCreated_t* result, bool failure) {
    Debug::LogInfo("Steam lobby created: result=" + std::to_string(failure ? -1 : static_cast<int>(result->m_eResult)) + " ioFailure=" + std::to_string(failure));
    // 戻る操作やタイムアウトの後に成功した非同期ロビーも必ず退出する
    if (!m_opening) {
        if (!failure && result->m_eResult == k_EResultOK) SteamMatchmaking()->LeaveLobby(CSteamID(result->m_ulSteamIDLobby));
        return;
    }
    m_opening = false;
    if (failure || result->m_eResult != k_EResultOK) { Fail("LOBBY CREATION FAILED - RETURN AND RETRY"); return; }

    // 作成したロビーへゲーム設定を登録する
    m_lobby = CSteamID(result->m_ulSteamIDLobby);
    m_owner = SteamUser()->GetSteamID();
    Debug::LogInfo("Steam host lobby: " + std::to_string(m_lobby.ConvertToUint64()));
    SteamMatchmaking()->SetLobbyData(m_lobby, "game", Game);
    SteamMatchmaking()->SetLobbyData(m_lobby, "build", Build);
    SteamMatchmaking()->SetLobbyData(m_lobby, "difficulty", "0");
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "build", Build);
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "shot", "0");
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "ready", "0");
    m_status = "INVITE A FRIEND WITH THE INVITE BUTTON";
}

/** @brief ロビー参加結果を処理する @param result 結果 @param failure 通信失敗 @return なし */
void SteamCoopSession::OnEntered(LobbyEnter_t* result, bool failure) {
    Debug::LogInfo("Steam lobby entered: response=" + std::to_string(failure ? -1 : static_cast<int>(result->m_EChatRoomEnterResponse)) + " ioFailure=" + std::to_string(failure));
    if (!m_opening) {
        if (!failure && result->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
            SteamMatchmaking()->LeaveLobby(CSteamID(result->m_ulSteamIDLobby));
        return;
    }
    m_opening = false;
    if (failure || result->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
        Fail("LOBBY JOIN FAILED - RETURN AND RETRY"); return;
    }

    // 参加先のゲームとビルドを検証する
    m_lobby = CSteamID(result->m_ulSteamIDLobby);
    m_owner = SteamMatchmaking()->GetLobbyOwner(m_lobby);
    Debug::LogInfo(std::string("Steam lobby metadata: game=") + SteamMatchmaking()->GetLobbyData(m_lobby, "game") +
        " build=" + SteamMatchmaking()->GetLobbyData(m_lobby, "build") + " expected=" + Build);
    if (std::strcmp(SteamMatchmaking()->GetLobbyData(m_lobby, "game"), Game) != 0 ||
        std::strcmp(SteamMatchmaking()->GetLobbyData(m_lobby, "build"), Build) != 0 ||
        SteamMatchmaking()->GetNumLobbyMembers(m_lobby) > 2) {
        Leave(); Fail("USE THE SAME SPACE YAKUZA BUILD AS THE HOST"); return;
    }

    // 自分の初期設定をロビーへ公開する
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "build", Build);
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "shot", "0");
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "ready", "0");
    m_status = "SELECT YOUR SHOT AND PRESS READY";
}

/** @brief ロビー作成者を判定する @return 自分がホストならtrue */
bool SteamCoopSession::IsHost() const { return m_initialized && m_owner == SteamUser()->GetSteamID(); }

/** @brief メンバーを取得する @param player ホスト0、ゲスト1 @return Steam ID */
CSteamID SteamCoopSession::Member(int player) const {
    if (!m_lobby.IsValid()) return {};
    if (player == 0) return m_owner;
    for (int i = 0; i < SteamMatchmaking()->GetNumLobbyMembers(m_lobby); ++i) {
        const auto member = SteamMatchmaking()->GetLobbyMemberByIndex(m_lobby, i);
        if (member != m_owner) return member;
    }
    return {};
}

/** @brief 現在の難易度を取得する @return 0から2、未取得なら-1 */
int SteamCoopSession::Difficulty() const {
    return m_inGame ? m_difficulty : InLobby() ? Choice(SteamMatchmaking()->GetLobbyData(m_lobby, "difficulty")) : 0;
}
/** @brief 機体を取得する @param player ホスト0、ゲスト1 @return 0から2、未参加なら-1 */
int SteamCoopSession::Shot(int player) const {
    return m_inGame ? m_shots[player] : Member(player).IsValid() ?
        Choice(SteamMatchmaking()->GetLobbyMemberData(m_lobby, Member(player), "shot")) : -1;
}
/** @brief 準備状態を取得する @param player ホスト0、ゲスト1 @return 準備済みならtrue */
bool SteamCoopSession::Ready(int player) const {
    return Member(player).IsValid() && std::strcmp(SteamMatchmaking()->GetLobbyMemberData(m_lobby, Member(player), "ready"), "1") == 0;
}
/** @brief 開始条件を検証する @return 両者が同じビルドで準備済みならtrue */
bool SteamCoopSession::CanStart() const {
    // ロビーと相手の基本状態を確認する
    if (m_failed || m_inGame || !InLobby() || !m_peer.IsValid() || Difficulty() < 0) return false;

    // 両者の準備状態とビルドを確認する
    for (int i = 0; i < 2; ++i) {
        if (!Ready(i) || Shot(i) < 0 || std::strcmp(SteamMatchmaking()->GetLobbyMemberData(m_lobby, Member(i), "build"), Build) != 0) return false;
    }
    return true;
}
/** @brief 招待画面を開く @return なし */
void SteamCoopSession::Invite() {
    if (InLobby() && IsHost() && !m_inGame) {
        Debug::LogInfo("Steam invite dialog: lobby=" + std::to_string(m_lobby.ConvertToUint64()));
        SteamFriends()->ActivateGameOverlayInviteDialog(m_lobby);
    }
}
/** @brief 自分の機体を変更する @return なし */
void SteamCoopSession::CycleShot() {
    if (!InLobby() || m_inGame || m_failed) return;
    const char value[] = {static_cast<char>('0' + (Shot(IsHost() ? 0 : 1) + 1) % 3), '\0'};
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "ready", "0");
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "shot", value);
}
/** @brief ホストが難易度を変更する @return なし */
void SteamCoopSession::CycleDifficulty() {
    if (!InLobby() || !IsHost() || m_inGame || m_failed || Ready(0) || Ready(1)) return;
    const char value[] = {static_cast<char>('0' + (Difficulty() + 1) % 3), '\0'};
    SteamMatchmaking()->SetLobbyMemberData(m_lobby, "ready", "0");
    SteamMatchmaking()->SetLobbyData(m_lobby, "difficulty", value);
}
/** @brief 準備状態を切り替える @return なし */
void SteamCoopSession::ToggleReady() {
    if (InLobby() && !m_inGame && !m_failed)
        SteamMatchmaking()->SetLobbyMemberData(m_lobby, "ready", Ready(IsHost() ? 0 : 1) ? "0" : "1");
}

/** @brief 通信データを送信する @param packet 8ワード @return 成功ならtrue */
bool SteamCoopSession::Send(const std::array<std::uint32_t, 8>& packet) {
    // 相手のSteam IDを送信先へ設定する
    SteamNetworkingIdentity identity;
    identity.SetSteamID(m_peer);
    if (SteamNetworkingMessages()->SendMessageToUser(identity, packet.data(), sizeof(packet),
        k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle, Channel) != k_EResultOK) {
        Fail("P2P SEND FAILED - RETURN TO TITLE"); return false;
    }
    return true;
}
/** @brief 同期設定を確定する @param seed 乱数種 @return なし */
void SteamCoopSession::Begin(unsigned seed) {
    // ロビー設定を固定更新へ引き継ぐ
    m_difficulty = Difficulty();
    m_shots = {Shot(0), Shot(1)};
    m_seed = seed;
    m_frames.Reset();
    m_inGame = true;
    m_lastReceive = std::chrono::steady_clock::now();
    m_status = "WAITING FOR PEER";
}

/** @brief ホストが開始設定を送信する @return なし */
void SteamCoopSession::Start() {
    // ホストと開始条件を確認する
    if (!IsHost() || !CanStart()) return;

    // 乱数種とプレイ設定を相手へ送信する
    const auto seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()) | 1u;
    const std::array<std::uint32_t, 8> packet {Magic, Protocol, 1, seed,
        static_cast<unsigned>(Difficulty()), static_cast<unsigned>(Shot(0)), static_cast<unsigned>(Shot(1)), 0};
    SteamMatchmaking()->SetLobbyJoinable(m_lobby, false);
    if (Send(packet)) Begin(seed);
}

/** @brief コールバックとP2P受信を処理する @return なし */
void SteamCoopSession::Poll() {
    if (!m_initialized) return;

    // Steamのコールバックを処理する
    SteamAPI_RunCallbacks();
    const auto now = std::chrono::steady_clock::now();
    if (m_opening && now - m_openStarted > std::chrono::seconds(30)) {
        Leave(); Fail("LOBBY CONNECTION TIMED OUT");
    }
    if (!InLobby() || m_failed) return;

    // ロビーとSteam接続の状態を確認する
    if (!SteamUser()->BLoggedOn() || SteamMatchmaking()->GetLobbyOwner(m_lobby) != m_owner) {
        Fail("HOST LEFT OR STEAM DISCONNECTED - RETURN TO TITLE"); return;
    }
    const auto peer = Member(IsHost() ? 1 : 0);
    if (peer != m_peer) Debug::LogInfo(peer.IsValid() ? "Steam peer joined lobby" : "Steam peer left lobby");
    if (m_inGame && peer != m_peer) { Fail("FRIEND DISCONNECTED - RETURN TO TITLE"); return; }
    if (peer != m_peer && m_peer.IsValid()) {
        SteamNetworkingIdentity old; old.SetSteamID(m_peer);
        SteamNetworkingMessages()->CloseSessionWithUser(old);
    }
    m_peer = peer;

    // ロビーメンバーからの固定長・既知プロトコルだけ受理する
    SteamNetworkingMessage_t* messages[64] {};
    const int count = SteamNetworkingMessages()->ReceiveMessagesOnChannel(Channel, messages, 64);
    for (int i = 0; i < count; ++i) {
        auto* message = messages[i];
        std::array<std::uint32_t, 8> packet {};
        const bool trusted = message->m_identityPeer.GetSteamID() == m_peer && m_peer.IsValid();
        const bool sizeValid = message->m_cbSize == sizeof(packet);
        if (sizeValid) std::memcpy(packet.data(), message->m_pData, sizeof(packet));
        message->Release();
        if (!trusted || m_failed) continue;
        if (!sizeValid || packet[0] != Magic || packet[1] != Protocol) { Fail("INCOMPATIBLE P2P DATA"); continue; }

        // 受信メッセージを開始設定または固定更新入力として処理する
        if (packet[2] == 1 && !IsHost() && !m_inGame && packet[3] && packet[4] < 3 && packet[5] < 3 && packet[6] < 3 && !packet[7]) {
            Begin(packet[3]);
            m_difficulty = static_cast<int>(packet[4]);
            m_shots = {static_cast<int>(packet[5]), static_cast<int>(packet[6])};
        } else if (packet[2] == 2 && m_inGame && packet[4] <= CooperativeInput::ValidButtons &&
            packet[5] <= CooperativeInput::ValidButtons && !packet[6] && !packet[7] &&
            m_frames.Push(IsHost() ? 1 : 0, packet[3], {static_cast<std::uint16_t>(packet[4]), static_cast<std::uint16_t>(packet[5])})) {
            m_lastReceive = now;
        } else { Fail("INVALID P2P FRAME - RETURN TO TITLE"); }
    }
    if (m_inGame && now - m_lastReceive > std::chrono::seconds(30)) Fail("P2P TIMED OUT - RETURN TO TITLE");
}

/** @brief 同期入力を送受信する @param local 自分の入力 @param inputs 同期済み入力 @return 更新可能ならtrue */
bool SteamCoopSession::Step(CooperativeInput& local, std::array<CooperativeInput, 2>& inputs) {
    if (!m_inGame || m_failed) return false;
    const int player = IsHost() ? 0 : 1;

    // 自分の入力を相手へ送信する
    if (m_frames.CanSubmit(player)) {
        const auto frame = m_frames.Next(player);
        if (!Send({Magic, Protocol, 2, frame, local.held, local.pressed, 0, 0})) return false;
        if (!m_frames.Push(player, frame, local)) { Fail("LOCAL INPUT BUFFER ERROR"); return false; }
        local.pressed = 0;
    }

    // 両者の入力が揃ったら固定更新へ渡す
    return m_frames.Pop(inputs);
}

/** @brief 招待を保留する @param event 招待情報 @return なし */
void SteamCoopSession::OnInvite(GameLobbyJoinRequested_t* event) {
    Debug::LogInfo("Steam invite received: lobby=" + std::to_string(event->m_steamIDLobby.ConvertToUint64()));
    if (m_inGame || !event->m_steamIDLobby.IsLobby()) {
        Debug::LogWarning("Steam invite ignored: playing or invalid lobby");
        return;
    }
    // 接続中も最新の招待を保持し、完了後に画面遷移で参加する
    m_pendingLobby = event->m_steamIDLobby;
    m_lobbyRequest = true;
}
/** @brief 同じロビーの相手だけ通信を許可する @param event 接続要求 @return なし */
void SteamCoopSession::OnSessionRequest(SteamNetworkingMessagesSessionRequest_t* event) {
    if (InLobby() && !m_failed && event->m_identityRemote.GetSteamID() == Member(IsHost() ? 1 : 0))
        SteamNetworkingMessages()->AcceptSessionWithUser(event->m_identityRemote);
}
/** @brief P2P失敗を記録する @param event 失敗情報 @return なし */
void SteamCoopSession::OnSessionFailed(SteamNetworkingMessagesSessionFailed_t* event) {
    if (event->m_info.m_identityRemote.GetSteamID() == m_peer) Fail("P2P DISCONNECTED - RETURN TO TITLE");
}
/** @brief Steamオーバーレイの表示状態を保存する @param event 表示状態 @return なし */
void SteamCoopSession::OnOverlay(GameOverlayActivated_t* event) { m_overlayActive = event->m_bActive != 0; }
/** @brief 更新を停止する @param message 表示文 @return なし */
void SteamCoopSession::Fail(const char* message) { Debug::LogError(message); m_failed = true; m_status = message; }
/** @brief ロビーと通信を解放する @return なし */
void SteamCoopSession::Leave() {
    // 相手とのP2P接続を閉じる
    if (m_initialized && m_peer.IsValid()) {
        SteamNetworkingIdentity identity; identity.SetSteamID(m_peer);
        SteamNetworkingMessages()->CloseSessionWithUser(identity);
    }

    // Steamロビーを退出する
    if (m_initialized && InLobby()) SteamMatchmaking()->LeaveLobby(m_lobby);

    // ロビーと同期状態を初期化する
    m_lobby.Clear(); m_peer.Clear(); m_owner.Clear();
    // 招待はロビー画面の再生成や接続タイムアウトをまたいで保持する
    m_inGame = m_failed = m_opening = false;
}
