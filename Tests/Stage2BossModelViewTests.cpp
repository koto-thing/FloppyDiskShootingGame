#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage2/Stage2BossModelView.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2State.h"

/**
 * @brief Stage2の砂嵐の濃度と副砲の砲口座標を検証する
 * @return なし
 */
void RunStage2BossModelViewTests() {
    // 登場から3秒で最大となり、会話や形態変更を挟んでも維持し、撃破後は晴れる
    using ShooterStages::Stage2::NextSandstormExposure;
    using ShooterStages::Stage2::SandstormFadeFrames;
    int exposure = 0;
    assert(NextSandstormExposure(exposure, false) == 0);
    for (int frame = 0; frame < SandstormFadeFrames; ++frame) {
        const int next = NextSandstormExposure(exposure, true);
        assert(next > exposure && next <= SandstormFadeFrames);
        exposure = next;
    }
    assert(exposure == SandstormFadeFrames);
    assert(NextSandstormExposure(exposure, true) == exposure);
    for (int frame = 0; frame < SandstormFadeFrames; ++frame) {
        const int next = NextSandstormExposure(exposure, false);
        assert(next < exposure && next >= 0);
        exposure = next;
    }
    assert(exposure == 0);
    assert(NextSandstormExposure(45, false) == 44);
    assert(ShooterStages::Stage2::State {}.sandstormExposure == 0);
    assert(ShooterStages::Stage2::State {}.sandstormFrame == 0);
    assert(ShooterStages::Stage2::SandstormParticlesPerLayer == 24);

    // ハッチ別射出間隔が範囲内で変化することを確認する
    const int firstInterval = ShooterStages::Stage2::Phase3FunnelLaunchInterval(0, 0);
    const int nextInterval = ShooterStages::Stage2::Phase3FunnelLaunchInterval(0, 1);
    const int otherHatchInterval = ShooterStages::Stage2::Phase3FunnelLaunchInterval(1, 0);
    assert(firstInterval >= 55 && firstInterval <= 145);
    assert(nextInterval >= 55 && nextInterval <= 145);
    assert(firstInterval != nextInterval);
    assert(firstInterval != otherHatchInterval);

    // Phase 1、2相当の固定照準では砲口が艦首方向へ砲身長分進むことを確認する
    BossModelTransform fixed {{10.0f, 20.0f, 30.0f}, {}, 0.0f, 2.0f};
    const Vector3 fixedMuzzle = LandBattleshipView::SecondaryGunMuzzlePosition(fixed, 0);
    assert(std::fabs(fixedMuzzle.x - 5.35f) < 0.0001f);
    assert(std::fabs(fixedMuzzle.y - 25.20f) < 0.0001f);
    assert(std::fabs(fixedMuzzle.z - 28.16f) < 0.0001f);

    // Phase 3相当の追尾照準では砲口が支点から照準先方向へ移動することを確認する
    BossModelTransform tracked {{}, {}, 0.0f, 1.0f};
    tracked.secondaryGunsTrackTarget = true;
    tracked.secondaryAimTarget = {-0.55f, 2.60f, 10.0f};
    const Vector3 trackedMuzzle = LandBattleshipView::SecondaryGunMuzzlePosition(tracked, 0);
    assert(std::fabs(trackedMuzzle.x - -0.55f) < 0.0001f);
    assert(std::fabs(trackedMuzzle.y - 2.60f) < 0.0001f);
    assert(std::fabs(trackedMuzzle.z - 0.855f) < 0.0001f);
}
