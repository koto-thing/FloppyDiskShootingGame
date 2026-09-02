#pragma once

#include "../Common/BossModelTransform.h"

#include "../../../../Engine/Graphics/IRenderBackend.h"

/** @brief Stage4ボスの独立破壊対象を描画へ反映する */
struct Stage4BossModelState {
    bool mainTurret = true;
    bool mainCannon = true;
    bool secondaryGuns[6] = {true, true, true, true, true, true};
    bool commandTower = true;
    bool exhaustStacks[2] = {true, true};
    bool frontRam = true;
};

/** @brief 黒塗り高級車と超重戦車を融合したStage4ボスのプロシージャル描画 */
class Stage4BossModelView {
public:
    static constexpr int Stage4LaneCount = 4;
    static constexpr float Stage4LaneWidth = 6.0f;
    static constexpr float ModelWidth = Stage4LaneCount * Stage4LaneWidth;
    static constexpr int ChassisPrimitiveCount = 10;
    static constexpr int TrackPrimitiveCount = 46;
    static constexpr int LuxuryBodyPrimitiveCount = 25;
    static constexpr int MainTurretPrimitiveCount = 9;
    static constexpr int MainCannonPrimitiveCount = 6;
    static constexpr int CommandTowerPrimitiveCount = 9;
    static constexpr int SecondaryGunPrimitiveCount = 18;
    static constexpr int ExhaustPrimitiveCount = 8;
    static constexpr int FrontPrimitiveCount = 19;
    static constexpr int DecorationPrimitiveCount = 10;
    static constexpr int PrimitiveCount = ChassisPrimitiveCount + TrackPrimitiveCount +
        LuxuryBodyPrimitiveCount + MainTurretPrimitiveCount + MainCannonPrimitiveCount +
        CommandTowerPrimitiveCount + SecondaryGunPrimitiveCount + ExhaustPrimitiveCount +
        FrontPrimitiveCount + DecorationPrimitiveCount;

    /**
     * @brief Stage4ボスをローカル部品の組み合わせで描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param state 独立破壊対象の描画状態
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, DrawPart&& drawPart,
        const Stage4BossModelState& state = {}) {
        // 常設車体を下層から順に描画する
        DrawChassis(transform, drawPart);
        DrawTracks(transform, drawPart);
        DrawLuxuryBody(transform, drawPart);

        // 将来の部位破壊単位ごとに上部構造を描画する
        if (state.mainTurret) DrawMainTurret(transform, drawPart);
        if (state.mainCannon) DrawMainCannon(transform, drawPart);
        if (state.commandTower) DrawCommandTower(transform, drawPart);
        DrawSecondaryGuns(transform, drawPart, state);
        DrawExhaustStacks(transform, drawPart, state);
        DrawFront(transform, drawPart, state.frontRam);
        DrawDecorations(transform, drawPart);
    }

private:
    inline static constexpr float MainBlack[] = {0.025f, 0.025f, 0.030f, 1.0f};
    inline static constexpr float ArmorBlack[] = {0.060f, 0.065f, 0.075f, 1.0f};
    inline static constexpr float HighlightBlack[] = {0.10f, 0.10f, 0.12f, 1.0f};
    inline static constexpr float Window[] = {0.015f, 0.020f, 0.035f, 1.0f};
    inline static constexpr float Gold[] = {0.55f, 0.38f, 0.08f, 1.0f};
    inline static constexpr float Light[] = {0.90f, 0.80f, 0.50f, 1.0f};
    inline static constexpr float Track[] = {0.035f, 0.038f, 0.045f, 1.0f};
    inline static constexpr float Wheel[] = {0.13f, 0.14f, 0.16f, 1.0f};

    /**
     * @brief ローカル部品をStage2と共通の親Transform合成処理へ渡す
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param shape 描画形状
     * @param localPosition ローカル座標
     * @param scale ローカル寸法
     * @param color 部品色
     * @param localYaw 部品固有のY軸回転
     * @param pitch 部品固有のZ軸回転
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, PrimitiveShape shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4],
        float localYaw = 0.0f, float pitch = 0.0f) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 position {
            transform.position.x +
                (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z +
                (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
        drawPart(static_cast<int>(shape), position, scale * transform.scale,
            color, transform.yaw + localYaw, pitch);
    }

    /**
     * @brief 低重心の積層装甲車体を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawChassis(const BossModelTransform& transform, DrawPart& drawPart) {
        // 中央、前部傾斜、後部機関室で一枚箱に見えない基礎車体を作る
        Part(transform, drawPart, PrimitiveShape::Box, {0.0f, 0.35f, 0.0f}, {14.4f, 1.35f, ModelWidth}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-6.55f, 0.28f, 0.0f}, {2.0f, 1.10f, ModelWidth - 0.6f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {5.75f, 0.65f, 0.0f}, {2.45f, 1.65f, ModelWidth - 0.4f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {0.15f, -0.58f, 0.0f}, {12.9f, 0.50f, ModelWidth - 2.0f}, Track);

        // 左右の張り出し装甲で幅と重量感を補う
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {-0.15f, 0.48f, side * 11.82f}, {11.8f, 0.72f, 0.36f}, ArmorBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {-6.35f, 0.36f, side * 11.70f}, {1.55f, 0.86f, 0.54f}, HighlightBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {5.65f, 0.64f, side * 11.70f}, {1.55f, 1.02f, 0.54f}, HighlightBlack, Math::Pi);
        }
    }

    /**
     * @brief 丸い端部と転輪を持つ左右の大型履帯を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawTracks(const BossModelTransform& transform, DrawPart& drawPart) {
        for (float side : {-1.0f, 1.0f}) {
            // 長い履帯板と円柱端部で角張りすぎない外周を作る
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, -0.08f, side * 11.10f}, {13.4f, 2.25f, 0.54f}, Track);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {-6.70f, -0.08f, side * 11.10f}, {2.28f, 0.58f, 2.28f}, Track, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {6.70f, -0.08f, side * 11.10f}, {2.28f, 0.58f, 2.28f}, Track, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, 0.91f, side * 11.16f}, {11.8f, 0.48f, 0.66f}, MainBlack);
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, -1.08f, side * 11.16f}, {11.8f, 0.30f, 0.66f}, MainBlack);

            // 規則的な中心位置と大小差を持つ八輪を外装の間から見せる
            for (int wheel = 0; wheel < 8; ++wheel) {
                const float x = -5.25f + static_cast<float>(wheel) * 1.50f;
                const float edge = std::fabs(static_cast<float>(wheel) - 3.5f) / 3.5f;
                const float diameter = 1.48f - edge * 0.22f;
                Part(transform, drawPart, PrimitiveShape::Cylinder, {x, -0.18f, side * 11.40f},
                    {diameter, 0.22f, diameter}, Wheel, Math::HalfPi, Math::HalfPi);
                Part(transform, drawPart, PrimitiveShape::Cylinder, {x, -0.18f, side * 11.53f},
                    {diameter * 0.42f, 0.12f, diameter * 0.42f}, Gold, Math::HalfPi, Math::HalfPi);
            }

            // 前後の斜め装甲で履帯上部を車体へつなぐ
            Part(transform, drawPart, PrimitiveShape::Prism, {-6.20f, 0.82f, side * 11.16f}, {1.30f, 0.62f, 0.70f}, ArmorBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {6.20f, 0.82f, side * 11.16f}, {1.30f, 0.62f, 0.70f}, ArmorBlack, Math::Pi);
        }
    }

    /**
     * @brief 戦車上へ融合した低く長いリムジン車体を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawLuxuryBody(const BossModelTransform& transform, DrawPart& drawPart) {
        // 長いボディを滑らかな段差で積み上げる
        Part(transform, drawPart, PrimitiveShape::Box, {-0.35f, 1.36f, 0.0f}, {11.8f, 1.08f, 19.8f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-5.75f, 1.65f, 0.0f}, {2.40f, 0.82f, 19.2f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {-0.55f, 2.02f, 0.0f}, {6.75f, 0.88f, 17.8f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {3.85f, 1.82f, 0.0f}, {2.05f, 0.78f, 18.8f}, ArmorBlack, Math::Pi);
        Part(transform, drawPart, PrimitiveShape::Prism, {-0.35f, 2.62f, 0.0f}, {6.15f, 0.40f, 16.4f}, HighlightBlack);

        // 両側の窓列と金色モールで高級車の読みやすさを作る
        for (float side : {-1.0f, 1.0f}) {
            for (int window = 0; window < 6; ++window) {
                const float x = -3.65f + static_cast<float>(window) * 1.30f;
                Part(transform, drawPart, PrimitiveShape::Box, {x, 2.18f, side * 9.02f},
                    {0.96f, 0.48f, 0.10f}, Window);
            }
            Part(transform, drawPart, PrimitiveShape::Box, {-0.15f, 1.58f, side * 9.96f}, {8.75f, 0.10f, 0.08f}, Gold);
            for (int handle = 0; handle < 3; ++handle) {
                Part(transform, drawPart, PrimitiveShape::Box,
                    {-2.90f + static_cast<float>(handle) * 2.45f, 1.83f, side * 9.97f},
                    {0.42f, 0.10f, 0.08f}, Gold);
            }
        }
    }

    /**
     * @brief 厚い前面と斜め側面を持つ主砲塔を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMainTurret(const BossModelTransform& transform, DrawPart& drawPart) {
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-1.45f, 2.92f, 0.0f}, {3.35f, 0.55f, 11.4f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {-1.30f, 3.40f, 0.0f}, {3.65f, 1.05f, 10.4f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-3.10f, 3.38f, 0.0f}, {1.15f, 1.18f, 10.0f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {0.52f, 3.34f, 0.0f}, {1.05f, 0.92f, 9.6f}, MainBlack, Math::Pi);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Prism, {-1.28f, 3.48f, side * 5.15f}, {2.80f, 0.82f, 0.38f}, HighlightBlack, side * 0.08f);
            Part(transform, drawPart, PrimitiveShape::Box, {-1.55f, 3.92f, side * 4.82f}, {1.65f, 0.12f, 0.10f}, Gold);
        }
        Part(transform, drawPart, PrimitiveShape::Box, {-0.95f, 4.02f, 0.0f}, {2.30f, 0.26f, 7.8f}, MainBlack);
    }

    /**
     * @brief 前方へ突き出す三重砲身と多重砲口を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMainCannon(const BossModelTransform& transform, DrawPart& drawPart) {
        // 円柱のY軸をローカル-Xへ倒し、全砲身を砲塔中心線へ揃える
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-3.25f, 3.55f, 0.0f}, {1.18f, 1.55f, 1.18f}, ArmorBlack, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-4.85f, 3.55f, 0.0f}, {0.92f, 2.35f, 0.92f}, HighlightBlack, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-6.80f, 3.55f, 0.0f}, {0.76f, 1.65f, 0.76f}, ArmorBlack, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-7.78f, 3.55f, 0.0f}, {1.16f, 0.42f, 1.16f}, MainBlack, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-8.02f, 3.55f, 0.0f}, {0.68f, 0.18f, 0.68f}, Window, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-7.55f, 3.55f, 0.0f}, {1.22f, 0.12f, 1.22f}, Gold, 0.0f, Math::HalfPi);
    }

    /**
     * @brief モデル最高点となる段積み指揮塔を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawCommandTower(const BossModelTransform& transform, DrawPart& drawPart) {
        Part(transform, drawPart, PrimitiveShape::Prism, {1.50f, 3.28f, 0.0f}, {2.25f, 0.62f, 2.05f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {1.65f, 3.82f, 0.0f}, {1.72f, 0.72f, 1.62f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {1.72f, 4.35f, 0.0f}, {1.35f, 0.48f, 1.28f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {1.72f, 4.72f, -0.67f}, {0.82f, 0.25f, 0.10f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {1.72f, 4.72f, 0.67f}, {0.82f, 0.25f, 0.10f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {1.03f, 4.72f, 0.0f}, {0.10f, 0.25f, 0.72f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {2.41f, 4.72f, 0.0f}, {0.10f, 0.25f, 0.72f}, Window);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {1.72f, 5.18f, 0.0f}, {0.16f, 0.72f, 0.16f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Sphere, {1.72f, 5.62f, 0.0f}, {0.40f, 0.40f, 0.40f}, Light);
    }

    /**
     * @brief 主砲横二基を含む左右六基の副砲を独立状態で描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param state 副砲ごとの描画状態
     * @return なし
     */
    template<class DrawPart>
    static void DrawSecondaryGuns(const BossModelTransform& transform, DrawPart& drawPart,
        const Stage4BossModelState& state) {
        constexpr Vector3 Positions[] = {
            {-2.85f, 3.48f, -3.45f}, {-2.85f, 3.48f, 3.45f},
            {-3.85f, 2.80f, -7.65f}, {-3.85f, 2.80f, 7.65f},
            {3.65f, 2.72f, -7.15f}, {3.65f, 2.72f, 7.15f}
        };
        for (int gun = 0; gun < 6; ++gun) {
            if (!state.secondaryGuns[gun]) continue;
            const Vector3& position = Positions[gun];
            Part(transform, drawPart, PrimitiveShape::Cylinder, position, {0.78f, 0.38f, 0.78f}, MainBlack);
            Part(transform, drawPart, PrimitiveShape::Box, {position.x - 0.18f, position.y + 0.28f, position.z},
                {0.88f, 0.42f, 0.68f}, ArmorBlack);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {position.x - 0.92f, position.y + 0.28f, position.z},
                {0.24f, 1.25f, 0.24f}, HighlightBlack, 0.0f, Math::HalfPi);
        }
    }

    /**
     * @brief 後方斜め上へ伸びる二本の段付き排気筒を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param state 排気筒ごとの描画状態
     * @return なし
     */
    template<class DrawPart>
    static void DrawExhaustStacks(const BossModelTransform& transform, DrawPart& drawPart,
        const Stage4BossModelState& state) {
        for (int stack = 0; stack < 2; ++stack) {
            if (!state.exhaustStacks[stack]) continue;
            const float side = stack == 0 ? -1.0f : 1.0f;
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.10f, 2.34f, side * 5.85f}, {0.62f, 0.78f, 0.62f}, MainBlack);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.30f, 3.05f, side * 5.85f}, {0.48f, 0.82f, 0.48f}, ArmorBlack, 0.0f, -0.22f);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.52f, 3.76f, side * 5.85f}, {0.36f, 0.72f, 0.36f}, HighlightBlack, 0.0f, -0.22f);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.61f, 4.07f, side * 5.85f}, {0.48f, 0.16f, 0.48f}, Gold, 0.0f, -0.22f);
        }
    }

    /**
     * @brief 高級車風グリル、灯火、前方ラムを描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param frontRam ラムを描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawFront(const BossModelTransform& transform, DrawPart& drawPart, bool frontRam) {
        // 二重グリルと七本の縦桟を最前面中央へ配置する
        Part(transform, drawPart, PrimitiveShape::Box, {-7.42f, 1.18f, 0.0f}, {0.18f, 1.62f, 1.82f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Box, {-7.53f, 1.18f, 0.0f}, {0.10f, 1.34f, 1.52f}, Window);
        for (int bar = 0; bar < 7; ++bar) {
            const float z = -0.60f + static_cast<float>(bar) * 0.20f;
            Part(transform, drawPart, PrimitiveShape::Box, {-7.60f, 1.18f, z}, {0.08f, 1.22f, 0.07f}, Gold);
        }

        // 左右二灯ずつの丸型ライトを装甲ハウジングから覗かせる
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {-7.25f, 1.48f, side * 9.25f}, {0.32f, 0.72f, 0.76f}, MainBlack);
            for (float offset : {-0.18f, 0.18f}) {
                Part(transform, drawPart, PrimitiveShape::Cylinder, {-7.48f, 1.48f, side * 9.25f + offset},
                    {0.28f, 0.16f, 0.28f}, Light, 0.0f, Math::HalfPi);
            }
        }

        // ラム本体と三本の歯を一単位として将来の突進破壊へ分離する
        if (frontRam) {
            Part(transform, drawPart, PrimitiveShape::Prism, {-8.00f, -0.48f, 0.0f}, {1.65f, 0.76f, ModelWidth}, ArmorBlack);
            for (float side : {-1.0f, 0.0f, 1.0f}) {
                Part(transform, drawPart, PrimitiveShape::Cone, {-8.78f, -0.50f, side * 8.0f},
                    {0.42f, 1.15f, 0.42f}, HighlightBlack, 0.0f, Math::HalfPi);
            }
        }
    }

    /**
     * @brief 黒い大面積を分割する少量の金装飾を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawDecorations(const BossModelTransform& transform, DrawPart& drawPart) {
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {2.20f, 1.12f, side * 10.35f}, {4.30f, 0.11f, 0.09f}, Gold);
            Part(transform, drawPart, PrimitiveShape::Sphere, {-5.55f, 1.86f, side * 9.95f}, {0.38f, 0.38f, 0.12f}, Gold);
            Part(transform, drawPart, PrimitiveShape::Box, {4.80f, 1.52f, side * 9.98f}, {0.46f, 0.18f, 0.10f}, Light);
        }
        for (float x : {-4.70f, -1.80f, 1.10f, 4.00f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {x, 0.98f, 0.0f}, {1.45f, 0.10f, 19.4f}, Gold);
        }
    }
};

static_assert(Stage4BossModelView::PrimitiveCount == 160);
