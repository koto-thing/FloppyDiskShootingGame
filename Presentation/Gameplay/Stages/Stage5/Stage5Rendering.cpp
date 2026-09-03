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

namespace {
constexpr float TowerFacadeColor[4] = { 0.10f, 0.13f, 0.24f, 1.0f };
constexpr float SatelliteLightColor[4] = { 0.82f, 0.94f, 1.0f, 1.0f };
constexpr float SearchlightColor[4] = { 1.00f, 0.82f, 0.20f, 0.24f };
constexpr float SearchlightLockedColor[4] = { 1.00f, 0.08f, 0.08f, 0.50f };
constexpr float StormCloudColor[4] = { 0.05f, 0.07f, 0.13f, 1.0f };
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
    // 壁面上昇だけ視線を上へ向け、屋上で水平へ戻す
    if (shooter.m_stage5.phase >= Stage5Phase::WallClimbTransition) {
        float climbWeight = 0.0f;
        if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
            climbWeight = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer) /
                static_cast<float>(WallClimbTransitionFrames));
        } else if (shooter.m_stage5.phase == Stage5Phase::WallClimbLower) {
            climbWeight = 0.42f;
        } else if (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle) {
            climbWeight = 0.68f;
        } else if (shooter.m_stage5.phase == Stage5Phase::WallClimbUpper) {
            climbWeight = 0.92f;
        } else if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival) {
            climbWeight = 1.0f - SmoothStep(Math::Clamp01(
                static_cast<float>(shooter.m_stage5.phaseTimer) / RooftopArrivalFrames));
        }
        railPosition.y -= climbWeight * 1.8f;
        railTarget.y += climbWeight * 13.0f;
        railTarget.z += climbWeight * 8.0f;
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
 * @brief EASTSOURCE専用モデルを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param enemy 描画する敵
 * @return 専用モデルを描画して共通ボスモデルを省略する場合true、共通ボスモデルを使用する場合false
 */
bool SideScrollingShooter::Stage5Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy) {
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
 * @brief 既存都市背景と同じ配置と寸法でStage5専用ビル群を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawCityBuildings(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideHalfHeight =
        (SideBackgroundZ - SideScrollingShooterShared::SideCameraZ) *
        std::tan(Math::ToRadians(SideScrollingShooterShared::SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideHalfWidth = sideHalfHeight * renderer.AspectRatio();

    // 旧30棟と同じ循環位置、接地面、幅、高さ、奥行きをモデル全体の外形へ適用する
    for (int index = 0; index < CityBuildingCount; ++index) {
        const bool leftSide = index % 2 == 0;
        const float sideX = WrapCityNdcX(
            index * CityBuildingNdcSpacing - shooter.m_scroll * 0.18f) *
            (sideHalfWidth + 2.0f);
        const float sideWidth = 3.65f + static_cast<float>((index * 11) % 3) * 0.38f;
        const float sideHeight = 3.8f + static_cast<float>((index * 17) % 5) * 1.18f;
        const float railWidth = 4.2f + static_cast<float>(index % 3) * 0.8f;
        const float railHeight = 9.0f + static_cast<float>((index * 17) % 5) * 2.4f;
        const float width = Math::Lerp(sideWidth, railWidth, railWeight);
        const float height = Math::Lerp(sideHeight, railHeight, railWeight);
        const float depth = Math::Lerp(0.42f, 7.0f, railWeight);
        const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
        const float groundY = Math::Lerp(-6.0f, -3.65f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.7f,
            10.0f + WrapCityDistance(
                static_cast<float>(index * 29) - shooter.m_scroll * 36.0f, 100.0f),
            railWeight);
        const Stage5BuildingType building = static_cast<Stage5BuildingType>(
            index % static_cast<int>(Stage5BuildingType::Count));
        const Vector3 modelSize = Stage5CityModelView::ModelSize(building);
        const Matrix4x4 root = Matrix4x4::Translation({x, groundY, z}) *
            Matrix4x4::Scale({width / modelSize.x, height / modelSize.y, depth / modelSize.z});

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
 * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawStageWorld3D(const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    const Stage5ModelTransform transform = TayamaTransform(shooter);
    TayamaModelState state = TayamaState(shooter);
    const bool lightning = shooter.m_stage5.phase < Stage5Phase::TayamaCommandCore &&
        ((shooter.m_frame % 241) < 3 || ((shooter.m_frame + 73) % 389) < 2);
    if (lightning) {
        for (bool& flash : state.hitFlash) flash = true;
    }

    // 同じ46パーツをビル端点から空母端点まで補間して描画する
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

    // 変形終盤から既存のエンジン炎HLSLを主推進機と生存中の揚力機関へ付ける
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

    // 甲板掃射、排気レーン、コアレーザーは発射前だけ危険範囲を固定表示する
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl && shooter.m_stage5.attackTimer % 210 < 36) {
        renderer.Draw(Rect {{0.0f, shooter.m_stage5.coreTargetY}, {1.86f, 0.055f}},
            {1.0f, 0.12f, 0.08f, 0.36f});
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines && shooter.m_stage5.attackTimer % 132 < 32) {
        renderer.Draw(Rect {{shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY}, {0.38f, 0.075f}},
            {1.0f, 0.34f, 0.08f, 0.42f});
        renderer.Draw(Circle {{shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY}, 0.12f},
            {1.0f, 0.58f, 0.12f, 0.58f});
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore && shooter.m_stage5.attackTimer % 180 < 42) {
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
        const int maxHp = shooter.m_enemies[0].maxHp > 0 ? shooter.m_enemies[0].maxHp : EastsourceMaxHp;
        const float hpRate = Math::Clamp01(shooter.m_displayBossHp / static_cast<float>(maxHp));
        shooter.DrawShape(renderer, 0.0f, 0.76f, BarWidth, 0.025f, Back);
        shooter.DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.76f,
            BarWidth * hpRate, 0.018f, Fill);
        shooter.DrawBossPhaseDividers(
            renderer, 0.755f, BarWidth, maxHp, Divider);
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
        std::snprintf(status, sizeof(status), "MOBILE FORTRESS TAYAMA  %03d%%",
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
    renderer.DrawText("MOBILE FORTRESS", TextAlign::Center, 0.011f,
        {0.65f, 0.82f, 0.90f, 1.0f}, {0.0f, 0.88f});
    renderer.DrawText("TAYAMA", TextAlign::Center, 0.022f,
        {0.20f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.83f});
    const char* phase = shooter.m_stage5.phase == Stage5Phase::TayamaFireControl ? "PHASE: FIRE CONTROL" :
        (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines ?
            "PHASE: LIFT ENGINES" : "PHASE: COMMAND CORE");
    renderer.DrawText(phase, {-BarWidth, 0.79f}, 0.012f,
        {1.0f, 0.82f, 0.30f, 1.0f});
    char components[96];
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
        std::snprintf(components, sizeof(components), "L-LIGHT[%c]  R-LIGHT[%c]  RADAR[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed ? 'X' : ' ');
    } else if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines) {
        std::snprintf(components, sizeof(components), "L-ENGINE[%c]  R-ENGINE[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed ? 'X' : ' ');
    } else {
        std::snprintf(components, sizeof(components), "COMMAND CORE[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed ? 'X' : ' ');
    }
    renderer.DrawText(components, TextAlign::Center, 0.010f,
        {0.72f, 0.86f, 0.92f, 1.0f}, {0.0f, 0.68f});
}
