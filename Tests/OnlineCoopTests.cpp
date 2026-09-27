#include "../Infrastructure/ExternalServices/OnlineCoopSession.h"
#include "../Infrastructure/Repositories/SettingsRepository.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>
#include <windows.h>

/** @brief 実通信を進めて条件成立を待つ @param a クライアント1 @param b クライアント2 @param done 成立条件 @param seconds 制限秒数 @return なし */
void Wait(OnlineCoopSession& a, OnlineCoopSession& b, const std::function<bool()>& done, unsigned seconds = 15) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    do {
        a.Poll(); b.Poll();
        if (done()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);
    std::fprintf(stderr, "Timeout: %s / %s\n", a.Status(), b.Status());
    assert(false);
}

/** @brief 実サーバー経由で協力通信と設定保存を検証する @param argc 引数数 @param argv 遅延テスト指定 @return 成功時0 */
int main(int argc, char** argv) {
    // 本番経路の計測では専用部屋だけを使用し、ランキングへ書き込まない
    const bool probe = argc > 1 && std::string(argv[1]) == "--probe";
    const bool bursts = argc > 2 && std::string(argv[1]) == "--bursts";
    const bool jitter = bursts || (argc > 2 && std::string(argv[1]) == "--jitter");
    const auto jitterMarker = jitter ? std::string(argv[2]) + (bursts ? ".bursts" : ".jitter") : "";
    // 共有バッファの既定値とOnline用の可変遅延を検証する
    for (const unsigned delay : {6u, 15u, 30u}) {
        CooperativeFrames frames;
        frames.Reset(delay);
        assert(frames.Next(0) == delay && frames.Next(1) == delay);
        for (unsigned frame = 0; frame < 600; ++frame) {
            assert(frames.CanSubmit(0) && frames.CanSubmit(1));
            assert(frames.Push(0, frame + delay, {CooperativeInput::Left, CooperativeInput::Bomb}));
            assert(frames.Push(1, frame + delay, {CooperativeInput::Fire, 0}));
            std::array<CooperativeInput, 2> inputs;
            assert(frames.Pop(inputs));
            assert(inputs[0].held == (frame < delay ? 0 : CooperativeInput::Left));
            assert(inputs[1].held == (frame < delay ? 0 : CooperativeInput::Fire));
        }
        frames.Reset();
        assert(frames.Next(0) == CooperativeFrames::Delay);
        // 先行量を増やしても入力番号は連続し、縮小時は既存入力を捨てず送信を待つ
        for (unsigned frame = 6; frame <= 12; ++frame) {
            assert(frames.CanSubmit(0, 12));
            assert(frames.Push(0, frame, {CooperativeInput::Left, CooperativeInput::Bomb}));
        }
        assert(!frames.CanSubmit(0) && !frames.CanSubmit(0, 12));
        assert(frames.CanSubmit(0, 13));
        assert(!frames.CanSubmit(0, UINT32_MAX));
    }
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
    a.OpenLobby(!probe);
    if (probe) Wait(a, b, [&] { return a.InLobby(); });
    b.OpenLobby(!probe, probe ? a.RoomCode() : "");
    Wait(a, b, [&] { return a.InLobby() && b.InLobby() && a.RoomCode() == b.RoomCode(); });
    assert(a.IsHost() != b.IsHost());
    auto& host = a.IsHost() ? a : b;
    auto& guest = a.IsHost() ? b : a;
    // 送信側で次のPollを呼ばなくても相手へ操作が届く
    host.CycleDifficulty();
    const auto deliveryDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (guest.Difficulty() != 1 && std::chrono::steady_clock::now() < deliveryDeadline) {
        guest.Poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(guest.Difficulty() == 1);
    guest.CycleShot();
    Wait(a, b, [&] { return a.Difficulty() == 1 && b.Difficulty() == 1 && a.Shot(1) == 1 && b.Shot(1) == 1; });
    // 描画相当の処理を600ms止めても、待ち時間がネットワークRTTへ混入しない
    for (int sample = 0; sample < 3; ++sample) {
        a.Poll(); b.Poll();
        if (probe) {
            const auto sampleUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
            Wait(a, b, [&] { return std::chrono::steady_clock::now() >= sampleUntil; });
        } else std::this_thread::sleep_for(std::chrono::milliseconds(600));
        a.Poll(); b.Poll();
        if (!probe) assert(a.RoundTripMs() < 250 && b.RoundTripMs() < 250);
    }
    std::printf("%s: network RTT=%u/%u ms\n", probe ? "Lobby probe" : "600ms main-thread pause", a.RoundTripMs(), b.RoundTripMs());
    host.ToggleReady(); guest.ToggleReady();
    Wait(a, b, [&] { return a.CanStart() && b.CanStart(); });
    host.Start();
    Wait(a, b, [&] { return a.InGame() && b.InGame(); });
    assert(a.Seed() == b.Seed());
    assert(a.InputDelay() == b.InputDelay());
    if (jitter) std::ofstream(jitterMarker) << "periodic input delay\n";
    if (argc > 1 && std::string(argv[1]) == "--delayed") {
        assert(a.InputDelay() >= 9 && a.RoundTripMs() >= 80 && b.RoundTripMs() >= 80);
    }

    // 両側で受け取った固定更新の順序とビットを検証する
    using Inputs = std::array<CooperativeInput, 2>;
    std::vector<Inputs> first, second;
    auto nextStep = std::chrono::steady_clock::now();
    unsigned waits = 0;
    const unsigned targetFrames = probe ? 3600 : bursts ? 2160 : jitter ? 1440 : 360;
    const unsigned recoveryFrame = bursts ? 1200 : 600;
    unsigned recurringWaits = 0;
    unsigned peakAhead = a.InputDelay();
    bool recovered = false;
    bool rankingRequested = false;
    Wait(a, b, [&] {
        assert(!a.Failed() && !b.Failed());
        // 実ゲームと同じ60Hzで進め、応答待ちによる停止を数える
        const auto now = std::chrono::steady_clock::now();
        if (now < nextStep) return false;
        nextStep += std::chrono::microseconds(16667);
        Inputs inputs;
        CooperativeInput left {CooperativeInput::Left, CooperativeInput::Bomb};
        CooperativeInput right {CooperativeInput::Fire, CooperativeInput::Confirm};
        const auto previousWaits = waits;
        if (first.size() < targetFrames) { if (a.Step(left, inputs)) first.push_back(inputs); else ++waits; }
        if (second.size() < targetFrames) { if (b.Step(right, inputs)) second.push_back(inputs); else ++waits; }
        if (bursts && first.size() >= 360 && second.size() >= 360 && !recovered) recurringWaits += waits - previousWaits;
        peakAhead = (std::max)({peakAhead, a.InputLookahead(), b.InputLookahead()});
        if (jitter && !recovered && first.size() >= recoveryFrame && second.size() >= recoveryFrame) {
            std::filesystem::remove(jitterMarker);
            recovered = true;
        }
        if (!probe && !rankingRequested && first.size() >= 60) { a.FetchRankings(false); rankingRequested = true; }
        return first.size() == targetFrames && second.size() == targetFrames;
    }, probe ? 180 : jitter ? 60 : 15);
    std::printf("60Hz: delay=%u RTT=%u/%u ms, stalled ticks=%u/%u\n", a.InputDelay(), a.RoundTripMs(), b.RoundTripMs(), waits, targetFrames * 2);
    std::printf("Input lookahead: peak=%u final=%u/%u frames\n", peakAhead, a.InputLookahead(), b.InputLookahead());
    if (bursts) std::printf("Recurring stalled ticks after warmup: %u\n", recurringWaits);
    std::fflush(stdout);
    if (!probe) assert(waits <= (jitter ? 60u : 12u));
    if (bursts) assert(recurringWaits <= 4);
    if (jitter) {
        assert(peakAhead > a.InputDelay() && peakAhead <= (std::min)(30u, a.InputDelay() + 6));
        assert(a.InputLookahead() == a.InputDelay() && b.InputLookahead() == b.InputDelay());
    }
    for (size_t frame = 0; frame < first.size(); ++frame) for (int player = 0; player < 2; ++player) {
        assert(first[frame][player].held == second[frame][player].held);
        assert(first[frame][player].pressed == second[frame][player].pressed);
        if (frame >= a.InputDelay())
            assert(first[frame][player].held == (player == (a.IsHost() ? 0 : 1) ? CooperativeInput::Left : CooperativeInput::Fire));
    }
    if (!probe && !jitter) {
        // エンディング相当の入力送信終了後も、接続確認だけで正常な状態を保つ
        const auto endingUntil = std::chrono::steady_clock::now() + std::chrono::seconds(31);
        Wait(a, b, [&] {
            assert(!a.Failed() && !b.Failed());
            return std::chrono::steady_clock::now() >= endingUntil;
        }, 35);
    }
    a.Leave();
    Wait(a, b, [&] { return b.Failed(); });
    // 入力受信から30秒を超えていても、退出通知をタイムアウトへ書き換えない
    assert(std::string(b.Status()) == "PARTNER DISCONNECTED - RETURN TO TITLE");
    b.Poll();
    assert(std::string(b.Status()) == "PARTNER DISCONNECTED - RETURN TO TITLE");
    b.Leave();
    if (probe || jitter) return 0;

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
    if (argc > 2) {
        // TCPが開いたまま無応答になってもタイムアウトし、安全に退出できる
        std::ofstream(argv[2]) << "drop traffic\n";
        Wait(a, b, [&] { return a.Failed(); });
        const auto before = std::chrono::steady_clock::now();
        a.Leave();
        assert(std::chrono::steady_clock::now() - before < std::chrono::seconds(2));
        std::puts("Silent connection timed out; pending receive cancelled within 2 seconds");
    }
    a.Leave();
    std::puts("Online WebSocket, matchmaking, private room, input, ranking and settings tests passed");
}
