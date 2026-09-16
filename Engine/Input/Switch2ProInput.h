#pragma once

#include "WindowsInputBackend.h"
#include <array>
#include <span>

/** @brief Switch 2 Proのレポート復号と接続時の中心補正 */
struct Switch2ProReport {
    std::array<int, 4> center {2048, 2048, 2048, 2048};
    std::array<int, 4> candidate {};
    unsigned stableSamples = 0;
    bool calibrated = false;

    /**
     * @brief USBのIDを除いた0x09レポートを既存ゲームパッド形式へ変換する
     * @param data Bluetooth通知またはUSBレポートのペイロード
     * @param state 正常なレポートの出力先
     * @return 補正が完了した有効レポートの場合はtrue
     */
    bool Decode(std::span<const unsigned char> data, XINPUT_GAMEPAD& state);

    /**
     * @brief 初代Proの0x30ペイロードを共通の操作割り当てへ変換する
     * @param data USBまたはBluetoothのIDを除いたペイロード
     * @param state 補正後の入力の出力先
     * @return 有効な補正済み入力の場合はtrue
     */
    bool DecodeSwitchPro(std::span<const unsigned char> data, XINPUT_GAMEPAD& state);
};

namespace Switch2ProInput {
/**
 * @brief 非同期で取得したSwitch 2 Proの入力を読み出す
 * @param state 最新入力の出力先
 * @param connection 接続を識別する通し番号の出力先
 * @param slot 直接接続スロット0または1
 * @return 有効な接続と新鮮な入力がある場合はtrue
 */
bool Poll(XINPUT_GAMEPAD& state, unsigned& connection, int slot = 0);
}
