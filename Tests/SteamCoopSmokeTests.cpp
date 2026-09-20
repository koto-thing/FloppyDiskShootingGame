#include "../Infrastructure/ExternalServices/SteamCoopSession.h"
#include <cassert>
#include <cstdio>
#include <thread>
#include <cstring>
#include "../Engine/Diagnostics/Debug.h"

/** @brief Steam接続なしで招待保留と画面遷移前後を検証する */
struct SteamCoopSessionTests {
    /** @brief 本番セッションの招待処理を検証する @return なし */
    static void Run() {
        SteamCoopSession session;
        GameLobbyJoinRequested_t invite {};
        invite.m_steamIDLobby = CSteamID(42, k_EChatInstanceFlagLobby, k_EUniversePublic, k_EAccountTypeChat);
        assert(invite.m_steamIDLobby.IsLobby());
        assert(!session.HasPlayer(0) && !session.HasPlayer(1));

        // 接続処理中に承認した招待は消費せず、失敗結果の後でも参加できる
        session.m_opening = true;
        session.OnInvite(&invite);
        assert(!session.TakeLobbyRequest());
        LobbyCreated_t failed {};
        failed.m_eResult = k_EResultFail;
        session.OnCreated(&failed, false);
        assert(session.TakeLobbyRequest());
        assert(!session.TakeLobbyRequest());
        session.Leave();
        assert(session.m_pendingLobby == invite.m_steamIDLobby);

        // タイムアウト時の退出でも、未消費の招待を保持する
        session.m_opening = true;
        session.OnInvite(&invite);
        session.Leave();
        assert(session.TakeLobbyRequest());

        // 参加要求の通信失敗でも、次の招待を処理できる
        session.m_opening = true;
        session.OnInvite(&invite);
        assert(!session.TakeLobbyRequest());
        session.OnEntered(nullptr, true);
        assert(session.TakeLobbyRequest());

        // メニューでの招待は即時受理し、無効な招待やプレイ中の招待は拒否する
        session.OnInvite(&invite);
        assert(session.TakeLobbyRequest());
        session.m_pendingLobby.Clear();
        session.m_inGame = true;
        session.OnInvite(&invite);
        assert(!session.TakeLobbyRequest() && !session.m_pendingLobby.IsValid());
        session.m_inGame = false;
        invite.m_steamIDLobby.Clear();
        session.OnInvite(&invite);
        assert(!session.TakeLobbyRequest());
        std::puts("Steam invitation regression tests passed (offline)");
    }
};

/** @brief 招待処理と単独ロビーを検証する @param argc 引数数 @param argv --offlineで実通信を省略 @return 成功0、Steam未起動2、接続失敗1 */
int main(int argc, char** argv) {
    // オフライン検証はSteamへのログインなしで実行する
    Debug::Initialize();
    SteamCoopSessionTests::Run();
    if (argc > 1 && std::strcmp(argv[1], "--offline") == 0) return 0;
    // Steam APIを初期化する
    SteamCoopSession session;
    session.Initialize(L"");
    if (!session.Available()) { std::puts(session.Status()); return 2; }

    // ロビー作成の完了を待つ
    session.OpenLobby();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!session.InLobby() && !session.Failed() && std::chrono::steady_clock::now() < deadline) {
        session.Poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!session.InLobby() || session.Failed()) { std::puts(session.Status()); return 1; }

    // ロビー設定の変更と開始条件を検証する
    assert(session.IsHost() && !session.CanStart() && !session.InGame());
    assert(session.Difficulty() == 0 && session.Shot(0) == 0);
    session.CycleDifficulty();
    session.CycleShot();
    session.ToggleReady();
    assert(session.Difficulty() == 1 && session.Shot(0) == 1 && session.Ready(0));
    session.Start();
    assert(!session.InGame());

    // ロビーを退出する
    session.Leave();
    assert(!session.InLobby() && !session.InGame());
    std::puts("Steam lobby smoke tests passed (no invitations sent)");
    return 0;
}
