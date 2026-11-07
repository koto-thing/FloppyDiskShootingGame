#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Application/UseCases/SceneManager.h"
#include "../Presentation/Scenes/TitleScene.h"
#include "../Presentation/Scenes/TestStage.h"
#include "../Presentation/Gameplay/GameplayRandom.h"
#include "../Engine/Time/Time.h"

/** @brief デモの入力境界を実機なしで検証する */
class InputTestAccess {
public:
    /** @brief キー状態を設定する @param down 押下状態 @return なし */
    static void Key(bool down) { Input::SetNativeKeyState('Z', down); }
    /** @brief ポインター移動を設定する @return なし */
    static void Move() { Input::AddMouseDelta(1.0f, 0.0f); }
    /** @brief 2P操作を設定する @param down 押下状態 @return なし */
    static void Pad(bool down) { Input::SetGamepadKeyState(1, KeyCode::Z, down); }
};

/** @brief 待機時間、中断、入力持越し、共有設定保持を検証する @return なし */
void CheckAttractScenes() {
    SceneManager<SceneType, SceneSharedData> scenes;
    scenes.AddScene<TitleScene>(SceneType::Title);
    scenes.AddScene<TestStage>(SceneType::TestStage);
    auto& data = scenes.getSharedData();
    data.difficulty = Hard;
    data.playerType = Spread;
    data.playerCount = 2;
    data.startStage = 3;
    scenes.Initialize(SceneType::Title);
    Input::BeginFrame();
    scenes.ProcessInput();
    Time::unscaledDeltaTime = 29.0f;
    scenes.ProcessInput();
    assert(!data.startDemo);
    InputTestAccess::Move();
    assert(Input::HasActivity() && !Input::HasActivity(false));
    scenes.ProcessInput();
    Input::BeginFrame();
    scenes.ProcessInput();
    assert(!data.startDemo);
    Time::unscaledDeltaTime = 1.0f;
    scenes.ProcessInput();
    assert(data.startDemo);
    const unsigned randomBefore = GameplayRandom::State;
    scenes.Tick();
    scenes.CommitTransitions();
    assert(!data.startDemo && data.startStage == 3 && data.playerCount == 2);

    // 2Pでも終了でき、押し続けた操作はタイトルを再始動させない
    InputTestAccess::Pad(true);
    assert(Input::HasActivity(false));
    scenes.ProcessInput();
    scenes.Tick();
    scenes.CommitTransitions();
    assert(GameplayRandom::State == randomBefore);
    Time::unscaledDeltaTime = 30.0f;
    scenes.ProcessInput();
    assert(!data.startDemo);
    InputTestAccess::Pad(false);
    scenes.ProcessInput();
    Input::BeginFrame();
    scenes.ProcessInput();
    scenes.ProcessInput();
    assert(data.startDemo);
    scenes.Tick();
    scenes.CommitTransitions();

    // 無操作デモも上限時間までにタイトルへ復帰し、通常設定を変えない
    for (int frame = 0; frame < 3601; ++frame) {
        scenes.Tick();
        scenes.CommitTransitions();
    }
    assert(GameplayRandom::State == randomBefore);
    assert(data.difficulty == Hard && data.playerType == Spread && data.startStage == 3);
    scenes.ProcessInput();
    scenes.ProcessInput();
    assert(data.startDemo);
    scenes.Dispose();
    InputTestAccess::Key(true);
    assert(Input::HasActivity(false));
    InputTestAccess::Key(false);
    Input::BeginFrame();
    assert(!Input::HasActivity());
}

#include "../Infrastructure/Repositories/SettingsRepository.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Common/StageDefinition.h"

/** @brief ステージ選択からの開始と進行状況の解放をGPUなしで検証する */
struct StageSelectTests {
    /** @brief 全ステージの開始地点、リスタート、解放条件を検証する @return なし */
    static void Run() {
        auto game = std::make_unique<SideScrollingShooter>();
        auto& g = *game;

        // デモは保存済み進行を変えず、撃破時に終了する
        const auto beforeDemo = SettingsRepository {}.Load();
        GameplayRandom::State = 0x9e3779b9U;
        g.InitializeDemo(nullptr);
        assert(g.m_demoMode && g.m_difficulty == Easy && g.m_playerCount == 1);
        for (int frame = 0; frame < 3600; ++frame) {
            assert(!g.TickDemo(frame));
            if (frame == 600) assert(g.Player().m_bomb.active);
        }
        g.m_galleryUnlocks = 0;
        g.UnlockGallery(GalleryEntry::LightEnemy);
        assert(g.m_galleryUnlocks == 0);
        g.m_clear = true;
        g.m_clearTimer = 100;
        g.m_missionStartTimer = 0;
        g.m_unlockedStage = 1;
        g.Tick();
        assert(g.m_unlockedStage == 1);
        const auto afterDemo = SettingsRepository {}.Load();
        assert(beforeDemo.galleryUnlocks == afterDemo.galleryUnlocks);
        assert(beforeDemo.unlockedStage == afterDemo.unlockedStage);
        g.Initialize(nullptr, Spread, Hard);
        assert(!g.m_demoMode && g.m_difficulty == Hard);

        // 全難易度・機体で各ステージの先頭から正しい状態で開始できる
        for (int stage = 1; stage <= 5; ++stage) {
            const float power = static_cast<float>(stage - 1);
            for (int difficulty = 0; difficulty < 3; ++difficulty) {
                for (int player = 0; player < 3; ++player) {
                    g.m_difficulty = static_cast<DifficultyType>(difficulty);
                    g.Reset(true);
                    g.Player().m_playerType = static_cast<PlayerType>(player);
                    g.StartFromStage(stage);
                    assert(g.m_stageNumber == stage && g.m_chapterNumber == 1);
                    assert(g.m_frame == g.m_stage->ChapterEndFrame(0));
                    assert(std::abs(g.Player().m_power - power) < 0.0001f);
                    assert(g.Player().m_power <= SideScrollingShooter::MaxPower);
                    assert(g.m_score == 0 && g.m_kills == 0);
                    assert(g.Player().m_bombCount == SideScrollingShooter::InitialBombCount);
                    assert(std::abs(g.m_stage5.checkpointPower - power) < 0.0001f);
                    g.Tick();
                    assert(g.m_stageNumber == stage);

                    // 被弾後も選択したステージと開始時スコアへ戻り、パワーは通常の減少だけを受ける
                    g.m_score = 1000;
                    g.RestartCurrentChapter();
                    assert(g.m_stageNumber == stage && g.m_score == 0);
                    assert(g.Player().m_power <= power + 0.0001f);
                }
            }

            // ボス戦でボムを使い切っても、繰り返しのリスタートで初期数へ戻る
            g.StartDebugCheckpoint(stage, 3, true);
            for (int remaining : {0, 1, 2}) {
                g.Player().m_bombCount = remaining;
                g.RestartCurrentChapter();
                assert(g.Player().m_bombCount == SideScrollingShooter::InitialBombCount);
            }
        }

        // ボス撃破で次だけを解放し、チュートリアルと最終ステージでは解放しない
        const auto tickCleared = [&](int stage, int unlocked, bool tutorial) {
            g.Reset(true);
            g.StartDebugCheckpoint(stage, 1, false);
            g.m_unlockedStage = unlocked;
            g.m_tutorialMode = tutorial;
            g.m_clear = true;
            g.m_clearTimer = 100;
            g.m_missionStartTimer = 0;
            g.Tick();
            g.m_tutorialMode = false;
            return g.m_unlockedStage;
        };
        assert(tickCleared(1, 1, false) == 2);
        assert(SettingsRepository {}.Load().unlockedStage >= 2);
        assert(tickCleared(4, 4, false) == 5);
        assert(tickCleared(1, 3, false) == 3);
        assert(tickCleared(5, 5, false) == 5);
        assert(tickCleared(2, 2, true) == 2);
    }
};

/** @brief ステージ選択の回帰チェックを実行する @return 成功時0 */
int main() {
    CheckAttractScenes();
    StageSelectTests::Run();
    std::puts("Stage select tests passed");
    return 0;
}
