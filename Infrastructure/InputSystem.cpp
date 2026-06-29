#include "InputSystem.h"
#include <windows.h>

bool InputSystem::IsKeyPressed(int vKey) {
    return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}
