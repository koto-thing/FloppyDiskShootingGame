#pragma once

#include <Windows.h>

/**
 * @brief Win32の入力メッセージをInputが扱う形式へ変換する
 */
class WindowsInputBackend {
public:
    /**
     * @brief キーボードとマウスのRaw Inputを登録する
     * @param hwnd 入力メッセージを受信するウィンドウハンドル
     * @return 登録に成功した場合はtrue
     */
    static bool Initialize(HWND hwnd);

    /**
     * @brief Win32メッセージから入力状態を更新する
     * @param message メッセージ識別子
     * @param wParam メッセージ固有の追加情報
     * @param lParam メッセージ固有の追加情報
     */
    static void ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
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
};
