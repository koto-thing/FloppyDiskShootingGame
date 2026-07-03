#include "TestStage.h"
#include "../../Temp/ShootingGame.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"

TestStage::TestStage() : m_game(std::make_unique<ShootingGame>()) {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    m_game->Reset();
}

void TestStage::ProcessInput() {
}

void TestStage::Tick() {
    m_game->Update();
}

void TestStage::Render(D3D12RenderingService& renderer) {
    m_game->Render(renderer);
}
