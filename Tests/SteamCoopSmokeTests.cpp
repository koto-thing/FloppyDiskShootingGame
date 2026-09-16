#include "../Infrastructure/ExternalServices/SteamCoopSession.h"
#include <cassert>
#include <cstdio>
#include <thread>

/** @brief 実際のSteam APIで単独ロビーの作成・選択・退出を検証する @return 成功0、Steam未起動2、接続失敗1 */
int main() {
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
