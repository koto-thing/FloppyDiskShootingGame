#pragma once

class D3D12RenderingService;
class GameManager;

class GameView {
public:
    GameView();
    ~GameView();

    void Render(D3D12RenderingService& renderer, const GameManager& gameManager);
};
