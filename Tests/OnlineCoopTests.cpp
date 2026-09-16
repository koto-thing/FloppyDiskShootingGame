#include "../Infrastructure/ExternalServices/OnlineCoopSession.h"
#include "../Infrastructure/Repositories/SettingsRepository.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>
#include <windows.h>

/** @brief 実HTTP通信を進めて条件成立を待つ @param a クライアント1 @param b クライアント2 @param done 成立条件 @return なし */
void Wait(OnlineCoopSession& a, OnlineCoopSession& b, const std::function<bool()>& done) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    do {
        a.Poll(); b.Poll();
        if (done()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);
    std::fprintf(stderr, "Timeout: %s / %s\n", a.Status(), b.Status());
    assert(false);
}

/** @brief 実サーバー経由で協力通信と設定保存を検証する @return 成功時0 */
int main() {
    // テスト専用の保存先を準備する
    wchar_t localPath[2048] {};
    assert(GetEnvironmentVariableW(L"LOCALAPPDATA", localPath, 2048));
    const auto directory = std::filesystem::path(localPath) / L"SpaceYakuza";
    assert(directory.wstring().find(L"OnlineTests") != std::wstring::npos);
    std::filesystem::create_directories(directory);
    std::ofstream(directory / "settings.dat") << "1\n1\n0.5\n1\n1\n0\n";
    SettingsRepository settings;
    assert(settings.Load().automaticMatchmaking);
    auto saved = settings.Load();
    saved.automaticMatchmaking = false;
    settings.Save(saved);
    assert(!settings.Load().automaticMatchmaking);
    assert(settings.Load().masterVolume == saved.masterVolume);

    // 自動検索で同じ部屋へ接続する
    OnlineCoopSession a, b;
    a.OpenLobby(true);
    b.OpenLobby(true);
    Wait(a, b, [&] { return a.InLobby() && b.InLobby() && a.RoomCode() == b.RoomCode(); });
    assert(a.IsHost() != b.IsHost());
    auto& host = a.IsHost() ? a : b;
    auto& guest = a.IsHost() ? b : a;
    host.CycleDifficulty(); guest.CycleShot();
    Wait(a, b, [&] { return a.Difficulty() == 1 && b.Difficulty() == 1 && a.Shot(1) == 1 && b.Shot(1) == 1; });
    host.ToggleReady(); guest.ToggleReady();
    Wait(a, b, [&] { return a.CanStart() && b.CanStart(); });
    host.Start();
    Wait(a, b, [&] { return a.InGame() && b.InGame(); });
    assert(a.Seed() == b.Seed());

    // 両側で受け取った固定更新の順序とビットを検証する
    using Inputs = std::array<CooperativeInput, 2>;
    std::vector<Inputs> first, second;
    Wait(a, b, [&] {
        assert(!a.Failed() && !b.Failed());
        Inputs inputs;
        CooperativeInput left {CooperativeInput::Left, CooperativeInput::Bomb};
        CooperativeInput right {CooperativeInput::Fire, CooperativeInput::Confirm};
        if (first.size() < 360 && a.Step(left, inputs)) first.push_back(inputs);
        if (second.size() < 360 && b.Step(right, inputs)) second.push_back(inputs);
        return first.size() == 360 && second.size() == 360;
    });
    for (size_t frame = 0; frame < first.size(); ++frame) for (int player = 0; player < 2; ++player) {
        assert(first[frame][player].held == second[frame][player].held);
        assert(first[frame][player].pressed == second[frame][player].pressed);
        if (frame >= CooperativeFrames::Delay)
            assert(first[frame][player].held == (player == (a.IsHost() ? 0 : 1) ? CooperativeInput::Left : CooperativeInput::Fire));
    }
    a.Leave();
    Wait(a, b, [&] { return b.Failed(); });
    b.Leave();

    // プライベート部屋へコード指定で接続する
    a.OpenLobby(false);
    Wait(a, b, [&] { return a.InLobby(); });
    b.OpenLobby(true);
    Wait(a, b, [&] { return b.InLobby(); });
    assert(a.RoomCode() != b.RoomCode());
    b.OpenLobby(false, a.RoomCode());
    Wait(a, b, [&] { return b.InLobby() && b.RoomCode() == a.RoomCode(); });
    assert(a.IsHost() && !b.IsHost());

    // 協力用と通常用のランキングを分けて取得する
    a.SubmitScore(2, 12345, true);
    Wait(a, b, [&] { return std::string(a.ScoreStatus()) == "SCORE UPLOADED"; });
    a.FetchRankings(true);
    a.FetchRankings(false);
    Wait(a, b, [&] { return std::string(a.RankingStatus(true)).find("GLOBAL RANKING -") == 0 &&
        std::string(a.RankingStatus(false)).find("GLOBAL RANKING -") == 0; });
    assert(a.Rankings(true)[2][0] == 12345);
    assert(a.Rankings(false)[2][0] == 0);
    a.Leave(); b.Leave();

    // 接続開始直後のキャンセル結果が次の部屋へ混入しない
    a.OpenLobby(true); a.Leave(); a.OpenLobby(false);
    Wait(a, b, [&] { return a.InLobby(); });
    assert(!a.Failed() && !a.Ready(0) && a.Difficulty() == 0);
    a.Leave();
    std::puts("Online HTTP, matchmaking, private room, input, ranking and settings tests passed");
}
