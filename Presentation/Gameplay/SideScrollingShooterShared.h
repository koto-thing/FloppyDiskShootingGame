#pragma once

namespace SideScrollingShooterShared {
inline constexpr float SideCameraZ = -16.0f;
inline constexpr float SideCameraFieldOfView = 38.0f;
inline constexpr int BossNameRevealFrames = 150;
inline constexpr const char BossWarningSirenMml[] = "t240 @5 o4 v13 r8 e2 r8 e2";

/**
 * @brief 点が軸平行楕円体の内側にあるか判定する
 * @param deltaX 楕円体中心から点までのX距離
 * @param deltaY 楕円体中心から点までのY距離
 * @param deltaZ 楕円体中心から点までのZ距離
 * @param radiusX 楕円体のX半径
 * @param radiusY 楕円体のY半径
 * @param radiusZ 楕円体のZ半径
 * @return 点が楕円体の内側または表面にある場合true
 */
constexpr bool HitsEllipsoid(float deltaX, float deltaY, float deltaZ,
    float radiusX, float radiusY, float radiusZ) {
    if (radiusX <= 0.0f || radiusY <= 0.0f || radiusZ <= 0.0f) return false;

    // 各軸を単位球へ正規化して距離を判定する
    const float x = deltaX / radiusX;
    const float y = deltaY / radiusY;
    const float z = deltaZ / radiusZ;
    return x * x + y * y + z * z <= 1.0f;
}

static_assert(HitsEllipsoid(2.0f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f));
static_assert(!HitsEllipsoid(2.01f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f));
}
