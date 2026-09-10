#include "../Application/UseCases/SceneManager.h"
#include "../Engine/Graphics/Renderer.h"

#include <stdexcept>

namespace {
enum class TestSceneKey { First, Second };
struct TestSharedData {};

class FirstScene final : public IScene<TestSceneKey, TestSharedData> {
public:
    /** @brief 初期化回数を記録する */
    void Initialize() override { ++initializeCount; }
    /** @brief 入力処理を受け取る */
    void ProcessInput() override {}
    /** @brief 次のシーンへの遷移を要求する */
    void Tick() override { changeScene(TestSceneKey::Second); }
    /** @brief 描画回数を記録する */
    void Render(Renderer&) override { ++renderCount; }
    /** @brief 破棄回数を記録する */
    void Dispose() override { ++disposeCount; }

    inline static int initializeCount = 0;
    inline static int disposeCount = 0;
    inline static int renderCount = 0;
};

class SecondScene final : public IScene<TestSceneKey, TestSharedData> {
public:
    /** @brief 初期化回数を記録する */
    void Initialize() override { ++initializeCount; }
    /** @brief 入力処理を受け取る */
    void ProcessInput() override {}
    /** @brief 固定更新を受け取る */
    void Tick() override {}
    /** @brief 描画回数を記録する */
    void Render(Renderer&) override { ++renderCount; }
    /** @brief 破棄回数を記録する */
    void Dispose() override { ++disposeCount; }

    inline static int initializeCount = 0;
    inline static int disposeCount = 0;
    inline static int renderCount = 0;
};

/** @brief テスト条件を検証する */
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

/** @brief シーン遷移とライフサイクルを検証する */
void RunSceneManagerTests() {
    // シーン登録と初期化を検証する
    FirstScene::initializeCount = 0;
    FirstScene::disposeCount = 0;
    FirstScene::renderCount = 0;
    SecondScene::initializeCount = 0;

    SceneManager<TestSceneKey, TestSharedData> manager;
    manager.AddScene<FirstScene>(TestSceneKey::First);
    manager.AddScene<SecondScene>(TestSceneKey::Second);
    manager.Initialize(TestSceneKey::First);
    Require(FirstScene::initializeCount == 1, "Initial scene must initialize");

    // フレーム境界での遷移適用を検証する
    manager.Tick();
    Require(FirstScene::disposeCount == 0, "Scene transition must wait for frame boundary");
    manager.CommitTransitions();
    Require(FirstScene::disposeCount == 1 && SecondScene::initializeCount == 1,
            "Pending scene transition must initialize the next scene");

    // 描画と破棄を検証する
    Renderer renderer;
    manager.Render(renderer);
    Require(SecondScene::renderCount == 1, "Renderer facade must render the active scene");
    manager.Dispose();
    Require(SecondScene::disposeCount == 1, "Scene disposal must be called");
}
