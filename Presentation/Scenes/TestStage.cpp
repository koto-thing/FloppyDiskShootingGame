#include "TestStage.h"
#include "../../Temp/ShootingGame.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"

TestStage::TestStage() {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    //m_game->Initialize();
}

void TestStage::ProcessInput() {
    // 入力は ShootingGame 内で InputSystem を直接参照するため、ここでは特になし
}

void TestStage::Tick() {
    //m_game->Update();
}

void TestStage::Render(D3D12RenderingService& renderer) {
    //m_game->Render(renderer);
}
