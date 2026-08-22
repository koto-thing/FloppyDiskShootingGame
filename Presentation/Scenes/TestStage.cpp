#include "TestStage.h"
#include "../../Temp/ShootingGame.h"
#include "../../Engine/Graphics/Renderer.h"

TestStage::TestStage() : m_game(std::make_unique<ShootingGame>()) {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    m_game->Reset();
}

void TestStage::ProcessInput() {
}

void TestStage::Tick() {
    m_game->Tick();
}

void TestStage::Shutdown() {
    m_game.reset();
}

void TestStage::Render(Renderer& renderer) {
    // ステージ本体が背景、3Dオブジェクト、UIを一つのRenderer経路へ登録する
    m_game->Render(renderer);
}
