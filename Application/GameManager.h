#pragma once
#include "../Domain/GameState.h"

class GameManager {
public:
    GameManager();
    ~GameManager();

    void Initialize();
    void ProcessInput();
    void Update();

    const GameState& GetGameState() const { return m_state; }
    GameState& GetGameState() { return m_state; }

private:
    GameState m_state;
};
