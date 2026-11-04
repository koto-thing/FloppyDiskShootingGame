#pragma once
#include "KeyCode.h"
#include <SDL.h>

/** @brief Mac/Linux/Webで共通のSDL入力を既存入力APIへ渡す */
class SdlInputBackend {
public:
    /** @brief ウィンドウとコントローラーを初期化する @param window 入力先 @return 成功時true */
    static bool Initialize(SDL_Window* window);
    /** @brief 所有するコントローラーを解放する @return なし */
    static void Shutdown();
    /** @brief SDLイベントを処理する @param event イベント @return なし */
    static void ProcessEvent(const SDL_Event& event);
    /** @brief コントローラー状態をフレームへ反映する @return なし */
    static void Update();
    /** @brief 抽象キーをSDLスキャンコードへ変換する @param key キー @return SDLスキャンコード */
    static int ToScancode(KeyCode key);
private:
    /** @brief コントローラー操作を既存キーへ反映する @param slot プレイヤー番号 @param controller 入力元 @param seconds 経過秒数 @return なし */
    static void ApplyGamepad(int slot, SDL_GameController* controller, float seconds);
};
