#include "InputSystem.h"
#include <windows.h>

/**
 * キーが押されているかどうかを判定する
 * @param vKey 仮想キーコード
 * @return 押されている場合：true、押されていない場合：false
 */
bool InputSystem::IsKeyPressed(int vKey) {
    return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}
