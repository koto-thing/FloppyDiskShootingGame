#pragma once

class D3D12Renderer;
class GameManager;

class GameView {
public:
    GameView();
    ~GameView();

    void Render(D3D12Renderer& renderer, const GameManager& gameManager);
};
