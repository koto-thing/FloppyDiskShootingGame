#pragma once

#include <array>
#include <cstddef>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// Win32のテキスト描画マクロをエンジン公開APIへ漏らさない
#ifdef DrawText
#undef DrawText
#endif

#include "KeyCode.h"
#include "MouseButton.h"
#include "../Math/Vector2.h"

class WindowsInputBackend;
class InputTestAccess;

/**
 * @brief キーボード、マウス、ゲームパッドの入力状態をフレーム単位で管理する
 */
class Input {
public:
    /**
     * @brief 入力状態を初期化してRaw Inputを登録する
     * @param hwnd 入力メッセージを受信するウィンドウハンドル
     * @return 初期化に成功した場合はtrue
     */
    static bool Initialize(HWND hwnd);

    /**
     * @brief 新しいフレームの入力受付を開始する
     * @return なし
     */
    static void BeginFrame();

    /**
     * @brief Windowsメッセージ処理後にゲームパッドを取得する
     * @return なし
     */
    static void PollGamepad();

    /**
     * @brief XInputゲームパッドが接続されているか取得する
     * @return 接続中の場合はtrue
     */
    static bool IsGamepadConnected();

    /**
     * @brief 指定したキーが押されているかを取得する
     * @param key 確認するキー
     * @return 押されている場合はtrue
     */
    static bool GetKey(KeyCode key);

    /**
     * @brief 指定したキーがこのフレームで押されたかを取得する
     * @param key 確認するキー
     * @return このフレームで押された場合はtrue
     */
    static bool GetKeyDown(KeyCode key);

    /**
     * @brief いずれかのキーがこのフレームで押されたかを取得する
     * @return このフレームでいずれかのキーが押された場合はtrue
     */
    static bool GetAnyKeyDown();

    /**
     * @brief 指定したキーがこのフレームで離されたかを取得する
     * @param key 確認するキー
     * @return このフレームで離された場合はtrue
     */
    static bool GetKeyUp(KeyCode key);

    /**
     * @brief 指定したマウスボタンが押されているかを取得する
     * @param button 確認するマウスボタン
     * @return 押されている場合はtrue
     */
    static bool GetMouseButton(MouseButton button);

    /**
     * @brief 指定したマウスボタンがこのフレームで押されたかを取得する
     * @param button 確認するマウスボタン
     * @return このフレームで押された場合はtrue
     */
    static bool GetMouseButtonDown(MouseButton button);

    /**
     * @brief 指定したマウスボタンがこのフレームで離されたかを取得する
     * @param button 確認するマウスボタン
     * @return このフレームで離された場合はtrue
     */
    static bool GetMouseButtonUp(MouseButton button);

    /**
     * @brief クライアント領域を基準としたマウス座標を取得する
     * @return マウス座標
     */
    static Vector2 GetMousePosition();

    /**
     * @brief このフレームのマウス移動量を取得する
     * @return マウス移動量
     */
    static Vector2 GetMouseDelta();

    /**
     * @brief このフレームのホイール回転量を取得する
     * @return WHEEL_DELTAを1としたホイール回転量
     */
    static float GetMouseWheelDelta();

    /**
     * @brief Win32メッセージを入力バックエンドへ渡す
     * @param message メッセージ識別子
     * @param wParam メッセージ固有の追加情報
     * @param lParam メッセージ固有の追加情報
     */
    static void ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    friend class WindowsInputBackend;
    friend class InputTestAccess;

    static std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_currentKeys;
    static std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_currentGamepadKeys;
    static std::array<unsigned char, static_cast<std::size_t>(KeyCode::Count)> m_frameStartKeySources;
    static std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_keyDown;
    static std::array<bool, static_cast<std::size_t>(KeyCode::Count)> m_keyUp;
    static std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_currentMouseButtons;
    static std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_currentGamepadMouseButtons;
    static std::array<unsigned char, static_cast<std::size_t>(MouseButton::Count)> m_frameStartMouseButtonSources;
    static std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_mouseButtonDown;
    static std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_mouseButtonUp;
    static bool m_gamepadConnected;
    static Vector2 m_mousePosition;
    static Vector2 m_mouseDelta;
    static float m_mouseWheelDelta;

    /**
     * @brief Win32仮想キーの状態を反映する
     * @param virtualKey Win32仮想キーコード
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetNativeKeyState(UINT virtualKey, bool isPressed);

    /**
     * @brief ゲームパッドから割り当てたキー状態を反映する
     * @param key 対象の抽象キー
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetGamepadKeyState(KeyCode key, bool isPressed);

    /**
     * @brief マウスボタンの状態を反映する
     * @param button 対象のマウスボタン
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetMouseButtonState(MouseButton button, bool isPressed);

    /**
     * @brief ゲームパッドから割り当てたマウスボタン状態を反映する
     * @param button 対象のマウスボタン
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetGamepadMouseButtonState(MouseButton button, bool isPressed);

    /**
     * @brief ゲームパッド由来のマウスボタン操作をクリックせず取り消す
     * @param button 対象のマウスボタン
     * @return なし
     */
    static void CancelGamepadMouseButtonState(MouseButton button);

    /**
     * @brief フォーカス喪失時に物理入力とフレーム内イベントを取り消す
     * @return なし
     */
    static void CancelNativeInputState();

    /**
     * @brief マウス移動量を加算する
     * @param x 水平方向の移動量
     * @param y 垂直方向の移動量
     */
    static void AddMouseDelta(float x, float y);

    /**
     * @brief マウスホイール回転量を加算する
     * @param delta WHEEL_DELTAを1とした回転量
     */
    static void AddMouseWheelDelta(float delta);

    /**
     * @brief クライアント領域を基準としたマウス座標を設定する
     * @param x 水平座標
     * @param y 垂直座標
     */
    static void SetMousePosition(float x, float y);

    /**
     * @brief KeyCodeをWin32仮想キーコードへ変換する
     * @param key 変換するキー
     * @return Win32仮想キーコード、未対応の場合は0
     */
    static int ToVirtualKey(KeyCode key);
};
