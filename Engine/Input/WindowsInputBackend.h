#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>

class WindowsInputBackendTestAccess;

/**
 * @brief Win32入力とXInputをInputが扱う形式へ変換する
 */
class WindowsInputBackend {
public:
    /**
     * @brief キーボードとマウスのRaw InputおよびXInputを初期化する
     * @param hwnd 入力メッセージを受信するウィンドウハンドル
     * @return 登録に成功した場合はtrue
     */
    static bool Initialize(HWND hwnd);

    /**
     * @brief 接続中のXInputゲームパッドを取得して入力状態へ反映する
     * @return なし
     */
    static void Update();

    /**
     * @brief Win32メッセージから入力状態を更新する
     * @param message メッセージ識別子
     * @param wParam メッセージ固有の追加情報
     * @param lParam メッセージ固有の追加情報
     * @return なし
     */
    static void ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    friend class WindowsInputBackendTestAccess;

    /**
     * @brief Raw Inputのキーボード情報を反映する
     * @param keyboard キーボード入力情報
     */
    static void ProcessKeyboard(const RAWKEYBOARD& keyboard);

    /**
     * @brief Raw Inputのマウス情報を反映する
     * @param mouse マウス入力情報
     */
    static void ProcessMouse(const RAWMOUSE& mouse);

    /**
     * @brief 左右を区別した仮想キーコードへ変換する
     * @param keyboard キーボード入力情報
     * @return 正規化したWin32仮想キーコード
     */
    static UINT NormalizeVirtualKey(const RAWKEYBOARD& keyboard);

    /**
     * @brief 再接続時の押下を抑止して取得済みゲームパッド状態を反映する
     * @param gamepad ゲームパッド状態、未接続の場合はnullptr
     * @param elapsedSeconds 前回取得からの秒数
     * @return なし
     */
    static void ProcessPolledGamepad(const XINPUT_GAMEPAD* gamepad, float elapsedSeconds);

    /**
     * @brief XInput状態を既存のキーとポインター操作へ割り当てる
     * @param gamepad ゲームパッド状態、未接続の場合はnullptr
     * @param elapsedSeconds 前回取得からの秒数
     * @return なし
     */
    static void ProcessGamepad(const XINPUT_GAMEPAD* gamepad, float elapsedSeconds);

    /**
     * @brief 右スティックでUI用ポインターを移動する
     * @param x 水平方向の正規化入力
     * @param y 垂直方向の正規化入力
     * @param elapsedSeconds 前回取得からの秒数
     * @return なし
     */
    static void MoveGamepadPointer(float x, float y, float elapsedSeconds);
};
