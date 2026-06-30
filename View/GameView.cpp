#include "GameView.h"
#include "../Infrastructure/D3D12Renderer.h"
#include "../Application/GameManager.h"
#include "../Presentation/GamePresenter.h"

GameView::GameView() {
}

GameView::~GameView() {
}

/**
 * ゲームの描画を行う
 * @param renderer D3D12Renderer のインスタンス
 * @param gameManager GameManager のインスタンス
 */
void GameView::Render(D3D12RenderingService& renderer, const GameManager& gameManager) {
    ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
    ConstantBufferData* cbvCpuData = static_cast<ConstantBufferData*>(renderer.GetCbvCpuData());
    ID3D12Resource* cbResource = renderer.GetConstantBuffer();

    int objectCount = GamePresenter::PrepareRenderData(gameManager.GetGameState(), cbvCpuData);

    for (int i = 0; i < objectCount; ++i) {
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = cbResource->GetGPUVirtualAddress() + i * 256;
        cmdList->SetGraphicsRootConstantBufferView(0, cbAddress);
        cmdList->DrawInstanced(4, 1, 0, 0);
    }
}
