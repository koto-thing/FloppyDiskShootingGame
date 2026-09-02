#include "Stage3Module.h"

#include <algorithm>
#include <cmath>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../../SideScrollingShooterShared.h"
#include "../Common/StageDefinition.h"

namespace {
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::SideCameraZ;

constexpr float DaySkyColor[4] = {0.30f, 0.68f, 0.92f, 1.0f};
constexpr float NightSkyColor[4] = {0.015f, 0.03f, 0.12f, 1.0f};
constexpr float WaterColor[4] = {0.04f, 0.34f, 0.60f, 1.0f};
constexpr float WaveColor[4] = {0.20f, 0.74f, 0.86f, 1.0f};
constexpr float FoamColor[4] = {0.78f, 0.94f, 0.92f, 1.0f};
constexpr float CloudColor[4] = {0.90f, 0.95f, 0.96f, 1.0f};
constexpr float SunColor[4] = {1.00f, 0.82f, 0.20f, 1.0f};
constexpr float SeaSerpentColor[4] = {0.05f, 0.24f, 0.20f, 1.0f};
constexpr float SeaSerpentBellyColor[4] = {0.28f, 0.62f, 0.48f, 1.0f};
constexpr float SeaSerpentEyeColor[4] = {1.00f, 0.84f, 0.16f, 1.0f};
constexpr float SeaSerpentSideEyeSurfaceOffset = 0.90f;
constexpr float SeaSerpentRailEyeSurfaceOffset = 1.70f;
constexpr int DawnStartFrame = 500;
constexpr int DawnFrame = 750;
static_assert(SeaSerpentSideEyeSurfaceOffset > 1.35f * 1.25f * 0.5f);
static_assert(SeaSerpentRailEyeSurfaceOffset > 2.50f * 1.25f * 0.5f);
static_assert(DawnStartFrame < DawnFrame);

}

struct SideScrollingShooter::Stage3Module::SeaSerpentMotion {
    int segmentCount;
    float progress;
    float direction;
    float sideOriginX;
    float railOriginX;
    float railOriginZ;
    float railDirection;
    float railTravel;
    float elevation;
    float travel;
    float segmentSpacing;
    float segmentDelay;
    float scale;
};

struct SideScrollingShooter::Stage3Module::SeaSerpentSegment {
    float progress;
    float elevation;
    float sideX;
    float railX;
    float railZ;
    float scale;
};

/** @brief Stage 3の敵出現とボス弾幕を定義する */
class SideScrollingShooter::Stage3Module::StageDefinitionImpl final : public SideScrollingShooter::Stage {
public:
    /**
     * @brief ステージ番号を取得する
     * @return Stage 3を表す番号
     */
    int StageIndex() const override {
        return 3;
    }

    /**
     * @brief Stage 3の経過フレームから通常敵の出現を選択する
     * @param frame Stage 3開始からの経過フレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {3, 24, 54, 1.10f, -0.82f, 0.88f, 50.0f},
            {4, 110, 250, 1.12f, -0.40f, -0.42f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {3, 10, 52, 1.10f, 0.28f, -0.88f, 40.0f},
            {5, 70, 165, 1.14f, 0.82f, 0.32f, 56.0f},
            {1, 150, 230, 1.16f, 0.05f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {4, 10, 140, 1.12f, -0.85f, -0.18f, 60.0f},
            {5, 60, 145, 1.14f, -0.28f, 0.86f, 50.0f},
            {1, 120, 180, 1.16f, 0.82f, -0.68f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }

    /**
     * @brief Stage 3ボスの一斉射撃数を取得する
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 5;
    }

    /**
     * @brief Stage 3ボスの指定番号の弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.024f, -0.020f},
            {-0.12f, 0.0f, -0.026f, -0.010f},
            {-0.12f, 0.0f, -0.027f, 0.0f},
            {-0.12f, 0.0f, -0.026f, 0.010f},
            {-0.12f, 0.0f, -0.024f, 0.020f}
        };
        constexpr BossBullet RailPattern[] = {
            {0.0f, 0.0f, -0.014f, -0.024f},
            {0.0f, 0.0f, -0.007f, -0.012f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.007f, 0.012f},
            {0.0f, 0.0f, 0.014f, 0.024f}
        };
        return (railMode ? RailPattern : SidePattern)[index % 5];
    }
};

const SideScrollingShooter::Stage& SideScrollingShooter::Stage3Module::Definition() {
    static const StageDefinitionImpl definition;
    return definition;
}

float SideScrollingShooter::Stage3Module::WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

float SideScrollingShooter::Stage3Module::NightBlend(int frame) {
    return 1.0f - Math::Clamp01(static_cast<float>(frame - DawnStartFrame) /
        static_cast<float>(DawnFrame - DawnStartFrame));
}

bool SideScrollingShooter::Stage3Module::GetSeaSerpentMotion(
    int frame, SeaSerpentMotion& motion) {
    constexpr int CycleFrames = 420;
    constexpr int Duration[] = {112, 138, 172};
    constexpr int Segments[] = {14, 16, 23};
    constexpr float Elevation[] = {8.2f, 6.4f, 12.0f};
    constexpr float Travel[] = {12.0f, 17.0f, 14.0f};
    constexpr float Spacing[] = {0.90f, 0.78f, 1.12f};
    constexpr float Delay[] = {0.055f, 0.042f, 0.050f};
    constexpr float Scale[] = {1.70f, 2.00f, 3.20f};
    static_assert(Segments[2] > Segments[0] &&
        Elevation[2] > Elevation[0] && Scale[2] > Scale[0]);
    const int cycle = frame / CycleFrames;
    const int action = cycle % 3;
    const int cycleFrame = frame % CycleFrames;
    const int startFrame = 48 + (cycle * 97) % 170;
    if (cycleFrame < startFrame || cycleFrame >= startFrame + Duration[action]) {
        return false;
    }

    motion.segmentCount = Segments[action];
    motion.progress = static_cast<float>(cycleFrame - startFrame) /
        static_cast<float>(Duration[action] - 1);
    motion.direction = cycle % 2 == 0 ? 1.0f : -1.0f;
    motion.sideOriginX = -motion.direction * Travel[action] * 0.5f;
    motion.railOriginX = -8.0f + static_cast<float>((cycle * 29) % 17);
    motion.railDirection = cycle % 2 == 0 ? -1.0f : 1.0f;
    motion.railOriginZ = motion.railDirection < 0.0f ? 68.0f : 4.0f;
    motion.railTravel = 64.0f;
    motion.elevation = Elevation[action];
    motion.travel = Travel[action];
    motion.segmentSpacing = Spacing[action];
    motion.segmentDelay = Delay[action];
    motion.scale = Scale[action];
    return true;
}

constexpr float SideScrollingShooter::Stage3Module::GetSeaSerpentSegmentProgress(
    float progress, int segmentCount, float segmentDelay, int segmentIndex) {
    const float tailDelay = static_cast<float>(segmentCount - 1) * segmentDelay;
    return Math::Clamp01(progress * (1.0f + tailDelay) -
        static_cast<float>(segmentIndex) * segmentDelay);
}

SideScrollingShooter::Stage3Module::SeaSerpentSegment
SideScrollingShooter::Stage3Module::GetSeaSerpentSegment(
    const SeaSerpentMotion& motion, int segmentIndex) {
    static_assert(GetSeaSerpentSegmentProgress(1.0f, 23, 0.05f, 22) >=
        1.0f - Math::Epsilon);
    const float progress = GetSeaSerpentSegmentProgress(
        motion.progress, motion.segmentCount, motion.segmentDelay, segmentIndex);
    return {
        progress,
        std::sin(Math::Pi * progress) *
            (motion.elevation - static_cast<float>(segmentIndex) * 0.16f),
        motion.sideOriginX + motion.direction *
            (progress * motion.travel - static_cast<float>(segmentIndex) * motion.segmentSpacing),
        motion.railOriginX +
            std::sin(progress * 4.0f + static_cast<float>(segmentIndex) * 0.6f) * 6.0f,
        motion.railOriginZ + motion.railDirection *
            (progress * motion.railTravel - static_cast<float>(segmentIndex) * 2.6f),
        motion.scale * (segmentIndex == 0 ? 1.25f : 1.0f)
    };
}

void SideScrollingShooter::Stage3Module::DrawSky(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    const float nightBlend = NightBlend(shooter.m_frame);
    const ColorF skyColor {
        Math::Lerp(DaySkyColor[0], NightSkyColor[0], nightBlend),
        Math::Lerp(DaySkyColor[1], NightSkyColor[1], nightBlend),
        Math::Lerp(DaySkyColor[2], NightSkyColor[2], nightBlend),
        1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

void SideScrollingShooter::Stage3Module::DrawBackground2D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();
    const float nightBlend = NightBlend(shooter.m_frame);
    const float dayBlend = 1.0f - nightBlend;
    const float waterColor[4] = {
        Math::Lerp(WaterColor[0], WaterColor[0] * 0.30f, nightBlend),
        Math::Lerp(WaterColor[1], WaterColor[1] * 0.36f, nightBlend),
        Math::Lerp(WaterColor[2], WaterColor[2] * 0.48f, nightBlend),
        1.0f
    };
    const float waveColor[4] = {
        Math::Lerp(WaveColor[0], WaveColor[0] * 0.38f, nightBlend),
        Math::Lerp(WaveColor[1], WaveColor[1] * 0.42f, nightBlend),
        Math::Lerp(WaveColor[2], WaveColor[2] * 0.55f, nightBlend),
        1.0f
    };
    const float foamColor[4] = {
        FoamColor[0], FoamColor[1], FoamColor[2], 1.0f - nightBlend * 0.35f
    };
    const float cloudColor[4] = {CloudColor[0], CloudColor[1], CloudColor[2], dayBlend};
    const float sunColor[4] = {SunColor[0], SunColor[1], SunColor[2], dayBlend};

    // 朝に合わせて太陽を昇らせ、Cubeの雲を空の上部へ流す
    if (dayBlend > 0.01f) {
        const float sunX = backgroundHalfWidth * 0.56f;
        const float sunY = backgroundHalfHeight * (0.26f + dayBlend * 0.30f);
        constexpr float SkyZ = SidePlaneZ + 11.6f;
        DrawModelPrimitive(renderer, camera, 1, sunX, sunY, SkyZ,
            1.05f, 1.05f, 0.14f, sunColor);
        DrawModelPrimitive(renderer, camera, 1, sunX - 0.62f, sunY, SkyZ,
            0.28f, 0.28f, 0.15f, sunColor);
        DrawModelPrimitive(renderer, camera, 1, sunX + 0.62f, sunY, SkyZ,
            0.28f, 0.28f, 0.15f, sunColor);
        for (int i = 0; i < 7; ++i) {
            const float x = WrapNdcX(i * 0.53f - shooter.m_scroll * 0.06f) * backgroundHalfWidth;
            const float y = backgroundHalfHeight *
                (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
            DrawModelPrimitive(renderer, camera, 1, x, y, SkyZ,
                1.15f, 0.18f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.62f, y - 0.07f, SkyZ,
                0.62f, 0.13f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1, x + 0.68f, y - 0.05f, SkyZ,
                0.54f, 0.12f, 0.16f, cloudColor);
        }
    }

    // 海面の上端Y=-6を保ち、ドット絵調の波と泡を横スクロールさせる
    DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
        60.0f, 10.0f, 0.3f, waterColor);
    for (int i = 0; i < 24; ++i) {
        const float x = WrapNdcX(i * 0.29f -
            shooter.m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
        const float y = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
        const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
        constexpr float Z = SidePlaneZ + 13.6f;
        DrawModelPrimitive(renderer, camera, 1, x, y, Z,
            width, 0.10f, 0.18f, waveColor);
        if (i % 3 == 0) {
            DrawModelPrimitive(renderer, camera, 1,
                x - width * 0.18f, y + 0.16f, Z - 0.02f,
                width * 0.42f, 0.07f, 0.19f, foamColor);
        }
    }
    DrawSeaSerpent(shooter, renderer, camera, 0.0f);
}

void SideScrollingShooter::Stage3Module::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();
    const float nightBlend = NightBlend(shooter.m_frame);
    const float dayBlend = 1.0f - nightBlend;
    const float waterColor[4] = {
        Math::Lerp(WaterColor[0], WaterColor[0] * 0.30f, nightBlend),
        Math::Lerp(WaterColor[1], WaterColor[1] * 0.36f, nightBlend),
        Math::Lerp(WaterColor[2], WaterColor[2] * 0.48f, nightBlend),
        1.0f
    };
    const float waveColor[4] = {
        Math::Lerp(WaveColor[0], WaveColor[0] * 0.38f, nightBlend),
        Math::Lerp(WaveColor[1], WaveColor[1] * 0.42f, nightBlend),
        Math::Lerp(WaveColor[2], WaveColor[2] * 0.55f, nightBlend),
        1.0f
    };
    const float foamColor[4] = {
        FoamColor[0], FoamColor[1], FoamColor[2], 1.0f - nightBlend * 0.35f
    };
    const float cloudColor[4] = {CloudColor[0], CloudColor[1], CloudColor[2], dayBlend};
    const float sunColor[4] = {SunColor[0], SunColor[1], SunColor[2], dayBlend};

    // 太陽と雲は横視点の配置からレール空間へ補間する
    if (dayBlend > 0.01f) {
        const float sideSunX = sideBackgroundHalfWidth * 0.56f;
        const float sideSunY = sideBackgroundHalfHeight * (0.26f + dayBlend * 0.30f);
        const float sunX = Math::Lerp(sideSunX, 32.0f, railWeight);
        const float sunY = Math::Lerp(sideSunY, 14.0f, railWeight);
        const float sunZ = Math::Lerp(SidePlaneZ + 11.6f, 72.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 1, sunX, sunY, sunZ,
            Math::Lerp(1.05f, 2.8f, railWeight),
            Math::Lerp(1.05f, 2.8f, railWeight),
            Math::Lerp(0.14f, 0.35f, railWeight), sunColor);
        DrawModelPrimitive(renderer, camera, 1,
            sunX - Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
        DrawModelPrimitive(renderer, camera, 1,
            sunX + Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
        for (int i = 0; i < 7; ++i) {
            const float sideX = WrapNdcX(i * 0.53f - shooter.m_scroll * 0.06f) *
                sideBackgroundHalfWidth;
            const float sideY = sideBackgroundHalfHeight *
                (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
            const float x = Math::Lerp(sideX,
                -42.0f + static_cast<float>((i * 73) % 840) / 10.0f, railWeight);
            const float y = Math::Lerp(sideY,
                8.0f + static_cast<float>((i * 7) % 10), railWeight);
            const float z = Math::Lerp(SidePlaneZ + 11.6f,
                32.0f + static_cast<float>((i * 37) % 70), railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z,
                Math::Lerp(1.15f, 2.2f, railWeight),
                Math::Lerp(0.18f, 0.30f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(0.62f, 1.2f, railWeight),
                y - Math::Lerp(0.07f, 0.12f, railWeight), z,
                Math::Lerp(0.62f, 1.1f, railWeight),
                Math::Lerp(0.13f, 0.22f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + Math::Lerp(0.68f, 1.3f, railWeight),
                y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                Math::Lerp(0.54f, 1.0f, railWeight),
                Math::Lerp(0.12f, 0.20f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
        }
    }

    // 横視点の海面と同じ波配置をレール空間へ補間する
    DrawModelPrimitive(renderer, camera, 1, 0.0f,
        Math::Lerp(-11.0f, -4.0f, railWeight),
        Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
        Math::Lerp(60.0f, 140.0f, railWeight),
        Math::Lerp(10.0f, 0.7f, railWeight),
        Math::Lerp(0.3f, 140.0f, railWeight), waterColor);
    for (int i = 0; i < 24; ++i) {
        const float sideX = WrapNdcX(i * 0.29f -
            shooter.m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
        const float sideY = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
        const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
        const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
        const float railZ = 8.0f + std::fmod(
            static_cast<float>(i * 43) - shooter.m_scroll * 28.0f + 110.0f, 110.0f);
        const float x = Math::Lerp(sideX, railX, railWeight);
        const float y = Math::Lerp(sideY, -3.65f + 0.045f * 0.5f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.6f, railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 1, x, y, z,
            Math::Lerp(width, 1.5f, railWeight),
            Math::Lerp(0.10f, 0.045f, railWeight),
            Math::Lerp(0.18f, 0.70f, railWeight), waveColor);
        if (i % 3 == 0) {
            const float foamY = Math::Lerp(
                sideY + 0.16f, -3.65f + 0.045f + 0.03f * 0.5f, railWeight);
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(width * 0.18f, 0.25f, railWeight),
                foamY, z - Math::Lerp(0.02f, 0.08f, railWeight),
                Math::Lerp(width * 0.42f, 0.65f, railWeight),
                Math::Lerp(0.07f, 0.03f, railWeight),
                Math::Lerp(0.19f, 0.72f, railWeight), foamColor);
        }
    }
    DrawSeaSerpent(shooter, renderer, camera, railWeight);
}

bool SideScrollingShooter::Stage3Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(shooter.m_frame, motion)) {
        return false;
    }

    // 描画と同じ胴体節配置を使い、横視点とレール視点の双方で判定する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const SeaSerpentSegment segment = GetSeaSerpentSegment(motion, i);
        if (shooter.IsRailGameplayActive()) {
            const float railHeight = 2.50f * segment.scale;
            const float visibleHeight = Math::Clamp01(segment.elevation / railHeight) * railHeight;
            if (visibleHeight <= 0.0f) {
                continue;
            }
            const float visibleScale = segment.scale * std::sqrt(visibleHeight / railHeight);
            const float railY = -3.65f + (segment.elevation < railHeight ?
                visibleHeight * 0.5f : segment.elevation - railHeight * 0.5f);
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                segment.railX, railY, segment.railZ, 1.25f * visibleScale)) {
                return true;
            }
        } else {
            const float sideHeight = 1.35f * segment.scale;
            const float visibleHeight = Math::Clamp01(segment.elevation / sideHeight) * sideHeight;
            if (visibleHeight <= 0.0f) {
                continue;
            }
            const float visibleWidth = 1.18f * segment.scale *
                std::sqrt(visibleHeight / sideHeight);
            const float hitWidth = visibleWidth * 0.5f + radius * WorldXScale;
            const float hitHeight = visibleHeight * 0.5f + radius * WorldYScale;
            const float dx = (ToWorldX(x) - segment.sideX) / hitWidth;
            const float sideY = -6.0f + (segment.elevation < sideHeight ?
                visibleHeight * 0.5f : segment.elevation - sideHeight * 0.5f);
            const float dy = (ToWorldY(y) - sideY) / hitHeight;
            if (dx * dx + dy * dy <= 1.0f) {
                return true;
            }
        }
    }
    return false;
}

void SideScrollingShooter::Stage3Module::DrawSeaSerpent(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(shooter.m_frame, motion)) {
        return;
    }

    // 通常跳躍、低空横断、超巨大ジャンプを判定と同じ胴体節配置で描画する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const SeaSerpentSegment segment = GetSeaSerpentSegment(motion, i);
        const float sideWidth = 1.18f * segment.scale;
        const float sideHeight = 1.35f * segment.scale;
        const float sideDepth = 0.72f * segment.scale;
        const float railSize = 2.50f * segment.scale;
        const float sideVisibleHeight = Math::Clamp01(segment.elevation / sideHeight) * sideHeight;
        const float railVisibleHeight = Math::Clamp01(segment.elevation / railSize) * railSize;
        if (sideVisibleHeight <= 0.0f && railVisibleHeight <= 0.0f) {
            continue;
        }
        const float sideVisibleScale = std::sqrt(sideVisibleHeight / sideHeight);
        const float railVisibleScale = std::sqrt(railVisibleHeight / railSize);
        const float x = Math::Lerp(segment.sideX, segment.railX, railWeight);
        const float sideY = -6.0f + (segment.elevation < sideHeight ?
            sideVisibleHeight * 0.5f : segment.elevation - sideHeight * 0.5f);
        const float railY = -3.65f + (segment.elevation < railSize ?
            railVisibleHeight * 0.5f : segment.elevation - railSize * 0.5f);
        const float y = Math::Lerp(sideY, railY, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.1f, segment.railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(sideWidth * sideVisibleScale,
                railSize * railVisibleScale, railWeight),
            Math::Lerp(sideVisibleHeight, railVisibleHeight, railWeight),
            Math::Lerp(sideDepth * sideVisibleScale,
                railSize * railVisibleScale, railWeight), SeaSerpentColor);
        if (i % 2 == 1 &&
            (sideVisibleHeight >= sideHeight || railVisibleHeight >= railSize)) {
            DrawModelPrimitive(renderer, camera, 5,
                x, y - Math::Lerp(0.48f, 0.82f, railWeight), z - 0.05f,
                Math::Lerp(0.52f, 1.15f, railWeight),
                Math::Lerp(0.28f, 0.48f, railWeight),
                Math::Lerp(0.18f, 0.42f, railWeight), SeaSerpentBellyColor);
        }
    }

    const SeaSerpentSegment head = GetSeaSerpentSegment(motion, 0);
    const float headSideVisibleHeight = Math::Clamp01(
        head.elevation / (1.35f * head.scale)) * 1.35f * head.scale;
    const float headRailVisibleHeight = Math::Clamp01(
        head.elevation / (2.50f * head.scale)) * 2.50f * head.scale;
    const float headX = Math::Lerp(head.sideX, head.railX, railWeight);
    const float headSideY = -6.0f + (head.elevation < 1.35f * head.scale ?
        headSideVisibleHeight * 0.5f : head.elevation - 1.35f * head.scale * 0.5f);
    const float headRailY = -3.65f + (head.elevation < 2.50f * head.scale ?
        headRailVisibleHeight * 0.5f : head.elevation - 2.50f * head.scale * 0.5f);
    const float headY = Math::Lerp(headSideY, headRailY, railWeight);
    const float headZ = Math::Lerp(SidePlaneZ + 13.1f, head.railZ, railWeight);

    // 頭が海面を出入りする短い時間だけ、水滴を初速と重力による放物線で飛ばす
    const float emergeProgress = std::asin((std::min)(
        1.0f, 1.35f * head.scale / motion.elevation)) / Math::HalfPi;
    const float reentryProgress = 1.0f - emergeProgress;
    const float splashCenter = head.progress < 0.5f ? emergeProgress : reentryProgress;
    const float splashTime = Math::Clamp01((head.progress -
        (splashCenter - emergeProgress)) / (emergeProgress * 2.0f));
    const bool splashActive = std::abs(head.progress - splashCenter) <= emergeProgress;
    if (splashActive) {
        for (int i = 0; i < 17; ++i) {
            const float spread = static_cast<float>(i - 8) *
                0.22f * motion.scale * splashTime;
            const float launchVelocity = (1.05f +
                static_cast<float>((i * 5) % 4) * 0.22f) * motion.scale;
            const float gravity = launchVelocity * 2.0f;
            const float dropletHeight = launchVelocity * splashTime -
                gravity * splashTime * splashTime * 0.5f;
            const float sideSplashX = head.sideX - motion.direction * spread;
            const float railSplashX = head.railX - motion.direction * spread;
            DrawModelPrimitive(renderer, camera, 5,
                Math::Lerp(sideSplashX, railSplashX, railWeight),
                Math::Lerp(-5.75f + dropletHeight, -3.45f + dropletHeight, railWeight),
                Math::Lerp(SidePlaneZ + 13.0f,
                    head.railZ - motion.railDirection * spread, railWeight),
                Math::Lerp(0.16f, 0.42f, railWeight) * motion.scale,
                Math::Lerp(0.28f, 0.65f, railWeight) * motion.scale,
                Math::Lerp(0.10f, 0.32f, railWeight) * motion.scale, FoamColor);
        }
    }
    if (headSideVisibleHeight >= 1.35f * head.scale ||
        headRailVisibleHeight >= 2.50f * head.scale) {
        // 目をカメラ側の頭部表面より前へ置き、移動中も胴体へ埋まらないようにする
        const Vector3 headPosition {headX, headY, headZ};
        const Vector3 eyePosition = headPosition +
            camera.Right() * (Math::Lerp(0.36f, 0.82f, railWeight) * motion.scale) +
            camera.Up() * (Math::Lerp(0.30f, 0.72f, railWeight) * motion.scale) +
            (camera.Position() - headPosition).Normalized() *
                (Math::Lerp(SeaSerpentSideEyeSurfaceOffset,
                    SeaSerpentRailEyeSurfaceOffset, railWeight) * motion.scale);
        DrawModelPrimitive(renderer, camera, 5,
            eyePosition.x, eyePosition.y, eyePosition.z,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.10f, 0.20f, railWeight) * motion.scale,
            SeaSerpentEyeColor);
    }
}
