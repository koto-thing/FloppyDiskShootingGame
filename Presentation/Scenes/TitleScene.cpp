#include "TitleScene.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <string>
#include "../../Engine/Graphics/Renderer.h"
#include "../../Infrastructure/ExternalServices/InputService.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"

#ifdef DrawText
#undef DrawText
#endif

/**
 * @brief タイトルシーンの初期化処理
 */
void TitleScene::Initialize() {
}

/**
 * @brief タイトルシーンの入力処理
 */
void TitleScene::ProcessInput() {
    // 画面左下の音量バーをマウス左クリック/ドラッグ操作
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) {
        POINT pt;
        GetCursorPos(&pt);
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            ScreenToClient(hwnd, &pt);
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            if (width > 0 && height > 0) {
                // NDC座標への変換 (-1.0 ~ 1.0)
                float mouseX = (static_cast<float>(pt.x) / width) * 2.0f - 1.0f;
                float mouseY = 1.0f - (static_cast<float>(pt.y) / height) * 2.0f;

                // 左下の音量バー位置判定 (X: -0.75f ~ -0.35f)
                constexpr float barLeft = -0.75f;
                constexpr float barRight = -0.35f;

                if (mouseX >= barLeft - 0.05f && mouseX <= barRight + 0.05f) {
                    float newVol = std::clamp((mouseX - barLeft) / (barRight - barLeft), 0.0f, 1.0f);

                    // Master バー (Y: -0.65f 付近)
                    if (mouseY >= -0.70f && mouseY <= -0.60f) {
                        AudioService::Get().SetMasterVolume(newVol);
                    }
                    // BGM バー (Y: -0.75f 付近)
                    else if (mouseY >= -0.80f && mouseY <= -0.70f) {
                        AudioService::Get().SetBGMVolume(newVol);
                    }
                    // SE バー (Y: -0.85f 付近)
                    else if (mouseY >= -0.90f && mouseY <= -0.80f) {
                        AudioService::Get().SetSEVolume(newVol);
                    }
                }
            }
        }
    }
}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
    // Enterキーが押されたら、TestStageに移行する
    if (InputService::IsKeyPressed(VK_RETURN)) {
        AudioService::Get().PlaySE(Audio::SfxrPreset::BlipSelect);
        changeScene(SceneType::TestStage);
    }
}

void TitleScene::Shutdown() {
}

/**
 * @brief タイトルシーンの描画処理
 */
void TitleScene::Render(Renderer& renderer) {
    // 画面上部に "TITLE" と表示
    renderer.DrawText(
        "FLOPPY DISK SHOOTING GAME",
        { -0.7f, 0.6f },
        0.04f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );
    
    // 画面下部に "PRESS ENTER TO START" と表示
    renderer.DrawText(
        "PRESS ENTER TO START",
        { -0.3f, 0.0f},
        0.02f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );

    // 画面左下の端っこに音量バーを配置
    auto makeBarStr = [](const char* label, float volume) {
        int filled = static_cast<int>(volume * 12.0f + 0.5f);
        filled = std::clamp(filled, 0, 12);
        std::string bar = std::string(label) + " [";
        for (int i = 0; i < 12; ++i) {
            bar += (i < filled) ? "=" : "-";
        }
        bar += "] ";
        char buf[16];
        snprintf(buf, sizeof(buf), "%3d%%", static_cast<int>(volume * 100.0f + 0.5f));
        bar += buf;
        return bar;
    };

    std::string masterStr = makeBarStr("MST", AudioService::Get().GetMasterVolume());
    std::string bgmStr    = makeBarStr("BGM", AudioService::Get().GetBGMVolume());
    std::string seStr     = makeBarStr("SE ", AudioService::Get().GetSEVolume());

    renderer.DrawText(masterStr, { -0.95f, -0.65f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
    renderer.DrawText(bgmStr,    { -0.95f, -0.75f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
    renderer.DrawText(seStr,     { -0.95f, -0.85f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
}
