#pragma once

namespace SideScrollingShooterShared {
inline constexpr float OceanFoamColor[4] = { 0.78f, 0.94f, 0.92f, 1.0f };
inline constexpr float SideCameraZ = -16.0f;
inline constexpr float SideCameraFieldOfView = 38.0f;
inline constexpr int Stage2BossApproachFrames = 90;
inline constexpr int Stage2BossAssemblyFrames = 90;
inline constexpr int BossNameRevealFrames = 150;

enum Stage5Cue {
    Stage5DistantThunder,
    Stage5Thunder,
    Stage5SearchlightDetect,
    Stage5SearchlightLocked,
    Stage5BarrageWarning,
    Stage5EastsourceEntrance,
    Stage5SignalLost,
    Stage5Transformation,
    Stage5WeakpointDestroyed,
    Stage5CoreWarning,
    Stage5ChainExplosion,
    Stage5FinalExplosion
};
}
