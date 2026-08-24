#include "../Application/UseCases/SceneManager.h"
#include "../Engine/Graphics/Renderer.h"

#include <stdexcept>

namespace {
enum class TestSceneKey { First, Second };
struct TestSharedData {};

class FirstScene final : public IScene<TestSceneKey, TestSharedData> {
public:
    void Initialize() override { ++initializeCount; }
    void ProcessInput() override {}
    void Tick() override { changeScene(TestSceneKey::Second); }
    void Render(Renderer&) override { ++renderCount; }
    void Dispose() override { ++disposeCount; }

    inline static int initializeCount = 0;
    inline static int disposeCount = 0;
    inline static int renderCount = 0;
};

class SecondScene final : public IScene<TestSceneKey, TestSharedData> {
public:
    void Initialize() override { ++initializeCount; }
    void ProcessInput() override {}
    void Tick() override {}
    void Render(Renderer&) override { ++renderCount; }
    void Dispose() override { ++disposeCount; }

    inline static int initializeCount = 0;
    inline static int disposeCount = 0;
    inline static int renderCount = 0;
};

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

void RunSceneManagerTests() {
    FirstScene::initializeCount = 0;
    FirstScene::disposeCount = 0;
    FirstScene::renderCount = 0;
    SecondScene::initializeCount = 0;

    SceneManager<TestSceneKey, TestSharedData> manager;
    manager.AddScene<FirstScene>(TestSceneKey::First);
    manager.AddScene<SecondScene>(TestSceneKey::Second);
    manager.Initialize(TestSceneKey::First);
    Require(FirstScene::initializeCount == 1, "Initial scene must initialize");

    manager.Tick();
    Require(FirstScene::disposeCount == 0, "Scene transition must wait for frame boundary");
    manager.CommitTransitions();
    Require(FirstScene::disposeCount == 1 && SecondScene::initializeCount == 1,
            "Pending scene transition must initialize the next scene");

    Renderer renderer;
    manager.Render(renderer);
    Require(SecondScene::renderCount == 1, "Renderer facade must render the active scene");
    manager.Dispose();
    Require(SecondScene::disposeCount == 1, "Scene disposal must be called");
}
