#include "TestStage.h"
#include "../Gameplay/SideScrollingShooter.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "../../Temp/ShootingGame.h"
#include "../../Engine/Graphics/Renderer.h"

TestStage::TestStage() : m_game(std::make_unique<SideScrollingShooter>()) {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    m_game->Initialize(getData().audio, getData().playerType);
}

void TestStage::ProcessInput() {
    m_game->ProcessInput();
}

void TestStage::Tick() {
    m_game->Tick();
}

void TestStage::Dispose() {
    m_game.reset();
}

void TestStage::Render(Renderer& renderer) {
    // ステージ本体が背景、3Dオブジェクト、UIを一つのRenderer経路へ登録する
    m_game->Render(renderer);
}
