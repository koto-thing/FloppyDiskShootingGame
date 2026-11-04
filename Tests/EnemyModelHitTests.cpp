#include <cassert>
#include <cstdio>
#include <memory>
#include <d3dcompiler.h>
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2Module.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"

struct EnemyModelHitTests {
    /** @brief 実ゲームの全ステージ描画と命中経路を検証する @return なし */
    static void Run() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        // 全ステージの通常敵で実描画コマンドを使い、遠方の弾を誤検出しない
        for (int stage = 1; stage <= 5; ++stage) {
            g.Reset();
            g.m_stageNumber = stage;
            g.m_stage = &Game::StageForNumber(stage, g.m_difficulty);
            g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
            g.m_viewTransitionTimer = 0;
            auto& enemy = g.m_enemies[0];
            enemy = {};
            enemy.active = true;
            enemy.hp = 100;
            enemy.z = 32.0f;
            Game::Shot shot;
            shot.z = 36;
            shot.vz = 10;
            shot.hitRadius = 0.03f;
            assert(g.HitEnemyModel(shot, enemy));
            assert(!g.m_modelCollisionRenderer.HasOverflowed());
            shot.x = 100;
            assert(!g.HitEnemyModel(shot, enemy));
        }
        // Lumiの旧本体球から外れる船体端への命中を実ダメージ経路まで確認する
        g.Reset();
        g.m_stageNumber = 2;
        g.m_stage = &Game::StageForNumber(2, g.m_difficulty);
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        g.m_viewTransitionTimer = 0;
        auto& boss = g.m_enemies[0];
        boss = {};
        boss.active = true;
        boss.type = 2;
        boss.hp = boss.maxHp = 1200;
        boss.z = 32;
        boss.phase = 1;
        const float hullY = -2.30f;
        Game::Shot edge;
        edge.active = true;
        edge.x = Game::FromWorldX(9.0f);
        edge.y = Game::FromWorldY(hullY);
        edge.z = 36;
        edge.vz = 10;
        edge.hitRadius = 0.02f;
        edge.damage = 1;
        assert(g.HitEnemyModel(edge, boss));
        assert(Game::Stage2Module::TryHitBossBody(g, edge, boss));
        g.HitPlayerShotTargets(edge);
        assert(boss.hp == 1199 && !edge.active);
        assert(!g.m_modelCollisionRenderer.HasOverflowed());
        // 船体外の空間ではHPが減らず、爆風でも同じ形状を使う
        edge.active = true;
        edge.x = Game::FromWorldX(30);
        edge.bomb = true;
        g.HitPlayerShotTargets(edge);
        assert(boss.hp == 1199 && edge.active);
        // 2Dでは実描画と同じ奥行き・遠近投影で船体の角へ当てる
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Side2D;
        ++g.m_frame;
        edge.bomb = false;
        edge.vz = 0;
        g.HitEnemyModel(edge, boss);
        const auto& hull = g.m_enemyModelCollision[0].primitives.front();
        const Vector3 corner = g.ShotModelProjection(edge).TransformPoint(
            hull.wvpMatrix.TransformPoint({0.49f, 0.49f, -0.49f}));
        edge.x = Game::FromWorldX(corner.x);
        edge.y = Game::FromWorldY(corner.y);
        assert(g.HitEnemyModel(edge, boss));
        // 破壊した部位の描画命令は同フレームでも消え、キャッシュへ残らない
        boss.bossPartHp.fill(10);
        g.HitEnemyModel(edge, boss);
        const auto completeCount = g.m_enemyModelCollision[0].primitives.size();
        boss.bossPartHp[Game::BossNose] = 0;
        g.HitEnemyModel(edge, boss);
        assert(g.m_enemyModelCollision[0].primitives.size() < completeCount);
    }
};

/** @brief ゲーム経路と共有HLSLのコンパイルを確認する @return 成功時0 */
int main() {
    // C++用の条件分岐を加えても描画用シェーダーの両エントリがコンパイルできる
    for (const char* entry : {"VSMain", "PSObject"}) {
        ID3DBlob* code = nullptr;
        ID3DBlob* errors = nullptr;
        const HRESULT result = D3DCompileFromFile(L"Shaders/ObjectShader.hlsl", nullptr, nullptr,
            entry, entry[0] == 'V' ? "vs_5_0" : "ps_5_0", 0, 0, &code, &errors);
        if (errors) { std::fputs(static_cast<const char*>(errors->GetBufferPointer()), stderr); errors->Release(); }
        assert(SUCCEEDED(result));
        if (code) code->Release();
    }
    EnemyModelHitTests::Run();
    std::puts("EnemyModelHitTests passed");
}
