#pragma once

#include <cmath>

#include "../Stages/Common/BossModelTransform.h"

/** @brief 共通大型戦闘機ボスの部位表示状態 */
struct LegacyBossModelState {
    bool nose = true;
    bool leftWing = true;
    bool rightWing = true;
    bool leftEngine = true;
    bool rightEngine = true;
    bool hit[5] {};
};

/** @brief Stage 1などで使用する共通大型戦闘機のプロシージャルモデル */
class LegacyBossModelView final {
public:
    /**
     * @brief 共通大型戦闘機を描画する
     * @param transform モデル全体の親Transform
     * @param drawPart 形状、位置、寸法、色、Yaw、Pitchを受け取る描画関数
     * @param state 部位の表示と被弾状態
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, DrawPart&& drawPart,
        const LegacyBossModelState& state = {}) {
        constexpr float Gray[] = {0.50f, 0.50f, 0.50f, 1.0f};
        constexpr float White[] = {0.60f, 0.60f, 0.60f, 1.0f};
        constexpr float Black[] = {0.20f, 0.20f, 0.20f, 1.0f};
        constexpr float Hit[] = {1.0f, 0.03f, 0.03f, 1.0f};
        auto color = [&](int part, const float base[4]) { return state.hit[part] ? Hit : base; };

        // 機首と上下の中央船体を描画する
        if (state.nose) {
            Part(transform, drawPart, 2, {0.0f, 3.0f, -14.0f}, {6.0f, 6.0f, 4.0f}, color(0, Gray));
            Part(transform, drawPart, 2, {0.0f, 2.0f, -17.5f}, {2.0f, 2.0f, 3.0f}, color(0, Gray));
            Part(transform, drawPart, 2, {0.0f, 4.5f, -20.0f}, {1.0f, 1.0f, 8.0f}, color(0, Black));
        }
        Part(transform, drawPart, 2, {0.0f, 2.0f, 0.0f}, {18.0f, 18.0f, 16.0f}, Gray);
        Part(transform, drawPart, 2, {0.0f, 2.0f, -10.0f}, {14.0f, 14.0f, 4.0f}, Gray);
        Part(transform, drawPart, 2, {0.0f, 2.0f, 10.0f}, {14.0f, 14.0f, 4.0f}, Gray);
        Part(transform, drawPart, 1, {0.0f, 12.0f, 2.0f}, {4.0f, 4.0f, 4.0f}, Gray);
        Part(transform, drawPart, 2, {0.0f, 13.0f, -2.0f}, {1.0f, 1.0f, 4.0f}, Black);
        Part(transform, drawPart, 2, {0.0f, -12.0f, 0.0f}, {4.0f, 4.0f, 10.0f}, Gray);
        Part(transform, drawPart, 2, {0.0f, -15.0f, 1.0f}, {2.0f, 2.0f, 8.0f}, Gray);
        Part(transform, drawPart, 2, {0.0f, -12.0f, -7.0f}, {1.0f, 1.0f, 6.0f}, Black);
        Part(transform, drawPart, 1, {2.0f, -8.0f, 0.0f}, {1.0f, 5.0f, 1.0f}, Black);
        Part(transform, drawPart, 1, {-2.0f, -8.0f, 0.0f}, {1.0f, 5.0f, 1.0f}, Black);

        // 左右主翼とエンジンを部位状態に合わせて描画する
        if (state.leftWing) {
            Part(transform, drawPart, 1, {13.0f, 2.0f, 0.0f}, {8.0f, 4.0f, 12.0f}, color(1, White));
            Part(transform, drawPart, 1, {21.0f, 2.0f, 0.0f}, {12.0f, 2.0f, 10.0f}, color(1, White));
        }
        if (state.rightWing) {
            Part(transform, drawPart, 1, {-13.0f, 2.0f, 0.0f}, {8.0f, 4.0f, 12.0f}, color(2, White));
            Part(transform, drawPart, 1, {-21.0f, 2.0f, 0.0f}, {12.0f, 2.0f, 10.0f}, color(2, White));
        }
        Part(transform, drawPart, 2, {0.0f, 3.0f, 15.0f}, {10.0f, 10.0f, 6.0f}, Gray);
        Part(transform, drawPart, 2, {7.0f, 3.0f, 18.0f}, {4.0f, 4.0f, 6.0f}, Black);
        Part(transform, drawPart, 2, {-7.0f, 3.0f, 18.0f}, {4.0f, 4.0f, 6.0f}, Black);
        Part(transform, drawPart, 1, {0.0f, -6.0f, 16.5f}, {2.0f, 8.0f, 3.0f}, White);
        Part(transform, drawPart, 1, {0.0f, 12.0f, 16.5f}, {2.0f, 8.0f, 3.0f}, White);
        if (state.leftEngine) {
            Part(transform, drawPart, 2, {6.0f, -6.0f, 10.0f}, {4.0f, 4.0f, 10.0f}, color(3, Black));
            Part(transform, drawPart, 2, {6.0f, -6.0f, 16.0f}, {2.0f, 2.0f, 2.0f}, color(3, Black));
        }
        if (state.rightEngine) {
            Part(transform, drawPart, 2, {-6.0f, -6.0f, 10.0f}, {4.0f, 4.0f, 10.0f}, color(4, Black));
            Part(transform, drawPart, 2, {-6.0f, -6.0f, 16.0f}, {2.0f, 2.0f, 2.0f}, color(4, Black));
        }
    }

private:
    /**
     * @brief 一つの部品を親Transformへ合成する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param shape 既存PrimitiveShapeに対応する番号
     * @param localPosition ローカル座標
     * @param localScale ローカル寸法
     * @param color 部品色
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, int shape,
        const Vector3& localPosition, const Vector3& localScale, const float color[4]) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 position {
            transform.position.x + (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z + (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
        drawPart(shape, position, localScale * transform.scale, color, transform.yaw, 0.0f);
    }
};
