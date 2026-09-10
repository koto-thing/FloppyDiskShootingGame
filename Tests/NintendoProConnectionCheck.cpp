#include "../Engine/Input/Switch2ProInput.h"
#include <cstdio>

/** @brief 実機を30秒間読み取り、ボタンと軸の応答を報告する @return 入力受信時0、未受信時1 */
int main() {
    // ゲームと同じ直接接続APIを通して受信と再接続を確認する
    std::puts("Reading Nintendo Pro input for 30 seconds; release sticks first, then press buttons and move sticks.");
    unsigned connectedSamples = 0, connection = 0;
    WORD buttons = 0;
    int leftXMin = 0, leftXMax = 0, leftYMin = 0, leftYMax = 0;
    const auto deadline = GetTickCount64() + 30000;
    while (GetTickCount64() < deadline) {
        XINPUT_GAMEPAD state {};
        if (Switch2ProInput::Poll(state, connection)) {
            ++connectedSamples;
            buttons |= state.wButtons;
            if (state.sThumbLX < leftXMin) leftXMin = state.sThumbLX;
            if (state.sThumbLX > leftXMax) leftXMax = state.sThumbLX;
            if (state.sThumbLY < leftYMin) leftYMin = state.sThumbLY;
            if (state.sThumbLY > leftYMax) leftYMax = state.sThumbLY;
        }
        Sleep(10);
    }
    std::printf("Connected samples=%u connection=%u buttons=%04x LX=[%d,%d] LY=[%d,%d]\n",
        connectedSamples, connection, buttons, leftXMin, leftXMax, leftYMin, leftYMax);
    return connectedSamples ? 0 : 1;
}
