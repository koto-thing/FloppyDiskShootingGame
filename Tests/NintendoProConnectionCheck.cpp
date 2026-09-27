#include "../Engine/Input/Switch2ProInput.h"
#include <cstdio>

/** @brief 実機を30秒間読み取り、ボタンと軸の応答を報告する @return 入力受信時0、未受信時1 */
int main() {
    // ゲームと同じ直接接続APIを通して受信と再接続を確認する
    std::puts("Reading Nintendo Pro input for 30 seconds; release sticks first, then press buttons and move sticks.");
    std::array<unsigned, 2> connectedSamples {}, connection {};
    std::array<WORD, 2> buttons {};
    std::array<int, 2> leftXMin {}, leftXMax {}, leftYMin {}, leftYMax {};
    const auto deadline = GetTickCount64() + 30000;
    while (GetTickCount64() < deadline) {
        for (int slot = 0; slot < 2; ++slot) {
            XINPUT_GAMEPAD state {};
            if (Switch2ProInput::Poll(state, connection[slot], slot)) {
                ++connectedSamples[slot];
                buttons[slot] |= state.wButtons;
                if (state.sThumbLX < leftXMin[slot]) leftXMin[slot] = state.sThumbLX;
                if (state.sThumbLX > leftXMax[slot]) leftXMax[slot] = state.sThumbLX;
                if (state.sThumbLY < leftYMin[slot]) leftYMin[slot] = state.sThumbLY;
                if (state.sThumbLY > leftYMax[slot]) leftYMax[slot] = state.sThumbLY;
            }
        }
        Sleep(10);
    }
    for (int slot = 0; slot < 2; ++slot) {
        std::printf("Native slot=%d samples=%u connection=%u buttons=%04x LX=[%d,%d] LY=[%d,%d]\n",
            slot, connectedSamples[slot], connection[slot], buttons[slot],
            leftXMin[slot], leftXMax[slot], leftYMin[slot], leftYMax[slot]);
    }
    return connectedSamples[0] || connectedSamples[1] ? 0 : 1;
}
