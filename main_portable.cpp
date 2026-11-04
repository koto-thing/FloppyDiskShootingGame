#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <memory>
#include <stdexcept>
#include <string_view>
#include "Engine/Diagnostics/Debug.h"
#include "Engine/Input/Input.h"
#include "Engine/Time/Time.h"
#include "Engine/UI/Button.h"
#include "Application/UseCases/SceneManager.h"
#include "Domain/ValueObjects/SceneSharedData.h"
#include "Domain/ValueObjects/SceneType.h"
#include "Infrastructure/ExternalServices/PortableRenderingService.h"
#include "Infrastructure/ExternalServices/AudioService.h"
#include "Infrastructure/Repositories/SettingsRepository.h"
#include "Engine/Graphics/Renderer.h"
#include "Presentation/Scenes/OptionScene.h"
#include "Presentation/Scenes/TitleScene.h"
#include "Presentation/Scenes/TestStage.h"
#include "Presentation/Scenes/TutorialStage.h"
#include "Presentation/Scenes/GalleryScene.h"
#include "Presentation/Scenes/CreditScene.h"
#include "Presentation/Scenes/ModeSelectionScene.h"
#include "Presentation/Scenes/StoryScene.h"
#include "Presentation/Scenes/EndingScene.h"
#include "Presentation/Scenes/RankingScene.h"

namespace {
/** @brief SDLと共通ゲームのライフサイクルを所有する */
struct Game {
    SDL_Window* window = nullptr;
    PortableRenderingService backend;
    Renderer renderer {backend};
    AudioService audio;
    SceneManager<SceneType, SceneSharedData> app;
    bool running = true;
    int smokeFrames = 0;

    /** @brief 初期化途中の失敗を含め所有資源を解放する */
    ~Game() {
        // コールバックから破棄済みの音声へアクセスさせない
        Button::SetClickSoundHandler({});
        app.Dispose();
        audio.Shutdown();
        backend.Cleanup();
        Input::Shutdown();
        if (window) SDL_DestroyWindow(window);
        Debug::Shutdown();
        SDL_Quit();
    }

    /** @brief プラットフォームとFloppy相当の全シーンを初期化する @return なし */
    void Initialize() {
        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
            throw std::runtime_error(SDL_GetError());
        Debug::Initialize();
#if defined(__EMSCRIPTEN__)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        constexpr Uint32 graphicsFlag = SDL_WINDOW_OPENGL;
#else
        constexpr Uint32 graphicsFlag = SDL_WINDOW_VULKAN;
#endif
        window = SDL_CreateWindow("Space Yakuza", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 720, graphicsFlag | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window || !Input::Initialize(window) || !backend.Initialize(window, 1280, 720))
            throw std::runtime_error("Window, input or renderer initialization failed");
        const GameSettings settings = SettingsRepository().Load();
        backend.SetRetroEffectEnabled(settings.retroEffectEnabled);
        audio.SetMasterVolume(settings.masterVolume);
        audio.SetBGMVolume(settings.bgmVolume);
        audio.SetSEVolume(settings.seVolume);
        if (!audio.Initialize()) throw std::runtime_error(SDL_GetError());
        Button::SetClickSoundHandler([this](Button::ClickSound sound) {
            Audio::SfxrParams params = Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::BlipSelect);
            params.startFrequency = sound == Button::ClickSound::Confirm ? 0.65f : 0.18f;
            audio.PlaySE(params);
        });
        app.getSharedData().audio = &audio;
    app.AddScene<TitleScene>(SceneType::Title);
    app.AddScene<ModeSelectionScene>(SceneType::ModeSelection);
    app.AddScene<StoryScene>(SceneType::Story);
    app.AddScene<TestStage>(SceneType::TestStage);
    app.AddScene<TutorialStage>(SceneType::TutorialStage);
    app.AddScene<GalleryScene>(SceneType::Gallery);
    app.AddScene<OptionScene>(SceneType::Option);
    app.AddScene<CreditScene>(SceneType::Credit);
    app.AddScene<EndingScene>(SceneType::Ending);
    app.AddScene<RankingScene>(SceneType::Ranking);

    // 初期シーンの設定
    app.Initialize(SceneType::Title);


        Time::Initialize();
    }

    /** @brief ブラウザとネイティブで共通の1フレームを実行する @return なし */
    void Frame() {
        // イベント処理をブラウザのフレーム間に戻せる形にする
        Input::BeginFrame();
        SDL_Event event {};
        while (SDL_PollEvent(&event)) {
            Input::ProcessEvent(event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                backend.Resize(event.window.data1, event.window.data2);
        }
        if (!running) return;
        Input::PollGamepad();
        Time::BeginFrame();
        app.ProcessInput();
#if defined(_DEBUG)
        if (Input::GetKeyDown(KeyCode::F7)) app.RequestTransition(SceneType::TutorialStage);
#endif
        // 既存Windows版と同じ固定更新上限を使う
        int steps = 0;
        while (Time::HasFixedStep() && steps < 8) {
            Time::ConsumeFixedStep();
            app.Tick();
            ++steps;
        }
        if (steps == 8 && Time::HasFixedStep()) Time::DiscardExcessFixedTime();
        app.CommitTransitions();
        audio.Update();
        renderer.BeginFrame();
        app.Render(renderer);
        renderer.EndFrame();
        if (smokeFrames > 0 && --smokeFrames == 0) running = false;
    }
};
std::unique_ptr<Game> game;
#if defined(__EMSCRIPTEN__)
/** @brief ブラウザへ制御を返しながらゲームを更新する @return なし */
void BrowserFrame() {
    try {
        game->Frame();
    } catch (const std::exception& error) {
        Debug::LogError(error.what());
        game->running = false;
    }
    if (!game->running) {
        emscripten_cancel_main_loop();
        game.reset();
        EM_ASM({ Module.setStatus('Game ended. Reload to play again.'); });
    }
}
#endif
}

/** @brief 各配布版のSDLエントリーポイント @param argc 引数数 @param argv 引数列 @return 成功時0、初期化失敗時1 */
int main(int argc, char** argv) {
    // 大型ステージと描画コマンドをスタックへ配置しない
    try {
        game = std::make_unique<Game>();
        game->Initialize();
        if (argc > 1 && std::string_view(argv[1]) == "--smoke-test") game->smokeFrames = 120;
#if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(BrowserFrame, 0, false);
#else
        while (game->running) game->Frame();
        game.reset();
#endif
        return 0;
    } catch (const std::exception& error) {
        Debug::LogError(error.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Space Yakuza", error.what(), nullptr);
        game.reset();
        return 1;
    }
}
