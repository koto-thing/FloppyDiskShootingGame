#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2Module.h"
#include "../Presentation/Gameplay/Stages/Stage4/Stage4Module.h"

struct HomingShotTests {
    /** @brief 高速移動への偏差射撃と実弾の通過点への十字の加減速を検証する @return なし */
    static void CheckPredictiveAim() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Easy);
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        const Vector3 aim = g.PlayerAimPoint();
        auto& enemy = g.m_enemies[0];
        enemy.active = true;
        enemy.hp = 10;
        enemy.x = Game::FromWorldX(aim.x);
        enemy.y = Game::FromWorldY(aim.y);
        enemy.z = aim.z;
        for (int frame = 0; frame < 60; ++frame) g.UpdateAimSnap();

        // 横へ高速移動する敵へ、発射後に曲がらない通常弾が到達する
        enemy.x += Game::FromWorldX(0.6f);
        g.UpdateAimSnap();
        assert(std::abs(g.Player().m_aimTargetVelocity.x - 0.6f) < 0.0001f);
        g.UpdateAimSnap(false);
        assert(std::abs(g.Player().m_aimTargetVelocity.x - 0.6f) < 0.0001f);
        const auto shot = g.MakeNormalPlayerShot();
        const Vector3 origin {Game::ToWorldX(shot.x), Game::ToWorldY(shot.y), shot.z};
        const Vector3 velocity {Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz};
        const float time = (enemy.z - origin.z) / velocity.z;
        const Vector3 target {Game::ToWorldX(enemy.x) + 0.6f * time, Game::ToWorldY(enemy.y), enemy.z};
        assert(Vector3::Distance(origin + velocity * time, target) < 0.001f);
        assert(std::abs(velocity.Length() - 1.45f) < 0.0001f);

        // 偏差射撃後もプレビューと実際に生成された通常弾の弾道が一致する
        g.Player().m_fire = true;
        g.TickPlayerWeapons();
        assert(g.m_shots[0].vx == shot.vx && g.m_shots[0].vy == shot.vy && g.m_shots[0].vz == shot.vz);
        Camera3D camera;
        g.ConfigureRailCamera(camera, g.m_aimViewport);
        const Vector3 future = origin + velocity * 15.0f;
        Vector2 screen;
        assert(camera.TryWorldToScreen(future, screen));
        const Vector2 desired {screen.x / g.m_aimViewport.width * 2.0f - 1.0f,
            1.0f - screen.y / g.m_aimViewport.height * 2.0f};
        assert(g.Player().m_crosshairPosition.x < desired.x);

        // 静止した目標へ移動開始時は加速し、接近すると減速して実弾の15フレーム先へ収束する
        enemy.x = Game::FromWorldX(aim.x + 0.8f);
        g.UpdateAimSnap();
        g.UpdateAimSnap();
        g.Player().m_crosshairPosition = {0.0f, 0.0f};
        g.Player().m_crosshairVelocity = {};
        g.UpdateAimSnap();
        const float initialSpeed = g.Player().m_crosshairVelocity.Length();
        g.UpdateAimSnap();
        assert(g.Player().m_crosshairVelocity.Length() > initialSpeed);
        for (int frame = 0; frame < 100; ++frame) g.UpdateAimSnap();
        const auto settled = g.MakeNormalPlayerShot();
        const Vector3 settledFuture = Vector3 {Game::ToWorldX(settled.x), Game::ToWorldY(settled.y), settled.z} +
            Vector3 {Game::ToWorldX(settled.vx), Game::ToWorldY(settled.vy), settled.vz} * 15.0f;
        assert(camera.TryWorldToScreen(settledFuture, screen));
        assert(std::abs(g.Player().m_crosshairPosition.x - (screen.x / g.m_aimViewport.width * 2.0f - 1.0f)) < 0.0001f);
        assert(std::abs(g.Player().m_crosshairPosition.y - (1.0f - screen.y / g.m_aimViewport.height * 2.0f)) < 0.0001f);
        assert(g.Player().m_crosshairVelocity.Length() < initialSpeed);

        // 弾と同速で接近する場合も迎撃でき、追いつけない横移動でも有限の弾道を保つ
        enemy.x = Game::FromWorldX(aim.x);
        for (int frame = 0; frame < 2; ++frame) g.UpdateAimSnap();
        g.Player().m_aimTargetVelocity = {0.0f, 0.0f, -1.45f};
        auto equalSpeed = g.MakeNormalPlayerShot();
        assert(std::isfinite(equalSpeed.vx) && std::isfinite(equalSpeed.vz));
        assert(std::abs(equalSpeed.vz - 1.45f) < 0.0001f);
        g.Player().m_aimTargetVelocity = {3.0f, 0.0f, 0.0f};
        auto unreachable = g.MakeNormalPlayerShot();
        assert(std::isfinite(unreachable.vx) && std::isfinite(unreachable.vy) && std::isfinite(unreachable.vz));

        // 捕捉対象の変更と2Dへの復帰では古い速度・十字の慣性を引き継がない
        enemy.active = false;
        g.m_enemies[1] = enemy;
        g.m_enemies[1].active = true;
        g.UpdateAimSnap();
        assert(g.Player().m_aimTargetVelocity == Vector3::Zero);
        g.m_viewMode = Game::ViewMode::Side2D;
        g.UpdateAimSnap();
        assert(!g.Player().m_crosshairInitialized && g.Player().m_crosshairVelocity == Vector2::Zero);
    }
    /** @brief 難易度別の捕捉範囲と最大弾道補正、および解除時の減衰を検証する @return なし */
    static void CheckAimSnapDifficulty() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Normal);
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        Camera3D camera;
        g.ConfigureRailCamera(camera, g.m_aimViewport);
        const Vector3 aim = g.PlayerAimPoint();
        Vector2 center, shifted;
        assert(camera.TryWorldToScreen(aim, center));
        assert(camera.TryWorldToScreen(aim + Vector3::Right, shifted));
        const float worldPerScreenHeight = g.m_aimViewport.height / (shifted.x - center.x);
        auto& enemy = g.m_enemies[0];
        enemy.active = true;
        enemy.hp = 10;
        enemy.y = Game::FromWorldY(aim.y);
        enemy.z = aim.z;
        float previousCorrection = 2.0f;
        for (auto difficulty : {Easy, Normal, Hard}) {
            g.m_difficulty = difficulty;
            const float radius = difficulty == Easy ? 0.09f : difficulty == Hard ? 0.035f : 0.06f;
            // 各難易度の境界の内外で捕捉が切り替わる
            for (float offset : {radius - 0.001f, radius + 0.001f}) {
                enemy.x = Game::FromWorldX(aim.x + worldPerScreenHeight * offset);
                g.UpdateAimSnap(false);
                assert(g.Player().m_aimSnapped == (offset < radius));
            }

            // 全難易度で捕捉できる同じ標的に対し、弾速を保って補正強度だけを変える
            enemy.x = Game::FromWorldX(aim.x + worldPerScreenHeight * 0.02f);
            g.Player().m_aimSnapBlend = 0.0f;
            g.Player().m_aimSnapOffset = {};
            for (int frame = 0; frame < 60; ++frame) g.UpdateAimSnap();
            const float strength = difficulty == Easy ? 1.0f : difficulty == Hard ? 0.45f : 0.75f;
            assert(std::abs(g.Player().m_aimSnapBlend - strength) < 0.0001f);
            Game::Shot shot;
            shot.x = g.Player().m_playerX;
            shot.y = g.Player().m_playerY;
            shot.z = g.PlayerWorldPosition().z + 2.0f;
            shot.vz = 1.45f;
            g.ApplyAimSnap(shot);
            const Vector3 velocity {Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz};
            assert(velocity.x > 0.0f && velocity.x < previousCorrection);
            assert(std::abs(velocity.Length() - 1.45f) < 0.0001f);
            previousCorrection = velocity.x;

            // 最大補正率が異なっても解除直後は滑らかに減衰し、最後は完全に戻る
            enemy.active = false;
            g.UpdateAimSnap();
            assert(g.Player().m_aimSnapBlend > 0.0f && g.Player().m_aimSnapBlend < strength);
            for (int frame = 0; frame < 40; ++frame) g.UpdateAimSnap();
            assert(g.Player().m_aimSnapBlend == 0.0f);
            enemy.active = true;
        }
    }
    /** @brief 常時表示の枠と弾道が捕捉・変更・解除時に連続して動くことを検証する @return なし */
    static void CheckSmoothAimSnap() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Normal);
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        Camera3D camera;
        g.ConfigureRailCamera(camera, g.m_aimViewport);
        Renderer renderer;
        g.UpdateAimSnap();
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);
        const float restingX = (renderer.Command(20).rect.position.x + renderer.Command(26).rect.position.x) * 0.5f;
        assert(std::abs(restingX - renderer.Command(6).rect.position.x) < 0.0001f);

        // 捕捉した最初のフレームでは、枠と発射方向を一気に標的へ飛ばさない
        const Vector3 aim = g.PlayerAimPoint();
        auto& enemy = g.m_enemies[0];
        enemy.active = true;
        enemy.hp = 10;
        enemy.x = Game::FromWorldX(aim.x + 0.8f);
        enemy.y = Game::FromWorldY(aim.y);
        enemy.z = aim.z;
        ++g.m_frame;
        g.UpdateAimSnap();
        assert(g.Player().m_aimSnapped);
        assert(g.Player().m_aimSnapOffset.x > 0.0f && g.Player().m_aimSnapOffset.x < 0.8f);
        const float firstOffset = g.Player().m_aimSnapOffset.x;
        g.UpdateAimSnap(false);
        assert(g.Player().m_aimSnapOffset.x == firstOffset);
        renderer.BeginFrame();
        g.DrawReticle(renderer, camera);
        const float acquiringX = (renderer.Command(20).rect.position.x + renderer.Command(26).rect.position.x) * 0.5f;
        assert(acquiringX > restingX);
        Game::Shot initial;
        initial.x = g.Player().m_playerX;
        initial.y = g.Player().m_playerY;
        initial.z = g.PlayerWorldPosition().z + 2.0f;
        initial.vz = 1.45f;
        auto acquiring = initial;
        g.ApplyAimSnap(acquiring);
        for (int frame = 0; frame < 40; ++frame) { ++g.m_frame; g.UpdateAimSnap(); }
        auto locked = initial;
        g.ApplyAimSnap(locked);
        assert(acquiring.vx > 0.0f && acquiring.vx < locked.vx);

        // 対象が反対側へ変わっても枠は現在位置から補間する
        const float lockedOffset = g.Player().m_aimSnapOffset.x;
        enemy.x = Game::FromWorldX(aim.x - 0.8f);
        ++g.m_frame;
        g.UpdateAimSnap();
        assert(g.Player().m_aimSnapOffset.x < lockedOffset && g.Player().m_aimSnapOffset.x > -0.8f);

        // 解除直後も枠と補正を残し、徐々に十字位置と通常弾道へ戻す
        const float previousOffset = g.Player().m_aimSnapOffset.x;
        const float previousBlend = g.Player().m_aimSnapBlend;
        enemy.active = false;
        ++g.m_frame;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        assert(g.Player().m_aimSnapOffset.x > 0.0f && g.Player().m_aimSnapOffset.x < previousOffset);
        assert(g.Player().m_aimSnapBlend > 0.0f && g.Player().m_aimSnapBlend < previousBlend);
        for (int frame = 0; frame < 40; ++frame) { ++g.m_frame; g.UpdateAimSnap(); }
        assert(g.Player().m_aimSnapOffset == Vector3::Zero && g.Player().m_aimSnapBlend == 0.0f);
        auto released = initial;
        g.ApplyAimSnap(released);
        assert(released.vx == initial.vx && released.vy == initial.vy && released.vz == initial.vz);
        renderer.BeginFrame();
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);
        assert(std::abs((renderer.Command(20).rect.position.x + renderer.Command(26).rect.position.x) * 0.5f - restingX) < 0.0001f);
    }
    /** @brief スナップ対象の選択、弾道補正、解除と表示を検証する @return なし */
    static void CheckAimSnap() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Easy);
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        const Vector3 aim = g.PlayerAimPoint();
        auto& enemy = g.m_enemies[0];
        enemy.active = true;
        enemy.hp = 10;
        enemy.x = Game::FromWorldX(aim.x + 0.5f);
        enemy.y = Game::FromWorldY(aim.y);
        enemy.z = aim.z;
        g.UpdateAimSnap();
        assert(g.Player().m_aimSnapped);
        const Vector3 target = g.Player().m_aimSnapTarget;
        for (int frame = 0; frame < 40; ++frame) { ++g.m_frame; g.UpdateAimSnap(); }

        // 十字を残して対象の枠を追加し、通常弾は同じ対象へ発射する
        Camera3D camera;
        g.ConfigureRailCamera(camera, g.m_aimViewport);
        Renderer renderer;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);
        Vector2 targetScreen;
        assert(camera.TryWorldToScreen(target, targetScreen));
        assert(std::abs((renderer.Command(20).rect.position.x + renderer.Command(26).rect.position.x) * 0.5f -
            (targetScreen.x / 1280.0f * 2.0f - 1.0f)) < 0.0001f);
        g.Player().m_fire = true;
        g.TickPlayerWeapons();
        const auto normal = g.m_shots[0];
        const Vector3 velocity {Game::ToWorldX(normal.vx), Game::ToWorldY(normal.vy), normal.vz};
        const Vector3 direction = (target - Vector3 {Game::ToWorldX(normal.x), Game::ToWorldY(normal.y), normal.z}).Normalized();
        assert(Vector3::Dot(velocity.Normalized(), direction) > 0.9999f);
        assert(std::abs(velocity.Length() - 1.45f) < 0.0001f);
        assert(g.m_shots[1].special && g.m_shots[2].special);
        assert(g.m_shots[1].vx != g.m_shots[2].vx);
        const auto snappedSpecial = g.m_shots[1];
        g.m_shots.fill({});
        g.Player().m_aimSnapped = false;
        g.Player().m_aimSnapBlend = 0.0f;
        g.FireSpecialShots();
        const auto& unsnappedSpecial = g.m_shots[0];
        assert(std::abs(snappedSpecial.vx - unsnappedSpecial.vx) > 0.0001f);
        const Vector3 snappedVelocity {Game::ToWorldX(snappedSpecial.vx), Game::ToWorldY(snappedSpecial.vy), snappedSpecial.vz};
        const Vector3 unsnappedVelocity {Game::ToWorldX(unsnappedSpecial.vx), Game::ToWorldY(unsnappedSpecial.vy), unsnappedSpecial.vz};
        assert(std::abs(snappedVelocity.Length() - unsnappedVelocity.Length()) < 0.0001f);
        g.m_shots.fill({});

        // 解像度を変えても同じ位置の敵へスナップする
        g.m_aimViewport = {0, 0, 2560, 1440};
        g.UpdateAimSnap();
        assert(g.Player().m_aimSnapped);
        g.m_aimViewport = {0, 0, 1280, 720};

        // 近傍外、背後、無効化、撃破済みの敵へはスナップしない
        enemy.x += 2.0f;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        enemy.x = Game::FromWorldX(aim.x + 0.5f);
        enemy.z = g.PlayerWorldPosition().z - 1.0f;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        enemy.z = aim.z;
        enemy.collisionEnabled = false;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        enemy.collisionEnabled = true;
        enemy.hp = 0;
        g.TickShots();
        assert(!g.Player().m_aimSnapped);
        renderer.BeginFrame();
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);

        // 2Dと切替中は標的が正面にいても弾道を変更しない
        enemy.hp = 10;
        g.m_viewMode = Game::ViewMode::Side2D;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        g.m_viewMode = Game::ViewMode::Rail3D;
        g.m_viewTransitionTimer = 1;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        g.m_viewTransitionTimer = 0;

        // 同じ候補でも照準に最も近い敵を選び、協力プレイでは各自の照準を使う
        auto& nearEnemy = g.m_enemies[1];
        nearEnemy = enemy;
        nearEnemy.x = Game::FromWorldX(aim.x + 0.1f);
        g.UpdateAimSnap();
        assert(std::abs(g.Player().m_aimSnapTarget.x - (aim.x + 0.1f)) < 0.0001f);
        g.m_playerCount = 2;
        g.m_activePlayer = 1;
        g.Player().m_playerX = 0.8f;
        g.UpdateAimSnap();
        assert(!g.Player().m_aimSnapped);
        assert(g.m_players[0].m_aimSnapped);

        // 壁面の上向き通常弾と周回戦の通常弾も、表示対象の命中位置へ向ける
        g.m_activePlayer = 0;
        g.m_playerCount = 1;
        g.m_enemies.fill({});
        g.m_stageNumber = 5;
        for (auto phase : {Game::Stage5Phase::WallClimbMiddle, Game::Stage5Phase::TayamaFireControl}) {
            g.m_stage5.phase = phase;
            g.m_stage5.tayamaOrbitAngle = 0.4f;
            const Vector3 aimPoint = g.PlayerAimPoint();
            enemy = {};
            enemy.active = true;
            enemy.hp = 10;
            enemy.x = Game::FromWorldX(aimPoint.x + 0.3f);
            enemy.y = Game::FromWorldY(aimPoint.y);
            enemy.z = aimPoint.z;
            g.UpdateAimSnap();
            assert(g.Player().m_aimSnapped);
            for (int frame = 0; frame < 60; ++frame) { ++g.m_frame; g.UpdateAimSnap(); }
            g.m_shots.fill({});
            g.Player().m_shotCooldown = 0;
            g.TickPlayerWeapons();
            const auto& shot = g.m_shots[0];
            Vector3 hitTarget = g.Player().m_aimSnapTarget;
            if (phase == Game::Stage5Phase::WallClimbMiddle) hitTarget.z = shot.z;
            const Vector3 desired = (hitTarget - Vector3 {Game::ToWorldX(shot.x), Game::ToWorldY(shot.y), shot.z}).Normalized();
            const Vector3 actual {Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz};
            assert(Vector3::Dot(desired, actual.Normalized()) > 0.9999f);
        }
    }
    /** @brief 3D照準の表示条件、投影位置、縦横比を検証する @return なし */
    static void CheckReticle() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Normal);
        Renderer renderer;
        Camera3D camera;
        camera.SetViewport({0, 0, 1280, 720});
        camera.SetPosition({0.0f, 0.0f, -10.0f});
        camera.LookAt({0.0f, 0.0f, 10.0f});

        // 2Dでは非表示、3Dでは暗い縁と黄色い十字を描画する
        g.m_viewMode = Game::ViewMode::Side2D;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 0);
        g.m_viewMode = Game::ViewMode::Rail3D;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);
        Vector2 expected;
        assert(camera.TryWorldToScreen(g.PlayerAimPoint(), expected));
        const auto& left = renderer.Command(8).rect;
        const auto& right = renderer.Command(10).rect;
        assert(std::abs((left.position.x + right.position.x) * 0.5f -
            (expected.x / 1280.0f * 2.0f - 1.0f)) < 0.0001f);
        assert(std::abs((left.position.y + right.position.y) * 0.5f -
            (1.0f - expected.y / 720.0f * 2.0f)) < 0.0001f);
        assert(std::abs(left.size.y * 720.0f - renderer.Command(9).rect.size.x * 1280.0f) < 0.0001f);

        // 壁面では上方向の弾道へ照準を合わせ、周回戦の2D視点でも非表示にする
        renderer.BeginFrame();
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::WallClimbLower;
        const Vector3 verticalTarget = g.PlayerAimPoint();
        camera.LookAt(verticalTarget);
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 0);
        g.m_stage5.phaseTimer = ShooterStages::Stage5::WallClimbFadeFrames;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 28);
        assert(std::abs(renderer.Command(8).rect.position.x + renderer.Command(10).rect.position.x) < 0.0001f);
        assert(std::abs(renderer.Command(8).rect.position.y + renderer.Command(10).rect.position.y) < 0.0001f);
        renderer.BeginFrame();
        g.m_stage5.phase = Game::Stage5Phase::TayamaFireControl;
        g.m_viewMode = Game::ViewMode::Side2D;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 0);
        g.m_stageNumber = 1;
        g.m_viewMode = Game::ViewMode::Rail3D;

        // どちら向きの視点切替中も、撃墜中とクリア中も非表示にする
        renderer.BeginFrame();
        g.m_viewTransitionTimer = 1;
        g.DrawReticle(renderer, camera);
        g.m_viewMode = Game::ViewMode::Side2D;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 0);
        g.m_viewMode = Game::ViewMode::Rail3D;
        g.m_viewTransitionTimer = 0;
        g.Player().m_playerDestructionTimer = 1;
        g.DrawReticle(renderer, camera);
        g.Player().m_playerDestructionTimer = 0;
        g.m_clear = true;
        g.DrawReticle(renderer, camera);
        assert(renderer.CommandCount() == 0);
    }
    /** @brief Stage4の全主砲と副砲がEasyのみ緩和されることを検証する @return なし */
    static void CheckStage4Easy() {
        using Game = SideScrollingShooter;
        using Weapon = ShooterStages::Stage4::MainWeaponType;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Normal);
        g.m_stageNumber = 4;
        Game::Enemy boss {};
        boss.type = 2;
        boss.bossPartHp.fill(100);
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            for (Weapon weapon : {Weapon::Phase1Cannon, Weapon::SiegeMortar, Weapon::RomanceCannon}) {
                g.m_stage4.currentWeapon = weapon;
                Game::Shot normal {};
                for (auto difficulty : {Normal, Hard, Easy}) {
                    g.m_difficulty = difficulty;
                    g.m_shots.fill({});
                    Game::Stage4Module::FireBossPartBarrage(g, boss);
                    const auto& shot = g.m_shots[0];
                    assert(shot.active);
                    if (difficulty == Normal) normal = shot;
                    const float speed = difficulty == Easy ? 0.75f : 1.0f;
                    const float radius = difficulty == Easy ? 0.7f : 1.0f;
                    assert(std::abs(shot.vx - normal.vx * speed) < 0.00001f);
                    assert(std::abs(shot.vy - normal.vy * speed) < 0.00001f);
                    assert(std::abs(shot.vz - normal.vz * speed) < 0.00001f);
                    assert(std::abs(shot.hitRadius - normal.hitRadius * radius) < 0.00001f);
                    assert(std::abs(shot.stage4.explosionRadius - normal.stage4.explosionRadius * radius) < 0.00001f);
                    assert(std::abs(shot.stage4.gravityScale - normal.stage4.gravityScale * speed * speed) < 0.00001f);
                    int count = 0;
                    for (const auto& bullet : g.m_shots) count += bullet.active;
                    assert(count == (weapon == Weapon::SiegeMortar ? (difficulty == Easy ? 4 : 6) : 1));
                }
            }
            // 一周期分の副砲を集計し、散射と狙撃の両方の減少を確認する
            for (auto difficulty : {Normal, Hard, Easy}) {
                g.m_difficulty = difficulty;
                int count = 0;
                for (int frame = 1; frame < 180; ++frame) {
                    boss.age = frame;
                    g.m_shots.fill({});
                    Game::Stage4Module::TickSecondaryGunAttacks(g, boss);
                    for (const auto& shot : g.m_shots)
                        if (shot.active && !shot.stage2.delayedEngine) ++count;
                }
                assert(count == (difficulty == Easy ? 14 : 26));
            }
        }
    }
    /** @brief Spreadの距離減衰と通常弾への非適用を実更新で検証する @return なし */
    static void CheckSpreadFalloff() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Initialize(nullptr, Spread, Normal);
        g.m_enemies.fill({});

        // 発射時の近距離威力と再利用スロットの距離リセットを確認する
        g.FireSpecialShots();
        assert(g.m_shots[0].damage == 3 && g.m_shots[0].travelDistance == 0.0f);
        for (bool rail : {false, true}) {
            g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            for (float distance : {0.0f, 3.49f, 3.5f, 6.99f, 7.0f, 100.0f}) {
                g.m_shots.fill({});
                auto& shot = g.m_shots[0];
                shot.active = shot.special = true;
                shot.playerType = Spread;
                shot.z = 10.0f;
                shot.travelDistance = distance;
                g.TickShots();
                assert(shot.active);
                assert(shot.damage == (distance < 3.5f ? 3 : distance < 7.0f ? 2 : 1));

                // 同じフレームの敵へのダメージに減衰後の値を使用する
                auto& enemy = g.m_enemies[0];
                enemy = {};
                enemy.active = enemy.collisionEnabled = true;
                enemy.hp = 100;
                enemy.x = shot.x;
                enemy.y = shot.y;
                enemy.z = shot.z;
                g.TickShots();
                assert(enemy.hp == 100 - shot.damage && !shot.active);
                enemy.active = false;
            }

            // 2Dの疑似奥行きは無視し、3Dでは奥行きを含む実距離を加算する
            auto& shot = g.m_shots[0];
            shot.active = true;
            shot.travelDistance = 0.0f;
            shot.vx = 0.03f;
            shot.vy = 0.04f;
            shot.vz = rail ? 0.5f : 0.0f;
            g.TickShots();
            const float dx = Game::ToWorldX(shot.vx);
            const float dy = Game::ToWorldY(shot.vy);
            const float expected = std::sqrt(dx * dx + dy * dy + shot.vz * shot.vz);
            assert(std::abs(shot.travelDistance - expected) < 0.00001f);

            // 自機の移動で距離が戻らず、通常弾には減衰を適用しない
            g.Player().m_playerX += 0.5f;
            shot.vx = shot.vy = shot.vz = 0.0f;
            g.TickShots();
            assert(std::abs(shot.travelDistance - expected) < 0.00001f);

            // 移動で境界を越えたフレームに威力が一段下がる
            for (float boundary : {3.5f, 7.0f}) {
                shot.travelDistance = boundary - 0.1f;
                shot.vx = 0.03f;
                g.TickShots();
                assert(shot.damage == (boundary == 3.5f ? 2 : 1));
            }
            shot.special = false;
            shot.damage = 1;
            g.TickShots();
            assert(shot.damage == 1);
        }
    }

    /** @brief 実際の追尾と部位衝突をGPUなしで検証する @return なし */
    static void Run() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
        g.m_stageNumber = 1;
        auto& nearEnemy = g.m_enemies[0];
        nearEnemy.active = nearEnemy.collisionEnabled = true;
        nearEnemy.hp = 100;
        nearEnemy.x = 0.4f;
        nearEnemy.y = 0.1f;
        auto& farEnemy = g.m_enemies[1];
        farEnemy = nearEnemy;
        farEnemy.hp = 1;
        farEnemy.x = 0.9f;
        farEnemy.y = -0.3f;
        Game::Shot shot;
        shot.vx = Game::PlayerShotConfigs[Homing].speed;

        // 遠い低HP敵より近い敵を優先し、破壊後は残った敵へ再選択する
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0 && shot.vy > 0.0f);
        nearEnemy.active = false;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == Game::BossPartCount + 1);
        farEnemy.active = false;
        const float vx = shot.vx;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == -1 && shot.vx == vx);

        // 部位照準が全ステージの2D/3D命中判定と一致し、破壊済み部位を返さない
        for (int stage = 1; stage <= 5; ++stage) {
            for (bool rail : {false, true}) {
                g.m_stageNumber = stage;
                g.m_stage5.phase = Game::Stage5Phase::EastsourceBattle;
                g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                auto& boss = g.m_enemies[0];
                boss = {};
                boss.type = 2;
                boss.active = boss.collisionEnabled = true;
                boss.hp = 100;
                boss.x = 0.6f;
                boss.z = 30.0f;
                boss.bossPartHp.fill(10);
                int count = 0;
                for (int i = 0; i < Game::BossPartCount; ++i) {
                    auto part = static_cast<Game::BossPart>(i);
                    Vector3 position;
                    if (!g.TryHitBossPart({}, boss, part, &position)) continue;
                    ++count;
                    Game::Shot probe;
                    probe.x = Game::FromWorldX(position.x);
                    probe.y = Game::FromWorldY(position.y);
                    probe.z = position.z;
                    assert(g.TryHitBossPart(probe, boss, part));
                    boss.bossPartHp[i] = 0;
                    part = static_cast<Game::BossPart>(i);
                    assert(!g.TryHitBossPart({}, boss, part, &position));
                    boss.bossPartHp[i] = 10;
                }
                assert(count > 0);
            }
        }

        // 近距離の小さな標的へ収束し、追尾中の速度を維持する
        g.m_stageNumber = 1;
        g.m_viewMode = Game::ViewMode::Side2D;
        nearEnemy = {};
        nearEnemy.active = nearEnemy.collisionEnabled = true;
        nearEnemy.hp = 10;
        nearEnemy.x = 0.3f;
        nearEnemy.y = 0.15f;
        shot = {};
        shot.vx = Game::PlayerShotConfigs[Homing].speed;
        bool hit = false;
        for (int frame = 0; frame < 30 && !hit; ++frame) {
            g.UpdateHomingShot(shot);
            assert(std::abs(std::hypot(shot.vx, shot.vy) - Game::PlayerShotConfigs[Homing].speed) < 0.00001f);
            shot.x += shot.vx;
            shot.y += shot.vy;
            hit = Game::Hit(shot.x, shot.y, shot.hitRadius, nearEnemy.x, nearEnemy.y, 0.03f);
        }
        assert(hit);

        // 3Dでも低HPより近距離を優先し、ワールド弾速を維持する
        g.m_viewMode = Game::ViewMode::Rail3D;
        nearEnemy.x = 0.1f;
        nearEnemy.y = 0.0f;
        nearEnemy.z = g.PlayerRailDepth() + 10.0f;
        farEnemy = nearEnemy;
        farEnemy.z += 20.0f;
        farEnemy.hp = 1;
        shot = {};
        shot.z = g.PlayerRailDepth();
        shot.vz = 1.45f;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0);
        assert(std::abs(Vector3(Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz).LengthSquared() - 1.45f * 1.45f) < 0.0001f);

        // 縦スクロール3Dでは敵の奥行きではなく弾平面へ投影した位置に向かう
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::WallClimbLower;
        farEnemy.active = false;
        nearEnemy.x = 0.2f;
        nearEnemy.y = 0.6f;
        nearEnemy.z = 46.0f;
        shot = {};
        shot.z = g.PlayerRailDepth();
        shot.vy = Game::PlayerShotConfigs[Homing].speed;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0 && shot.vx > 0.0f && shot.vy > 0.0f && shot.vz == 0.0f);

        // 周回でZ負方向へ撃つ場合も有効な弱点を狙い、破壊済み弱点を除外する
        nearEnemy.active = false;
        g.m_stage5.phase = Game::Stage5Phase::TayamaFireControl;
        g.m_stage5.tayamaOrbitAngle = Math::Pi;
        g.m_frame = 1;
        auto& weakpoint = g.m_stage5.tayamaWeakpoints[0];
        weakpoint.type = Game::TayamaWeakpoint::LeftSearchlight;
        weakpoint.active = true;
        weakpoint.destroyed = false;
        weakpoint.hp = 10;
        Vector3 weakpointPosition;
        assert(Game::Stage5Module::GetHomingTarget(g, 0, weakpointPosition));
        const Vector3 player = g.PlayerWorldPosition();
        shot = {};
        shot.x = Game::FromWorldX(player.x);
        shot.y = Game::FromWorldY(player.y);
        shot.z = player.z;
        shot.vz = weakpointPosition.z > player.z ? 1.45f : -1.45f;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget >= static_cast<int>(g.m_enemies.size()) * (Game::BossPartCount + 1));
        weakpoint.destroyed = true;
        assert(!Game::Stage5Module::GetHomingTarget(g, 0, weakpointPosition));
        // Stage2主砲は入力の継続先を狙い、予測で実際の自機位置を変更しない
        g.m_stageNumber = 2;
        g.m_difficulty = Hard;
        // 照準予測の検証中に発射される弾で移動状態を中断しない
        g.Player().m_invincible = 9999;
        g.Player().m_playerDestructionTimer = 0;
        for (bool rail : {false, true}) {
            g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            for (bool slow : {false, true}) {
                for (int input = 0; input < 16; ++input) {
                    g.Player().m_moveLeft = (input & 1) != 0;
                    g.Player().m_moveRight = (input & 2) != 0;
                    g.Player().m_moveUp = (input & 4) != 0;
                    g.Player().m_moveDown = (input & 8) != 0;
                    g.Player().m_slowMove = slow;
                    for (int cycle : {0, 30, 59}) {
                        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
                        for (int frame = cycle + 1; frame < 90; ++frame) g.TickPlayer();
                        const float expectedX = g.Player().m_playerX;
                        const float expectedY = g.Player().m_playerY;
                        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
                        Game::Enemy boss {};
                        boss.phase = 3.0f;
                        boss.hp = 30;
                        boss.maxHp = 100;
                        boss.age = 1;
                        g.m_stage2.boss = {};
                        g.m_stage2.boss.actionAge = cycle;
                        Game::Stage2Module::TickBoss(g, boss);
                        assert(g.Player().m_playerX == 0.0f && g.Player().m_playerY == 0.0f);
                        if (cycle == 0) assert(boss.attackWarningFrames == 90);
                        // 一度に予測位置へ飛ばず、各軸で予測位置との間へ進む
                        assert(expectedX == 0.0f ? boss.actionX == 0.0f :
                            boss.actionX / expectedX > 0.0f && boss.actionX / expectedX < 1.0f);
                        assert(expectedY == 0.0f ? boss.actionY == 0.0f :
                            boss.actionY / expectedY > 0.0f && boss.actionY / expectedY < 1.0f);
                        const float aimX = boss.actionX;
                        const float aimY = boss.actionY;
                        const float aimZ = boss.actionZ;
                        // HARDは確定後の0.5秒間と発射中に照準を固定する
                        g.Player().m_moveRight = !g.Player().m_moveRight;
                        for (int lockedFrame = 60; lockedFrame < 102; ++lockedFrame) {
                            g.m_stage2.boss.actionAge = lockedFrame;
                            Game::Stage2Module::TickBoss(g, boss);
                            assert(boss.actionX == aimX && boss.actionY == aimY && boss.actionZ == aimZ);
                        }
                        g.Player().m_moveRight = !g.Player().m_moveRight;
                    }
                }
            }
        }
        // 次の予告開始と発射後の追従再開でも照準を初期化しない
        for (int cycle : {0, 102, 179}) {
            Game::Enemy boss {};
            boss.phase = 3.0f;
            boss.hp = 30;
            boss.maxHp = 100;
            boss.age = 1;
            boss.actionX = 1.0f;
            g.Player().m_playerX = g.Player().m_playerY = 0.0f;
            g.Player().m_moveLeft = g.Player().m_moveRight = g.Player().m_moveUp = g.Player().m_moveDown = false;
            g.m_stage2.boss.actionAge = cycle;
            Game::Stage2Module::TickBoss(g, boss);
            assert(boss.actionX > 0.8f && boss.actionX < 1.0f);
        }
        // EASY/NORMALは移動入力があっても現在位置へ追従し、確定後は固定する
        for (auto difficulty : {Easy, Normal}) {
            g.m_difficulty = difficulty;
            g.Player().m_moveRight = true;
            g.Player().m_playerX = 0.5f;
            Game::Enemy boss {};
            boss.phase = 3.0f;
            boss.hp = 30;
            boss.maxHp = 100;
            boss.age = 1;
            boss.actionX = g.Player().m_playerX;
            g.m_stage2.boss.actionAge = 30;
            Game::Stage2Module::TickBoss(g, boss);
            assert(boss.actionX == g.Player().m_playerX);
            const float aimX = boss.actionX;
            const float aimY = boss.actionY;
            const float aimZ = boss.actionZ;
            g.Player().m_playerX = -0.5f;
            for (int frame = 60; frame < 132; ++frame) {
                g.m_stage2.boss.actionAge = frame;
                Game::Stage2Module::TickBoss(g, boss);
                assert(boss.actionX == aimX && boss.actionY == aimY && boss.actionZ == aimZ);
            }
        }
        // ラスボス第2形態の3D移動端と、その位置から撃つ弾の生存を確認する
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::TayamaDragonBattle;
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        g.Player().m_moveLeft = g.Player().m_moveRight = g.Player().m_moveUp = g.Player().m_moveDown = false;
        for (float edge : {-1.35f, 1.35f}) {
            g.Player().m_playerX = edge * 2.0f;
            g.Player().m_playerY = 4.0f;
            g.TickPlayer();
            assert(g.Player().m_playerX == edge && g.Player().m_playerY == 3.5f);
            g.m_shots.fill({});
            auto& edgeShot = g.m_shots[0];
            edgeShot.active = true;
            edgeShot.x = edge;
            edgeShot.y = g.Player().m_playerY;
            edgeShot.z = 10.0f;
            g.TickShots();
            assert(edgeShot.active);
        }
        std::puts("HomingShotTests passed");
    }
};

/** @brief 追尾回帰チェックを実行する @return 成功時0 */
int main() {
    HomingShotTests::CheckPredictiveAim();
    HomingShotTests::CheckAimSnapDifficulty();
    HomingShotTests::CheckSmoothAimSnap();
    HomingShotTests::CheckAimSnap();
    HomingShotTests::CheckReticle();
    HomingShotTests::CheckStage4Easy();
    HomingShotTests::CheckSpreadFalloff();
    HomingShotTests::Run();
    return 0;
}
