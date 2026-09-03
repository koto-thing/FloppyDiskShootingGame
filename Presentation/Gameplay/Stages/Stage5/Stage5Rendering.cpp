#include "Stage5Module.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../../SideScrollingShooterShared.h"
#include "../Common/StageDefinition.h"
#include "Stage5ModelView.h"
#include "Stage5CityModelView.h"
#include "WallSecurityDroneModelView.h"

namespace {
constexpr float TowerFacadeColor[4] = { 0.10f, 0.13f, 0.24f, 1.0f };
constexpr float SatelliteLightColor[4] = { 0.82f, 0.94f, 1.0f, 1.0f };
constexpr float SearchlightColor[4] = { 1.00f, 0.82f, 0.20f, 0.24f };
constexpr float SearchlightLockedColor[4] = { 1.00f, 0.08f, 0.08f, 0.50f };
constexpr float DronePointerColor[4] = { 1.00f, 0.03f, 0.02f, 0.72f };
constexpr float StormCloudColor[4] = { 0.05f, 0.07f, 0.13f, 1.0f };
constexpr float BuildingWindowColor[4] = { 0.20f, 0.78f, 0.92f, 1.0f };
constexpr int RainCycle = 240;
constexpr int RainFallSpeed = 4;
constexpr int RainSplashDuration = 32;

/**
 * @brief 雨粒の落下位相を周期内へ折り返す
 * @param index 雨粒番号
 * @param frame 現在フレーム
 * @return 0が接地、RainCycle未満が上空となる落下位相
 */
constexpr int RainFallPhase(int index, int frame) {
    return (index * 83 + RainCycle - (frame * RainFallSpeed) % RainCycle) % RainCycle;
}

/**
 * @brief 接地直後の経過位相を跳ね返り進行率へ変換する
 * @param fallPhase 現在の落下位相
 * @return 表示期間中は0から1、期間外は負数
 */
constexpr float RainSplashProgress(int fallPhase) {
    const int age = (RainCycle - fallPhase) % RainCycle;
    return age < RainSplashDuration ?
        static_cast<float>(age) / static_cast<float>(RainSplashDuration) : -1.0f;
}
constexpr int CityBuildingCount = 30;
constexpr float CityBuildingNdcSpacing = 2.0f / CityBuildingCount;
constexpr float Stage5CityBuildingScale = 2.0f;
constexpr int Stage5CityBuildingCount = static_cast<int>(Stage5BuildingType::Count);
static_assert(Stage5CityBuildingCount == 7);

/**
 * @brief 値を横画面の循環範囲へ収める
 * @param value 循環前の値
 * @return -1以上1未満の値
 */
float WrapCityNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) wrapped += 2.0f;
    return wrapped - 1.0f;
}

/**
 * @brief 値を正の距離範囲へ収める
 * @param value 循環前の値
 * @param length 循環距離
 * @return 0以上length未満の値
 */
float WrapCityDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) wrapped += length;
    return wrapped;
}

/**
 * @brief Stage 5進行状態から雨量を取得する
 * @param phase 現在の進行状態
 * @param chapter 現在のチャプター番号
 * @param tayamaTransformation TAYAMA変形率
 * @param phaseTimer 現在状態の経過フレーム数
 * @return 0から1の雨量
 */
constexpr float RainIntensity(ShooterStages::Stage5::Phase phase, int chapter,
    float tayamaTransformation, int phaseTimer) {
    if (phase == ShooterStages::Stage5::Phase::Approach) {
        return 0.22f + static_cast<float>(chapter - 1) * 0.27f;
    }
    if (phase <= ShooterStages::Stage5::Phase::EastsourceFall) return 1.0f;
    if (phase <= ShooterStages::Stage5::Phase::WallClimbUpper) {
        return 0.88f - tayamaTransformation * 0.28f;
    }
    if (phase <= ShooterStages::Stage5::Phase::TayamaFireControl) return 0.42f;
    if (phase == ShooterStages::Stage5::Phase::TayamaLiftEngines) return 0.22f;
    if (phase == ShooterStages::Stage5::Phase::TayamaCommandCore) {
        return 0.22f * (1.0f - Math::Clamp01(static_cast<float>(phaseTimer) / 180.0f));
    }
    return 0.0f;
}

static_assert(RainIntensity(ShooterStages::Stage5::Phase::Approach, 1, 0.0f, 0) == 0.22f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::EastsourceBattle, 3, 0.0f, 0) == 1.0f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::TayamaCommandCore, 3, 1.0f, 180) == 0.0f);
static_assert(RainFallPhase(0, 0) == 0);
static_assert(RainFallPhase(0, 1) == RainCycle - RainFallSpeed);
static_assert(RainSplashProgress(RainCycle - RainFallSpeed) == 0.125f);
static_assert(RainSplashProgress(RainCycle - RainSplashDuration) < 0.0f);
}

/**
 * @brief Stage 5の壁面上昇と崩壊に合わせてレールカメラを補正する
 * @param shooter 判定対象
 * @param railPosition 補正するカメラ位置
 * @param railTarget 補正する注視点
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ApplyCameraCorrection(
    const SideScrollingShooter& shooter, Vector3& railPosition, Vector3& railTarget) {
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
        // 暗転解除後は道路を進み、ビル直前から壁面上方へカメラを向ける
        const float approach = SmoothStep(ShooterStages::Stage5::WallApproachProgress(
            shooter.m_stage5.phaseTimer));
        const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
            shooter.m_stage5.phaseTimer));
        const float turn = SmoothStep(Math::Clamp01(climb * 4.0f));
        const Vector3 playerPosition {
            ToWorldX(shooter.m_playerX),
            -0.65f + climb * ShooterStages::Stage5::WallClimbHeight,
            Math::Lerp(ShooterStages::Stage5::WallApproachStartZ,
                ShooterStages::Stage5::WallApproachEndZ, approach)
        };
        const Vector3 roadCamera = playerPosition + Vector3 {0.0f, 1.2f, -9.0f};
        const Vector3 climbCamera = playerPosition + Vector3 {0.0f, 0.5f, -16.0f};
        const Vector3 roadTarget = playerPosition + Vector3 {0.0f, 0.0f, 14.0f};
        constexpr float BuildingTopY = -3.65f + ShooterStages::Stage5::PandDBuildingHeight;
        const Vector3 climbTarget {
            playerPosition.x,
            (std::min)(playerPosition.y + 48.0f, BuildingTopY),
            42.0f
        };
        railPosition = Vector3::Lerp(roadCamera, climbCamera, turn);
        railTarget = Vector3::Lerp(roadTarget, climbTarget, turn);
        return;
    }

    // 第2部道中は自機後方の少し上から中央外壁のはるか上を見上げる
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        const float section = shooter.m_stage5.phase == Stage5Phase::WallClimbLower ? 0.0f :
            (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle ? 0.5f : 1.0f);
        railPosition.y += 0.72f;
        railPosition.z -= 6.0f;
        railTarget.y += Math::Lerp(10.0f, 14.0f, section);
        railTarget.z += 8.0f;
        return;
    }

    // 屋上到着後は振動を伴って後退し、変形前のビル全体を画角へ収める
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        const float reveal = shooter.m_stage5.phase == Stage5Phase::RooftopArrival ?
            SmoothStep(Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer -
                ShooterStages::Stage5::WallClimbFadeFrames) / 120.0f)) : 1.0f;
        const float shake = shooter.m_stage5.phase == Stage5Phase::RooftopArrival ?
            std::sin(static_cast<float>(shooter.m_stage5.phaseTimer) * 0.72f) * 0.16f : 0.0f;
        const Stage5ModelTransform boss = TayamaTransform(shooter);
        const float towerCenterY = boss.position.y +
            (TayamaModelView::TowerBoundsMin.y + TayamaModelView::TowerBoundsMax.y) *
                0.5f * boss.scale;
        const float targetY = Math::Lerp(towerCenterY, boss.position.y + 0.8f,
            shooter.m_stage5.tayamaTransformation);
        railPosition = Vector3::Lerp({0.0f, 1.4f, 35.0f},
            {0.0f, 5.0f, 17.0f}, reveal) + Vector3 {shake, -shake * 0.45f, 0.0f};
        railTarget = {0.0f, targetY, 57.0f};
        return;
    }

    // 戦闘中は自機の横移動に引かれない正面構図で巨大メカを中央へ固定する
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        const Stage5ModelTransform boss = TayamaTransform(shooter);
        railPosition = {ToWorldX(shooter.m_playerX) * 0.08f, 1.5f, -9.0f};
        railTarget = {0.0f, boss.position.y + 0.8f, boss.position.z};
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        const float pullBack = Math::Clamp01(
            static_cast<float>(shooter.m_stage5.phaseTimer - 60) / 390.0f);
        railPosition.z -= pullBack * 8.0f;
        railPosition.y += pullBack * 2.0f;
        railTarget.z += pullBack * 7.0f;
    }
}

/**
 * @brief Stage 5演出用のレールカメラ画角を取得する
 * @param shooter 判定対象
 * @param defaultFieldOfView 通常の画角
 * @return 適用する画角
 */
float SideScrollingShooter::Stage5Module::CameraFieldOfView(
    const SideScrollingShooter& shooter, float defaultFieldOfView) {
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        // 広角化して中央外壁の上方と下方の市街を同時に画面へ残す
        return Math::Lerp(defaultFieldOfView, 70.0f, shooter.RailBlend());
    }
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        return 70.0f;
    }
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        return 42.0f;
    }
    if (shooter.m_stage5.phase != Stage5Phase::WallClimbTransition) {
        return defaultFieldOfView;
    }
    const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
        shooter.m_stage5.phaseTimer));
    return Math::Lerp(70.0f, 132.0f,
        SmoothStep(Math::Clamp01(climb * 4.0f)));
}

/**
 * @brief 現在の演出状態で敵を描画するか判定する
 * @param shooter 判定対象
 * @param enemy 描画候補
 * @return 描画する場合true、格納庫内に隠す場合false
 */
bool SideScrollingShooter::Stage5Module::ShouldDrawEnemy(
    const SideScrollingShooter& shooter, const Enemy& enemy) {
    return !(shooter.m_stage5.phase == Stage5Phase::EastsourceIntro &&
        shooter.m_stage5.phaseTimer < 58 && enemy.type == Stage::BossEnemy);
}

/**
 * @brief Stage 5専用2D画面エフェクトを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawOverlay2D(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    DrawScreenEffects(shooter, renderer);
}

/**
 * @brief EASTSOURCE撃破後ムービー用の自機描画Transformを適用する
 * @param shooter 判定対象
 * @param position 補正する自機ワールド座標
 * @param pitch 補正する自機X軸回転
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ApplyPlayerRenderCorrection(
    const SideScrollingShooter& shooter, Vector3& position, float& pitch) {
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        // カメラを残し、自機だけを画面奥へ飛ばしてから暗転する
        const float progress = SmoothStep(ShooterStages::Stage5::EastsourceFlyAwayProgress(
            shooter.m_stage5.phaseTimer));
        position.z = Math::Lerp(PlayerRailZ, 42.0f, progress);
        position.y += progress * 1.8f;
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
        const float approach = SmoothStep(ShooterStages::Stage5::WallApproachProgress(
            shooter.m_stage5.phaseTimer));
        const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
            shooter.m_stage5.phaseTimer));
        const float turn = SmoothStep(Math::Clamp01(climb * 4.0f));
        position.y = -0.65f + climb * ShooterStages::Stage5::WallClimbHeight;
        position.z = Math::Lerp(ShooterStages::Stage5::WallApproachStartZ,
            ShooterStages::Stage5::WallApproachEndZ, approach);
        pitch = Math::Lerp(0.0f, -Math::HalfPi, turn);
        return;
    }
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        // 2Dから3Dへ切り替わる間も機首を連続的に壁面上方へ向ける
        pitch = Math::Lerp(0.0f, -Math::HalfPi, shooter.RailBlend());
    }
}

/**
 * @brief Stage 5専用3D画面エフェクトを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawOverlay3D(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    DrawScreenEffects(shooter, renderer);
}

/**
 * @brief Stage 5専用HUDを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return 専用HUDが共通ボスHUDを置き換える場合true、共通ボスHUDを描画する場合false
 */
bool SideScrollingShooter::Stage5Module::DrawHud(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    // Approach以外は現行と同じく空表示の終幕も共通ボスHUDへ戻さない
    if (shooter.m_stage5.phase == Stage5Phase::Approach) return false;
    DrawStage5Hud(shooter, renderer);
    return true;
}

/**
 * @brief EASTSOURCE専用攻撃予告を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param enemy 攻撃予告を持つ敵
 * @param size 共通処理で算出した予告サイズ
 * @return 専用予告を描画して共通予告を省略する場合true、共通予告を使用する場合false
 */
bool SideScrollingShooter::Stage5Module::DrawSpecialAttackWarning3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float size) {
    if (enemy.type != Stage::BossEnemy ||
        shooter.m_stage5.phase < Stage5Phase::EastsourceIntro ||
        shooter.m_stage5.phase > Stage5Phase::EastsourceBattle) return false;

    // 固定した照準地点をプレイヤー面へ表示して発射後の追尾と誤認させない
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    shooter.DrawModelPrimitive(renderer, camera, 1,
        ToWorldX(enemy.attackWarningTargetX), ToWorldY(enemy.attackWarningTargetY),
        PlayerRailZ + 0.3f, size, size, size, FlashColor);
    return true;
}

/**
 * @brief EASTSOURCEと外壁警備ドローンの専用モデルを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param enemy 描画する敵
 * @return 専用モデルを描画して共通敵モデルを省略する場合true
 */
bool SideScrollingShooter::Stage5Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy) {
    if (enemy.type == Stage::WallSecurityDroneEnemy) {
        // サーチライトと機関銃を走査地点または接触時に固定した地点へ向ける
        WallSecurityDronePose pose;
        pose.sensorYaw = (std::clamp)((enemy.turretAimX - enemy.x) * 0.72f, -0.68f, 0.68f);
        pose.sensorPitch = (std::clamp)((enemy.turretAimY - enemy.y) * 0.34f, -0.32f, 0.32f);
        pose.searchLightYaw = pose.sensorYaw;
        pose.searchLightPitch = pose.sensorPitch;
        pose.machineGunYaw = pose.sensorYaw;
        pose.machineGunPitch = pose.sensorPitch;
        pose.machineGunDeployment = SmoothStep(Math::Clamp01(
            (EnemyRailFarZ - enemy.z) / 12.0f));
        pose.contactExtension = pose.machineGunDeployment;
        pose.warningLightColor = enemy.motionAge > 0 ?
            WallSecurityDroneModelView::AttackWarning : (enemy.recoilAge > 0 ?
                WallSecurityDroneModelView::DetectedWarning :
                WallSecurityDroneModelView::PatrolWarning);
        const Stage5ModelTransform transform {
            {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z}, {}, 1.45f};
        WallSecurityDroneModelView::DrawAll(transform, pose,
            [&](PrimitiveShape shape, const Matrix4x4& world,
                const ColorF& color, WallSecurityDronePartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });

        // 灯体レンズから走査地点まで細い赤色光軸と着弾点を描画する
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);
        const Vector3 source = root.TransformPoint(
            WallSecurityDroneModelView::SearchLightOriginLocalPosition(
                pose.searchLightYaw, pose.searchLightPitch));
        const Vector3 target {ToWorldX(enemy.turretAimX), ToWorldY(enemy.turretAimY),
            shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ};
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float beamYaw = std::atan2(direction.z, -direction.x);
        const float beamPitch = -std::asin(direction.y);
        const Matrix4x4 beamWorld = Matrix4x4::Translation(
            source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(beamYaw) * Matrix4x4::RotationZ(beamPitch) *
            Matrix4x4::Scale({length, enemy.motionAge > 0 ? 0.055f : 0.025f, 0.025f});
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), beamWorld, DronePointerColor);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Sphere), target.x, target.y, target.z,
            0.16f, 0.16f, 0.05f, DronePointerColor);
        return true;
    }
    if (enemy.type != Stage::BossEnemy) return false;

    const Stage5ModelTransform transform = EastsourceTransform(shooter, enemy);
    const EastsourceModelState state = EastsourceState(enemy);

    // 参照ブランチdrawBoss1の26パーツとXYZ回転を変更せず描画する
    EastsourceModelView::VisitParts(transform, state,
        [&](PrimitiveShape shape, const Matrix4x4& world,
            const ColorF& color, EastsourcePartGroup) {
            const float partColor[] = {color.r, color.g, color.b, color.a};
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(shape), world, partColor);
        });

    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };
    for (int part = BossNose; part <= BossRightEngine; ++part) {
        const int maxHp = enemy.bossPartMaxHp[part];
        if (maxHp <= 0 || enemy.bossPartHp[part] <= 0 ||
            enemy.bossPartHp[part] * 100 > maxHp * 35) continue;
        const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
            transform, state, Groups[part]);
        if (!bounds.valid) continue;
        const Matrix4x4 world = Matrix4x4::Translation(bounds.center) *
            Matrix4x4::Scale({0.82f, 0.82f * 1.7f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            static_cast<float>(enemy.age) / 30.0f + static_cast<float>(part) * 0.37f, 1});
    }
    return true;
}

/**
 * @brief EASTSOURCEの破壊部位を専用デブリへ変換する
 * @param shooter 更新対象
 * @param enemy デブリ生成元
 * @param bossPart 破壊部位
 * @return 専用デブリを生成して共通デブリ生成を省略する場合true、共通デブリ生成を使用する場合false
 */
bool SideScrollingShooter::Stage5Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& enemy, int bossPart) {
    if (enemy.type != Stage::BossEnemy) return false;

    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };
    const EastsourcePartGroup detached = bossPart >= BossNose && bossPart <= BossRightEngine ?
        Groups[bossPart] : EastsourcePartGroup::Body;
    EastsourceModelState intact;
    const Stage5ModelTransform transform = EastsourceTransform(shooter, enemy);
    int pieceNumber = 0;

    // 破壊グループの実モデルパーツだけを既存の小型Debrisプールへ送る
    EastsourceModelView::VisitParts(transform, intact,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color,
            EastsourcePartGroup group) {
            if (group != detached) return;
            const Vector3 center = world.TransformPoint(Vector3::Zero);
            const float radius = (std::max)(0.24f,
                (std::min)(1.2f, Stage5ModelDetail::WorldPartRadius(world) * 0.55f));
            const float direction = center.x < ToWorldX(enemy.x) ? -1.0f : 1.0f;
            const float pieceColor[] = {color.r, color.g, color.b, color.a};
            const int debrisShape = shape == PrimitiveShape::Box ? 1 :
                (shape == PrimitiveShape::Cylinder ? 2 :
                    (shape == PrimitiveShape::Cone ? 3 :
                        (shape == PrimitiveShape::Prism ? 4 : 5)));
            shooter.SpawnDebrisPiece(center.x, center.y, center.z,
                direction * (0.035f + static_cast<float>(pieceNumber % 3) * 0.008f),
                0.018f + static_cast<float>(pieceNumber % 2) * 0.012f,
                -0.025f + static_cast<float>(pieceNumber % 3) * 0.018f,
                0.0f, direction * 0.10f, debrisShape,
                radius, radius * 0.65f, radius, pieceColor, 90, 64, false);
            ++pieceNumber;
        });
    return true;
}

/**
 * @brief Stage5ではStage4の約2倍へ拡大した専用ビル群を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawCityBuildings(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    if (ShooterStages::Stage5::IsRooftopPhase(shooter.m_stage5.phase)) return;

    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideHalfHeight =
        (SideBackgroundZ - SideScrollingShooterShared::SideCameraZ) *
        std::tan(Math::ToRadians(SideScrollingShooterShared::SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideHalfWidth = sideHalfHeight * renderer.AspectRatio();
    const float buildingScale = shooter.m_stageNumber == 5 ? Stage5CityBuildingScale : 1.0f;
    const int buildingCount = shooter.m_stageNumber == 5 ?
        Stage5CityBuildingCount : CityBuildingCount;
    const float buildingSpacing = shooter.m_stageNumber == 5 ?
        2.0f / Stage5CityBuildingCount : CityBuildingNdcSpacing;

    // Stage5は2倍モデル7種を1棟ずつ並べ、ファサード同士の重なりを防ぐ
    for (int index = 0; index < buildingCount; ++index) {
        const bool leftSide = index % 2 == 0;
        const float sideX = WrapCityNdcX(
            index * buildingSpacing - shooter.m_scroll * 0.18f) *
            (sideHalfWidth + 2.0f);
        const float sideWidth = 3.65f + static_cast<float>((index * 11) % 3) * 0.38f;
        const float sideHeight = 3.8f + static_cast<float>((index * 17) % 5) * 1.18f;
        const float railHeight = 9.0f + static_cast<float>((index * 17) % 5) * 2.4f;
        const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
        const float groundY = Math::Lerp(-6.0f, -3.65f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.7f,
            10.0f + WrapCityDistance(
                static_cast<float>(index * 29) - shooter.m_scroll * 36.0f, 100.0f),
            railWeight);
        const Stage5BuildingType building = static_cast<Stage5BuildingType>(
            index % static_cast<int>(Stage5BuildingType::Count));
        const Vector3 modelSize = Stage5CityModelView::ModelSize(building);
        const float railScale = railHeight * buildingScale / modelSize.y;
        const Vector3 modelScale {
            Math::Lerp(sideWidth * buildingScale / modelSize.x, railScale, railWeight),
            Math::Lerp(sideHeight * buildingScale / modelSize.y, railScale, railWeight),
            Math::Lerp(0.42f * buildingScale / modelSize.z, railScale, railWeight)
        };

        // 1部から2部へのムービー中は横道路と交差する市街ビルを除外する
        constexpr float CrossRoadNearZ = 24.0f;
        constexpr float CrossRoadFarZ = 42.0f;
        constexpr float CrossRoadClearance = 4.0f;
        const float modelHalfDepth = modelSize.z * modelScale.z * 0.5f;
        if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition &&
            z + modelHalfDepth > CrossRoadNearZ - CrossRoadClearance &&
            z - modelHalfDepth < CrossRoadFarZ + CrossRoadClearance) {
            continue;
        }

        const Matrix4x4 root = Matrix4x4::Translation({x, groundY, z}) *
            Matrix4x4::Scale(modelScale);

        // 既存の暗紺壁とシアン／マゼンタ系アクセントを保ったまま形状だけ差し替える
        Stage5CityModelView::VisitBuilding(building, root,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });

        // 密度を崩さないよう3棟ごとに一つだけ独立広告をマウントする
        if (index % 3 != 0) continue;
        const Stage5SignMount mount = Stage5CityModelView::SignMount(building, 0);
        const Matrix4x4 adWorld = root * Stage5ModelDetail::Matrix(mount.transform);
        const Stage5AdType ad = static_cast<Stage5AdType>(
            (index / 3) % static_cast<int>(Stage5AdType::Count));
        Stage5CityModelView::VisitAd(ad, adWorld,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });
    }
}

/**
 * @brief 第2部道中のNEO AIZU市街とPANDD会ビルを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawPart2Background(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    constexpr float GroundTopY = -3.65f;
    constexpr float BuildingZ = 45.0f;
    constexpr float EdgeColor[] = {0.20f, 0.34f, 0.48f, 1.0f};
    constexpr float CloudColor[] = {0.07f, 0.09f, 0.15f, 0.92f};
    const float weight = SmoothStep(Math::Clamp01(railWeight));
    const float moveWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f));
    const float expandWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f - 1.0f));
    const float sideHalfHeight =
        (SideBackgroundZ - SideScrollingShooterShared::SideCameraZ) *
        std::tan(Math::ToRadians(SideScrollingShooterShared::SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideHalfWidth = sideHalfHeight * renderer.AspectRatio();
    const int routeFrame = ShooterStages::Stage5::Part2RouteElapsedFrames(
        shooter.m_stage5.phase, shooter.m_stage5.phaseTimer);

    // 雲は2Dでは上空、3Dでは外壁の左右を上から下へ循環させる
    for (int cloud = 0; cloud < 4; ++cloud) {
        const float sideCloudX = Math::Lerp(-sideHalfWidth * 0.82f,
            sideHalfWidth * 0.62f, static_cast<float>(cloud) / 3.0f);
        const float sideCloudCycleHeight = sideHalfHeight * 2.2f;
        const float sideCloudY = -sideHalfHeight * 1.1f + WrapCityDistance(
            (static_cast<float>(cloud) + 0.5f) * sideCloudCycleHeight / 4.0f -
                static_cast<float>(routeFrame) *
                ShooterStages::Stage5::Part2SideSceneryFallSpeed,
            sideCloudCycleHeight);
        const float railCloudX = cloud < 2 ? -34.0f : 34.0f;
        constexpr float RailCloudBottomY = -10.0f;
        constexpr float RailCloudCycleHeight = 44.0f;
        const float railCloudY = RailCloudBottomY + WrapCityDistance(
            34.0f + static_cast<float>(cloud) * 11.0f -
                static_cast<float>(routeFrame) *
                ShooterStages::Stage5::Part2RailSceneryFallSpeed,
            RailCloudCycleHeight);
        const float railCloudZ = 58.0f + static_cast<float>(cloud) * 8.0f;
        for (int lobe = 0; lobe < 3; ++lobe) {
            const float lobeOffset = static_cast<float>(lobe - 1);
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                Math::Lerp(sideCloudX + lobeOffset * 0.72f,
                    railCloudX + lobeOffset * 2.6f, weight),
                Math::Lerp(sideCloudY + (lobe == 1 ? 0.22f : 0.0f),
                    railCloudY + (lobe == 1 ? 0.8f : 0.0f), weight),
                Math::Lerp(SidePlaneZ + 12.2f, railCloudZ, weight),
                Math::Lerp(lobe == 1 ? 1.8f : 1.35f, lobe == 1 ? 6.8f : 5.2f, weight),
                Math::Lerp(lobe == 1 ? 0.62f : 0.48f, lobe == 1 ? 2.2f : 1.6f, weight),
                Math::Lerp(0.24f, 2.8f, weight), CloudColor);
        }
    }

    // 2Dでは右三分の一、3Dでは画面中央奥へ同じ外壁を連続移動する
    const float sideBuildingWidth = sideHalfWidth * 0.68f;
    const float sideBuildingHeight = sideHalfHeight * 2.18f;
    const float sideBuildingX = sideHalfWidth - sideBuildingWidth * 0.5f;
    constexpr float RailBuildingBottomY = -36.0f;
    const float compactHeight = Math::Lerp(sideBuildingHeight, 32.0f, moveWeight);
    const float expandedHeight = ShooterStages::Stage5::PandDBuildingHeight +
        GroundTopY - RailBuildingBottomY;
    const float buildingHeight = Math::Lerp(compactHeight,
        expandedHeight, expandWeight);
    const float compactY = Math::Lerp(0.0f, GroundTopY + 16.0f, moveWeight);
    const float buildingY = Math::Lerp(compactY,
        RailBuildingBottomY + expandedHeight * 0.5f, expandWeight);
    const float buildingX = Math::Lerp(sideBuildingX, 0.0f, moveWeight);
    const float buildingZ = Math::Lerp(SidePlaneZ + 12.8f, BuildingZ, moveWeight);
    const float buildingWidth = Math::Lerp(sideBuildingWidth,
        ShooterStages::Stage5::PandDBuildingWidth, weight);
    const float buildingDepth = Math::Lerp(0.55f, 6.0f, moveWeight);
    // 超巨大部分は単純なビル躯体とし、新規モデルを縦に引き延ばさない
    shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
        buildingX, buildingY, buildingZ,
        buildingWidth, buildingHeight, buildingDepth, TowerFacadeColor);

    // PANDD会建造物は元の縦横比のまま超巨大ビルの屋上へ載せる
    const Vector3 capSize = TayamaModelView::TowerSize *
        ShooterStages::Stage5::PandDBuildingCapScale;
    const Matrix4x4 capRoot = TayamaModelView::BuildingRoot(
        buildingX, buildingY + buildingHeight * 0.5f, buildingZ, capSize);
    TayamaModelView::VisitParts(capRoot, 0.0f, TayamaModelState {},
        [&](PrimitiveShape shape, const Matrix4x4& world,
            const ColorF& color, TayamaPartGroup) {
            const float partColor[] = {color.r, color.g, color.b, color.a};
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(shape), world, partColor);
        });

    // 外壁両端の縦線で3D時に上方へ収束する輪郭を強調する
    for (int side = -1; side <= 1; side += 2) {
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            buildingX + static_cast<float>(side) * buildingWidth * 0.495f,
            buildingY, buildingZ - buildingDepth * 0.51f,
            Math::Lerp(0.12f, 0.42f, weight), buildingHeight,
            Math::Lerp(0.08f, 0.22f, weight), EdgeColor);
    }

    // 窓列を高速で上から下へ循環させ、外壁に沿った上昇速度を見せる
    constexpr int WindowRowCount = 12;
    const float sideWindowSpacing = sideHalfHeight * 0.24f;
    const float sideWindowScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2SideSceneryFallSpeed,
        sideWindowSpacing);
    constexpr float RailWindowSpacing = 12.0f;
    const float railWindowScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2RailSceneryFallSpeed,
        RailWindowSpacing);
    for (int row = 0; row < WindowRowCount; ++row) {
        for (int column = 0;
            column < ShooterStages::Stage5::PandDBuildingWindowColumns; ++column) {
            const float columnOffset =
                (static_cast<float>(column) /
                    static_cast<float>(ShooterStages::Stage5::PandDBuildingWindowColumns - 1) -
                    0.5f) * 0.75f;
            const float sideWindowX = sideBuildingX + columnOffset * sideBuildingWidth;
            const float sideWindowY = -sideHalfHeight - sideWindowSpacing +
                static_cast<float>(row) * sideWindowSpacing - sideWindowScroll;
            const float railWindowX = columnOffset *
                ShooterStages::Stage5::PandDBuildingWidth;
            const float railWindowY = GroundTopY - RailWindowSpacing +
                static_cast<float>(row) * RailWindowSpacing - railWindowScroll;
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                Math::Lerp(sideWindowX, railWindowX, weight),
                Math::Lerp(sideWindowY, railWindowY, weight),
                Math::Lerp(SidePlaneZ + 12.45f, 41.82f, weight),
                Math::Lerp(sideBuildingWidth * 0.10f,
                    ShooterStages::Stage5::PandDBuildingWidth * 0.08f, weight),
                Math::Lerp(0.32f, 0.52f, weight),
                Math::Lerp(0.08f, 0.18f, weight), BuildingWindowColor);
        }
    }
}

/**
 * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawStageWorld3D(const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    const bool wallFacade = shooter.m_stage5.phase == Stage5Phase::WallClimbTransition;
    if (wallFacade) {
        // 外壁上昇用の超巨大ビル躯体を道路へ接地する
        constexpr float GroundTopY = -3.65f;
        constexpr float BuildingZ = 45.0f;
        constexpr float CrossRoadWidth = 180.0f;
        constexpr float CrossRoadDepth = 18.0f;
        constexpr float CrossRoadCenterZ = BuildingZ - 3.0f - CrossRoadDepth * 0.5f;
        constexpr float CrossRoadSurfaceY = GroundTopY + 0.11f;
        constexpr float CrossRoadColor[] = {0.105f, 0.125f, 0.19f, 1.0f};
        constexpr float CrossRoadLaneColor[] = {0.72f, 0.84f, 0.88f, 1.0f};
        constexpr float CrossRoadCenterColor[] = {0.92f, 0.72f, 0.22f, 1.0f};

        // 既存の縦方向道路を進入路にし、ビル前面へ片側2車線の横道路を接続する
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, CrossRoadSurfaceY, CrossRoadCenterZ,
            CrossRoadWidth, 0.08f, CrossRoadDepth, CrossRoadColor);

        // 二重中央線と左右の破線で対向2車線ずつを示す
        for (int line = -1; line <= 1; line += 2) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                0.0f, CrossRoadSurfaceY + 0.045f,
                CrossRoadCenterZ + static_cast<float>(line) * 0.18f,
                CrossRoadWidth, 0.025f, 0.12f, CrossRoadCenterColor);
            for (int segment = 0; segment < 18; ++segment) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    -85.0f + static_cast<float>(segment) * 10.0f,
                    CrossRoadSurfaceY + 0.045f,
                    CrossRoadCenterZ + static_cast<float>(line) * 4.5f,
                    5.0f, 0.025f, 0.12f, CrossRoadLaneColor);
            }
        }

        // 手前路肩線は進入路幅だけ空けてT字路の接続口を示す
        constexpr float ApproachRoadWidth = 24.0f;
        constexpr float ShoulderHalfWidth =
            (CrossRoadWidth - ApproachRoadWidth) * 0.25f;
        for (int side = -1; side <= 1; side += 2) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(side) *
                    (ApproachRoadWidth * 0.5f + ShoulderHalfWidth),
                CrossRoadSurfaceY + 0.045f,
                CrossRoadCenterZ - CrossRoadDepth * 0.5f + 0.3f,
                ShoulderHalfWidth * 2.0f, 0.025f, 0.16f, CrossRoadLaneColor);
        }

        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, GroundTopY + ShooterStages::Stage5::PandDBuildingHeight * 0.5f,
            BuildingZ, ShooterStages::Stage5::PandDBuildingWidth,
            ShooterStages::Stage5::PandDBuildingHeight, 6.0f, TowerFacadeColor);

        // 接近と壁面上昇の全区間で見える窓を超巨大ビル正面へ規則配置する
        constexpr int WindowRowCount = 50;
        constexpr float WindowRowSpacing = 24.0f;
        for (int row = 0; row < WindowRowCount; ++row) {
            for (int column = 0;
                column < ShooterStages::Stage5::PandDBuildingWindowColumns; ++column) {
                const float columnOffset =
                    (static_cast<float>(column) /
                        static_cast<float>(
                            ShooterStages::Stage5::PandDBuildingWindowColumns - 1) -
                        0.5f) * 0.75f;
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    columnOffset * ShooterStages::Stage5::PandDBuildingWidth,
                    GroundTopY + WindowRowSpacing * (static_cast<float>(row) + 0.5f),
                    BuildingZ - 3.18f,
                    ShooterStages::Stage5::PandDBuildingWidth * 0.08f,
                    0.72f, 0.18f, BuildingWindowColor);
            }
        }

        // PANDD会建造物はスケールを均一に保ち屋上へ載せる
        const Vector3 capSize = TayamaModelView::TowerSize *
            ShooterStages::Stage5::PandDBuildingCapScale;
        const Matrix4x4 capRoot = TayamaModelView::BuildingRoot(
            0.0f, GroundTopY + ShooterStages::Stage5::PandDBuildingHeight,
            BuildingZ, capSize);
        TayamaModelView::VisitParts(capRoot, 0.0f, TayamaModelState {},
            [&](PrimitiveShape shape, const Matrix4x4& world,
                const ColorF& color, TayamaPartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });
        return;
    }

    // 第2部道中は背景側で同じ巨大ビルモデルを描画済み
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) return;

    if (shooter.m_stage5.phase <= Stage5Phase::EastsourceFall) {
        // 第1部遠景はラスボス変形前モデルではなく超巨大な市街ビルを配置する
        constexpr float GroundTopY = -3.65f;
        constexpr float MegaBuildingScale = 10.0f;
        constexpr float BackgroundSetback = 30.0f;
        const Stage5ModelTransform landmark = TayamaTransform(shooter);
        const Matrix4x4 root = Matrix4x4::Translation(
            {0.0f, GroundTopY, landmark.position.z + BackgroundSetback}) *
            Matrix4x4::Scale({MegaBuildingScale, MegaBuildingScale, MegaBuildingScale});
        Stage5CityModelView::VisitBuilding(Stage5BuildingType::TowerSub, root,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });
        return;
    }

    if (ShooterStages::Stage5::IsRooftopPhase(shooter.m_stage5.phase)) {
        constexpr float RoofColor[] = {0.075f, 0.09f, 0.13f, 1.0f};
        constexpr float RoofEdgeColor[] = {0.20f, 0.34f, 0.48f, 1.0f};
        constexpr float RoofLightColor[] = {0.08f, 0.72f, 1.0f, 1.0f};
        constexpr float RoofWidth = 144.0f;
        constexpr float RoofDepth = 150.0f;
        constexpr float RoofCenterZ = 48.0f;

        // 通常道路を覆う屋上床と外周壁で超巨大ビル上端を示す
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, ShooterStages::Stage5::RooftopSurfaceY - 0.35f, RoofCenterZ,
            RoofWidth, 0.7f, RoofDepth, RoofColor);
        for (int side = -1; side <= 1; side += 2) {
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(side) * (RoofWidth * 0.5f - 0.6f),
                ShooterStages::Stage5::RooftopSurfaceY + 0.45f, RoofCenterZ,
                1.2f, 1.6f, RoofDepth, RoofEdgeColor);
        }

        // 奥行き方向の誘導灯で広い屋上面とボスの設置位置を読ませる
        for (int marker = 0; marker < 9; ++marker) {
            const float z = 14.0f + static_cast<float>(marker) * 11.0f;
            for (int side = -1; side <= 1; side += 2) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    static_cast<float>(side) * 15.0f,
                    ShooterStages::Stage5::RooftopSurfaceY + 0.04f, z,
                    0.35f, 0.08f, 4.8f, RoofLightColor);
            }
        }
    }

    const Stage5ModelTransform transform = TayamaTransform(shooter);
    TayamaModelState state = TayamaState(shooter);
    const bool lightning = shooter.m_stage5.phase < Stage5Phase::TayamaCommandCore &&
        ((shooter.m_frame % 241) < 3 || ((shooter.m_frame + 73) % 389) < 2);
    if (lightning) {
        for (bool& flash : state.hitFlash) flash = true;
    }

    // 同じ46パーツをビル端点から巨大メカ端点まで補間して描画する
    TayamaModelView::VisitParts(transform, shooter.m_stage5.tayamaTransformation, state,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, TayamaPartGroup) {
            const float brightness = lightning ? 1.55f : 1.0f;
            const float partColor[] = {
                (std::min)(1.0f, color.r * brightness),
                (std::min)(1.0f, color.g * brightness),
                (std::min)(1.0f, color.b * brightness), color.a
            };
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
        });

    // 変形終盤から既存のエンジン炎HLSLを背部と生存中の脚部機関へ付ける
    if (shooter.m_stage5.phase >= Stage5Phase::CarrierTransformation &&
        shooter.m_stage5.phase < Stage5Phase::TayamaCollapse) {
        constexpr TayamaPartGroup EngineGroups[] = {
            TayamaPartGroup::MainThruster,
            TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine
        };
        for (int engine = 0; engine < 3; ++engine) {
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                shooter.m_stage5.tayamaTransformation, state, EngineGroups[engine]);
            if (!bounds.valid) continue;
            const float width = engine == 0 ? 2.2f : 1.25f;
            const Matrix4x4 flameWorld = Matrix4x4::Translation(
                bounds.center + Vector3 {0.0f, -1.0f - static_cast<float>(engine) * 0.12f, 0.0f}) *
                Matrix4x4::Scale({width, 2.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * flameWorld,
                static_cast<float>((shooter.m_frame + engine * 7) % 24) / 24.0f, 3});
        }
    }

    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        // 破裂前は赤色警告灯と左右へ押し出される格納庫装甲を段階表示する
        const float warningColor[] = {1.0f, 0.04f, 0.03f,
            (shooter.m_stage5.phaseTimer / 5) % 2 == 0 ? 1.0f : 0.28f};
        for (int light = -2; light <= 2; ++light) {
            shooter.DrawModelPrimitive(renderer, camera, 1,
                static_cast<float>(light) * 1.35f, 1.5f, 61.5f,
                0.34f, 0.18f, 0.16f, warningColor);
        }
        const float deformation = SmoothStep(Math::Clamp01(
            static_cast<float>(shooter.m_stage5.phaseTimer - 18) / 40.0f));
        shooter.DrawModelPrimitive(renderer, camera, 1, -1.5f - deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor,
            0.0f, deformation * 0.16f);
        shooter.DrawModelPrimitive(renderer, camera, 1, 1.5f + deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor, 0.0f, -deformation * 0.16f);
    }

    // 嵐の上層とCOMMAND CORE以降の雲海を少数のCube帯で表現する
    const bool aboveStorm = shooter.m_stage5.phase >= Stage5Phase::TayamaCommandCore;
    const int cloudCount = aboveStorm ? 22 : 14;
    for (int i = 0; i < cloudCount; ++i) {
        const float x = -34.0f + static_cast<float>((i * 47) % 680) / 10.0f;
        const float z = 24.0f + static_cast<float>((i * 31 + shooter.m_frame / 3) % 760) / 10.0f;
        const float y = aboveStorm ? -5.8f + static_cast<float>(i % 3) * 0.32f :
            12.0f + static_cast<float>(i % 4) * 1.1f;
        const float cloudColor[] = {
            aboveStorm ? 0.32f : StormCloudColor[0],
            aboveStorm ? 0.38f : StormCloudColor[1],
            aboveStorm ? 0.48f : StormCloudColor[2],
            aboveStorm ? 0.82f : 0.72f
        };
        shooter.DrawModelPrimitive(renderer, camera, 1, x, y, z,
            8.0f + static_cast<float>(i % 4) * 2.0f, 0.75f, 3.5f, cloudColor);
    }

    // 現フェーズのサーチライト基部と、追尾上限を持つ光軸を同じ座標で描画する
    int activeLights = 0;
    bool tayamaLights = false;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbLower) activeLights = 1;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle) activeLights = 2;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbUpper) activeLights = 3;
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
        activeLights = 2;
        tayamaLights = true;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceBattle && shooter.m_enemies[0].active &&
        shooter.m_enemies[0].bossPhase >= BossNormalPhase2 && shooter.m_enemies[0].age % 180 < 90) {
        activeLights = 1;
    }
    for (int index = 0; index < activeLights; ++index) {
        const SearchlightState& light = shooter.m_stage5.searchlights[index];
        if (light.destroyed) continue;
        Vector3 source {
            ToWorldX((static_cast<float>(index) - 1.0f) * 0.72f),
            ToWorldY(0.72f - static_cast<float>(index) * 0.22f),
            tayamaLights ? 57.0f : 46.0f
        };
        if (tayamaLights) {
            const TayamaPartGroup group = index == 0 ?
                TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
                transform, shooter.m_stage5.tayamaTransformation, state, group);
            if (bounds.valid) source = bounds.center;
        }
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector3 target {
            ToWorldX(locked ? light.lockedX : light.beamX),
            ToWorldY(locked ? light.lockedY : light.beamY), PlayerRailZ
        };
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float* beamColor = locked ? SearchlightLockedColor : SearchlightColor;
        const Matrix4x4 beamWorld = Matrix4x4::Translation(source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, locked ? 0.12f : 0.18f, locked ? 0.12f : 0.18f});
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box), beamWorld, beamColor);
        shooter.DrawModelPrimitive(renderer, camera, 2, source.x, source.y, source.z,
            0.72f, 0.42f, 0.72f, locked ? SearchlightLockedColor : SatelliteLightColor);
    }

    // 有効弱点へ小さな発光リングを重ねて攻略対象を明示する
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        constexpr TayamaPartGroup Groups[] = {
            TayamaPartGroup::LeftSearchlight, TayamaPartGroup::RightSearchlight,
            TayamaPartGroup::FireControlRadar, TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine, TayamaPartGroup::CommandCore
        };
        for (const TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
            if (!weakpoint.active || weakpoint.destroyed) continue;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                shooter.m_stage5.tayamaTransformation, state, Groups[static_cast<std::size_t>(weakpoint.type)]);
            if (!bounds.valid) continue;
            const float size = (std::max)(0.75f, (std::min)(2.2f, bounds.radius * 0.45f));
            const Matrix4x4 world = Matrix4x4::Translation(bounds.center + Vector3 {0.0f, 0.0f, -0.12f}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                static_cast<float>(shooter.m_frame % 30) / 30.0f, 0});
        }
    }

    // 最終フェーズの胸部コアへ射撃予告と同期した収束光を重ねる
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore &&
        shooter.m_stage5.phaseTimer > 75 &&
        shooter.m_stage5.attackTimer % 180 < 42) {
        const Stage5GroupBounds core = TayamaModelView::GroupBounds(transform,
            shooter.m_stage5.tayamaTransformation, state, TayamaPartGroup::CommandCore);
        if (core.valid) {
            const float charge = static_cast<float>(shooter.m_stage5.attackTimer % 180) / 42.0f;
            const Matrix4x4 chargeWorld = Matrix4x4::Translation(
                core.center + Vector3 {0.0f, 0.0f, -1.0f}) *
                Matrix4x4::Scale({1.0f + charge * 1.8f,
                    1.0f + charge * 1.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * chargeWorld,
                charge, 0});
        }
    }

    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse && shooter.m_stage5.phaseTimer >= 330 &&
        shooter.m_stage5.phaseTimer < TayamaCollapseFrames) {
        // 最終90フレームは内部白光と二重衝撃波で輪郭ごと消滅させる
        const float finalProgress = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 450) / 90.0f);
        const float glow = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 330) / 120.0f);
        const Matrix4x4 glowWorld = Matrix4x4::Translation(transform.position) *
            Matrix4x4::Scale({3.0f + glow * 8.0f, 2.0f + glow * 5.0f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * glowWorld,
            static_cast<float>(shooter.m_frame % 24) / 24.0f, 0});
        if (finalProgress > 0.0f) {
            const Matrix4x4 shockwave = Matrix4x4::Translation(transform.position) *
                Matrix4x4::Scale({4.0f + finalProgress * 24.0f,
                    4.0f + finalProgress * 24.0f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * shockwave,
                finalProgress, 0});
        }
    }
}

/**
 * @brief Stage 5の雨粒を3D空間へ描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawRain3D(const SideScrollingShooter& shooter,
    Renderer& renderer, const Camera3D& camera, float railWeight) {
    const float intensity = RainIntensity(shooter.m_stage5.phase, shooter.m_chapterNumber,
        shooter.m_stage5.tayamaTransformation, shooter.m_stage5.phaseTimer);
    const int rainCount = intensity > 0.0f ? static_cast<int>(32.0f + intensity * 64.0f) : 0;
    const int frame = shooter.m_frame;
    const float weight = Math::Clamp01(railWeight);

    // 横視点は前景、中景、後景の3層とし、遷移に合わせて3D降雨域へ展開する
    for (int index = 0; index < rainCount; ++index) {
        constexpr float SideGroundY = -6.0f;
        constexpr float RailGroundY = -3.65f;
        const int fallPhase = RainFallPhase(index, frame);
        const float fallRate = static_cast<float>(fallPhase) / static_cast<float>(RainCycle);
        const int sideColumn = (index * 137 + frame * 2) % 340;
        const float streakLength = 0.80f + static_cast<float>(index % 5) * 0.18f;
        const float railStreakLength = streakLength * 1.55f;
        const float sideZ = SidePlaneZ - 3.0f + static_cast<float>(index % 3) * 4.0f;
        const Vector3 sidePosition {
            -17.0f + static_cast<float>(sideColumn) * 0.1f,
            SideGroundY + streakLength * 0.5f + fallRate * 15.0f,
            sideZ
        };

        const int railColumn = (index * 137 + frame * 2) % 400;
        const float railZ = -2.0f + static_cast<float>((index * 197) % 960) * 0.1f;
        const Vector3 railPosition {
            -20.0f + static_cast<float>(railColumn) * 0.1f,
            RailGroundY + railStreakLength * 0.5f + fallRate * 24.0f,
            railZ
        };

        const float thickness = 0.045f + intensity * 0.025f;
        const Vector3 scale {
            thickness,
            Math::Lerp(streakLength, railStreakLength, weight),
            thickness
        };
        const Vector3 rotation {Math::Lerp(0.0f, 0.08f, weight), 0.0f,
            -0.18f - static_cast<float>(index % 3) * 0.025f};
        const float rainColor[] = {0.50f, 0.72f, 0.90f, 0.16f + intensity * 0.34f};
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            Vector3::Lerp(sidePosition, railPosition, weight), scale, rotation, rainColor);

        // 落下位相が地面を折り返した直後だけ小さなBox雨粒を跳ね上げる
        const float splashProgress = RainSplashProgress(fallPhase);
        if (splashProgress < 0.0f) continue;
        const int framesSinceImpact =
            ((RainCycle - fallPhase) % RainCycle + RainFallSpeed - 1) / RainFallSpeed;
        const int sideImpactColumn =
            (sideColumn + 340 - framesSinceImpact * 2) % 340;
        const int railImpactColumn =
            (railColumn + 400 - framesSinceImpact * 2) % 400;
        const float arc = std::sin(splashProgress * Math::Pi);
        const float particleSize = Math::Lerp(0.09f, 0.035f, splashProgress);
        const float splashColor[] = {
            0.58f, 0.78f, 0.96f, intensity * (1.0f - splashProgress) * 0.62f
        };
        for (int particle = 0; particle < 4; ++particle) {
            const float direction = static_cast<float>(particle) - 1.5f;
            const Vector3 sideSplash {
                -17.0f + static_cast<float>(sideImpactColumn) * 0.1f +
                    direction * splashProgress * 0.24f,
                SideGroundY + particleSize * 0.5f + arc *
                    (0.28f + static_cast<float>(particle % 2) * 0.12f),
                sideZ + (particle % 2 == 0 ? -1.0f : 1.0f) * splashProgress * 0.16f
            };
            const Vector3 railSplash {
                -20.0f + static_cast<float>(railImpactColumn) * 0.1f +
                    direction * splashProgress * 0.42f,
                RailGroundY + particleSize * 0.5f + arc *
                    (0.42f + static_cast<float>(particle % 2) * 0.18f),
                railZ + direction * splashProgress * 0.28f
            };
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                Vector3::Lerp(sideSplash, railSplash, weight),
                {particleSize, particleSize, particleSize}, {}, splashColor);
        }
    }

}

/**
 * @brief Stage 5の稲光と照準表示を画面空間へ描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawScreenEffects(const SideScrollingShooter& shooter, Renderer& renderer) {
    const float intensity = RainIntensity(shooter.m_stage5.phase, shooter.m_chapterNumber,
        shooter.m_stage5.tayamaTransformation, shooter.m_stage5.phaseTimer);

    // 稲光はTAYAMAの輪郭と警告灯を一瞬だけ強調する
    if (intensity > 0.30f && ((shooter.m_frame % 241) < 3 || ((shooter.m_frame + 73) % 389) < 2)) {
        const float alpha = (shooter.m_frame % 2 == 0 ? 0.30f : 0.16f) * intensity;
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.72f, 0.82f, 1.0f, alpha});
    }

    // 撃破地点、壁面ムービー、第2部開始を暗転でつなぐ
    const float fadeAlpha = SmoothStep(ShooterStages::Stage5::CinematicFadeAlpha(
        shooter.m_stage5.phase, shooter.m_stage5.phaseTimer));
    if (fadeAlpha > 0.0f) {
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}},
            {0.0f, 0.0f, 0.0f, fadeAlpha});
    }
    if (IsCinematic(shooter)) return;

    // 胸部掃射、脚部斉射、コアレーザーは発射前だけ危険範囲を固定表示する
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phaseTimer > 75 && shooter.m_stage5.attackTimer % 210 < 36) {
        renderer.Draw(Rect {{0.0f, shooter.m_stage5.coreTargetY}, {1.86f, 0.055f}},
            {1.0f, 0.12f, 0.08f, 0.36f});
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines &&
        shooter.m_stage5.phaseTimer > 75 && shooter.m_stage5.attackTimer % 132 < 32) {
        renderer.Draw(Rect {{shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY}, {0.38f, 0.075f}},
            {1.0f, 0.34f, 0.08f, 0.42f});
        renderer.Draw(Circle {{shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY}, 0.12f},
            {1.0f, 0.58f, 0.12f, 0.58f});
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore &&
        shooter.m_stage5.phaseTimer > 75 && shooter.m_stage5.attackTimer % 180 < 42) {
        const Vector2 target {shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY};
        renderer.Draw(Circle {target, 0.11f}, {1.0f, 0.08f, 0.04f, 0.62f});
    }

    // 検出円と固定ロック地点を表示し、光軸がロック後に追尾しないことを示す
    const bool eastsourceSearchlight = shooter.m_stage5.phase == Stage5Phase::EastsourceBattle &&
        shooter.m_enemies[0].active && shooter.m_enemies[0].bossPhase >= BossNormalPhase2 &&
        shooter.m_enemies[0].age % 180 < 90;
    const bool showSearchlights = eastsourceSearchlight ||
        (shooter.m_stage5.phase >= Stage5Phase::WallClimbLower &&
            shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper) ||
        shooter.m_stage5.phase == Stage5Phase::TayamaFireControl;
    if (!showSearchlights) return;
    for (const SearchlightState& light : shooter.m_stage5.searchlights) {
        if (light.destroyed || light.phase == SearchlightPhase::Cooldown) continue;
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector2 target {locked ? light.lockedX : light.beamX,
            locked ? light.lockedY : light.beamY};
        const bool detecting = light.phase == SearchlightPhase::Detecting;
        const ColorF color = locked ? ColorF {1.0f, 0.08f, 0.08f, 0.86f} :
            (detecting ? ColorF {1.0f, 0.78f, 0.18f, 0.34f} :
                ColorF {0.92f, 0.82f, 0.42f, 0.16f});
        renderer.Draw(Circle {target, locked ? 0.075f : SearchlightDetectionRadius}, color);
        if (locked) {
            renderer.Draw(Rect {{target.x - 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x + 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x, target.y - 0.10f}, {0.008f, 0.055f}}, color);
            renderer.Draw(Rect {{target.x, target.y + 0.10f}, {0.008f, 0.055f}}, color);
        }
    }
}

/**
 * @brief Stage 5専用HUDを描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawStage5Hud(const SideScrollingShooter& shooter, Renderer& renderer) {
    if (shooter.m_stage5.phase == Stage5Phase::Approach ||
        shooter.m_stage5.phase == Stage5Phase::TayamaCollapse ||
        shooter.m_stage5.phase == Stage5Phase::EndingReady) return;
    constexpr float Back[] = {0.08f, 0.05f, 0.12f, 0.90f};
    constexpr float Fill[] = {0.96f, 0.14f, 0.24f, 1.0f};
    constexpr float Accent[] = {0.16f, 0.82f, 1.0f, 1.0f};
    constexpr float Divider[] = {1.0f, 0.82f, 0.30f, 1.0f};
    constexpr float BarWidth = 0.62f;

    if (shooter.m_stage5.phase == Stage5Phase::EastsourceBattle) {
        const float hpRate = Math::Clamp01(shooter.m_displayBossHp / static_cast<float>(EastsourceMaxHp));
        shooter.DrawShape(renderer, 0.0f, 0.76f, BarWidth, 0.025f, Back);
        shooter.DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.76f,
            BarWidth * hpRate, 0.018f, Fill);
        shooter.DrawBossPhaseDividers(
            renderer, 0.755f, BarWidth, EastsourceMaxHp, Divider);
        renderer.DrawText("EASTSOURCE", TextAlign::Center, 0.017f,
            {1.0f, 0.42f, 0.55f, 1.0f}, {0.0f, 0.86f});
        constexpr const char* Labels[] = {"PRECISION", "CROSSFIRE", "PURSUIT", "LAST CONTRACT"};
        const int phase = shooter.m_enemies[0].active ? shooter.m_enemies[0].bossPhase : 0;
        renderer.DrawText(Labels[(std::clamp)(phase, 0, 3)], {-BarWidth, 0.81f}, 0.012f,
            {1.0f, 0.82f, 0.30f, 1.0f});
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        renderer.DrawText("HOSTILE SIGNAL APPROACHING", TextAlign::Center, 0.018f,
            {1.0f, 0.34f, 0.32f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        renderer.DrawText("SIGNAL LOST", TextAlign::Center, 0.030f,
            {1.0f, 0.18f, 0.18f, 1.0f}, {0.0f, 0.12f});
        return;
    }
    if (shooter.m_stage5.phase >= Stage5Phase::WallClimbTransition &&
        shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper) {
        const char* section = shooter.m_stage5.phase <= Stage5Phase::WallClimbLower ?
            "WALL CLIMB: LOWER" : (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle ?
                "WALL CLIMB: MIDDLE" : "WALL CLIMB: UPPER");
        renderer.DrawText(section, TextAlign::Center, 0.017f,
            {0.85f, 0.94f, 1.0f, 1.0f}, {0.0f, 0.84f});
        char status[40];
        int remaining = 0;
        for (const SearchlightState& light : shooter.m_stage5.searchlights) if (!light.destroyed) ++remaining;
        std::snprintf(status, sizeof(status), "SEARCHLIGHTS ACTIVE %d", remaining);
        renderer.DrawText(status, TextAlign::Center, 0.012f,
            {1.0f, 0.74f, 0.20f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        char status[48];
        std::snprintf(status, sizeof(status), "GIANT MECHA TAYAMA  %03d%%",
            static_cast<int>(shooter.m_stage5.tayamaTransformation * 100.0f));
        renderer.DrawText(status, TextAlign::Center, 0.018f,
            {0.30f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.82f});
        return;
    }

    // TAYAMA戦は現在フェーズの有効弱点HP合計だけを表示する
    int maxHp = 0;
    for (const TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        if (IsTayamaWeakpointActiveForPhase(weakpoint.type, shooter.m_stage5.phase)) {
            maxHp += weakpoint.maxHp;
        }
    }
    const float hpRate = maxHp > 0 ? Math::Clamp01(shooter.m_displayBossHp / static_cast<float>(maxHp)) : 0.0f;
    shooter.DrawShape(renderer, 0.0f, 0.74f, BarWidth, 0.025f, Back);
    shooter.DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.74f,
        BarWidth * hpRate, 0.018f, Accent);
    renderer.DrawText("GIANT MECHA", TextAlign::Center, 0.011f,
        {0.65f, 0.82f, 0.90f, 1.0f}, {0.0f, 0.88f});
    renderer.DrawText("TAYAMA", TextAlign::Center, 0.022f,
        {0.20f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.83f});
    const char* phase = shooter.m_stage5.phase == Stage5Phase::TayamaFireControl ? "PHASE: TARGETING ARRAY" :
        (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines ?
            "PHASE: SIEGE LIMBS" : "PHASE: CORE OVERDRIVE");
    renderer.DrawText(phase, {-BarWidth, 0.79f}, 0.012f,
        {1.0f, 0.82f, 0.30f, 1.0f});
    char components[96];
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
        std::snprintf(components, sizeof(components), "L-LIGHT[%c]  R-LIGHT[%c]  RADAR[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed ? 'X' : ' ');
    } else if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines) {
        std::snprintf(components, sizeof(components), "L-LEG[%c]  R-LEG[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed ? 'X' : ' ');
    } else {
        std::snprintf(components, sizeof(components), "COMMAND CORE[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed ? 'X' : ' ');
    }
    renderer.DrawText(components, TextAlign::Center, 0.010f,
        {0.72f, 0.86f, 0.92f, 1.0f}, {0.0f, 0.68f});
}
