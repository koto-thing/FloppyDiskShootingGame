#include "TestStage.h"
#include "../Gameplay/SideScrollingShooter.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"

TestStage::TestStage() : m_game(std::make_unique<SideScrollingShooter>()) {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    m_game->Initialize(getData().audio);
}

void TestStage::ProcessInput() {
    m_game->ProcessInput();
}

void TestStage::Tick() {
    m_game->Tick();
}

void TestStage::Render(D3D12RenderingService& renderer) {
    m_game->Render(renderer);
}
