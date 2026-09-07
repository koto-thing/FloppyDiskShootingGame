#pragma once

class InputSystem {
public:
    /**
     * @brief 仮想キーが押されているか判定する
     * @param vKey Windowsの仮想キーコード
     * @return 押されている場合true
     */
    static bool IsKeyPressed(int vKey);
};
