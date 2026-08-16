#pragma once

#include <cstdint>

enum class KeyCode : std::uint16_t
{
    None = 0,

    // Alphabet
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Number row
    Alpha0,
    Alpha1,
    Alpha2,
    Alpha3,
    Alpha4,
    Alpha5,
    Alpha6,
    Alpha7,
    Alpha8,
    Alpha9,

    // Function keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // Navigation
    UpArrow,
    DownArrow,
    LeftArrow,
    RightArrow,

    Home,
    End,
    PageUp,
    PageDown,
    Insert,
    Delete,

    // Common keys
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,

    CapsLock,
    NumLock,
    ScrollLock,

    // Modifier keys
    LeftShift,
    RightShift,

    LeftControl,
    RightControl,

    LeftAlt,
    RightAlt,

    LeftWindows,
    RightWindows,

    // Numpad
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,

    NumpadAdd,
    NumpadSubtract,
    NumpadMultiply,
    NumpadDivide,
    NumpadDecimal,
    NumpadEnter,

    // Symbols
    Minus,
    Equals,

    LeftBracket,
    RightBracket,

    Backslash,
    Semicolon,
    Quote,

    Comma,
    Period,
    Slash,

    GraveAccent,

    // Misc
    PrintScreen,
    Pause,

    Count
};