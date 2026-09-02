#pragma once

#include <cmath>
#include <cstdint>
#include <initializer_list>

#include "../Common/BossModelTransform.h"
#include "../../../../Engine/Math/Math.h"

/** @brief Stage3封鎖バリアの面 */
enum class Stage3BarrierFace : std::uint8_t {
    Front = 1 << 0,
    Rear = 1 << 1,
    Left = 1 << 2,
    Right = 1 << 3,
    Bottom = 1 << 4
};

/** @brief Stage3封鎖バリアの表示姿勢 */
struct Stage3BarrierCagePose {
    float openAmount = 1.0f;
    float alpha = 1.0f;
    float scrollOffset = 0.0f;
    float flicker = 0.0f;
    std::uint8_t visibleFaces = 0x1f;
};

/** @brief Stage3ボス下部の5面封鎖バリア描画 */
class Stage3BarrierCageView final {
public:
    static constexpr float BarrierHalfLength = 7.8f;
    static constexpr float BarrierHalfWidth = 2.8f;
    static constexpr float BarrierHeight = 5.6f;
    static constexpr float BarrierTopY = -5.45f;
    static constexpr int FramePrimitiveCount = 20;
    static constexpr int FieldPrimitiveCount = 5;
    static constexpr int MarqueeStripPrimitiveCount = 4;
    static constexpr int MarqueeDotPrimitiveCountPerFace = 132;
    static constexpr int MarqueeDotPrimitiveCount = MarqueeDotPrimitiveCountPerFace * 4;

    /**
     * @brief 表示対象面をビットマスクへ変換する
     * @param face 変換する面
     * @return visibleFacesへ指定するビット
     */
    static constexpr std::uint8_t FaceBit(Stage3BarrierFace face) {
        return static_cast<std::uint8_t>(face);
    }

    /**
     * @brief 有効な5面を描画する
     * @param transform ボスと共有する親Transform
     * @param pose 開度、透明度、マーキー位置、表示面
     * @param drawPart Primitive描画関数
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, const Stage3BarrierCagePose& pose,
        DrawPart&& drawPart) {
        // 開度0ではPrimitiveを発行しない
        if (Math::Clamp01(pose.openAmount) <= 0.0f || Math::Clamp01(pose.alpha) <= 0.0f) return;

        // 各面を独立して描画し将来の部分消失へ接続する
        for (Stage3BarrierFace face : {Stage3BarrierFace::Front, Stage3BarrierFace::Rear,
                Stage3BarrierFace::Left, Stage3BarrierFace::Right, Stage3BarrierFace::Bottom}) {
            if ((pose.visibleFaces & FaceBit(face)) != 0) DrawFace(face, transform, pose, drawPart);
        }
    }

    /**
     * @brief 指定した1面をフィールド、フレーム、マーキーに分けて描画する
     * @param face 描画する面
     * @param transform ボスと共有する親Transform
     * @param pose 開度、透明度、マーキー位置
     * @param drawPart Primitive描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawFace(Stage3BarrierFace face, const BossModelTransform& transform,
        const Stage3BarrierCagePose& pose, DrawPart&& drawPart) {
        const float amount = Math::Clamp01(pose.openAmount);
        const float alpha = Math::Clamp01(pose.alpha) * amount;
        if (amount <= 0.0f || alpha <= 0.0f) return;

        // 上端を固定してフィールドを下方向へ展開する
        const float height = BarrierHeight * amount;
        const float centerY = BarrierTopY - height * 0.5f;
        const float bottomY = BarrierTopY - height;
        const float pulse = 0.86f + Math::Clamp01(pose.flicker) * 0.14f;
        const float fieldColor[4] = {0.04f * pulse, 0.55f * pulse, 0.82f * pulse, 0.28f * alpha};
        const float frameColor[4] = {0.30f * pulse, 0.90f * pulse, 1.00f, 0.92f * alpha};

        // 面ごとの薄いフィールドと四辺の発光フレームを描画する
        DrawField(face, transform, centerY, bottomY, height, fieldColor, drawPart);
        DrawFrame(face, transform, centerY, bottomY, height, frameColor, drawPart);
        if (face != Stage3BarrierFace::Bottom) {
            DrawMarquee(face, transform, pose.scrollOffset, centerY, alpha, pulse, drawPart);
        }
    }

private:
    static constexpr float FrameThickness = 0.10f;
    static constexpr float FieldThickness = 0.035f;
    static constexpr float MarqueeHeight = 0.96f;
    static constexpr float MarqueeThickness = 0.055f;
    static constexpr int GlyphWidth = 5;
    static constexpr int GlyphHeight = 7;
    static constexpr int GlyphAdvance = 6;
    inline static constexpr char MarqueeText[] = "PLAY BALL!!!";
    static constexpr int MarqueeCharacterCount = sizeof(MarqueeText) - 1;
    static constexpr int MarqueeColumnCount = MarqueeCharacterCount * GlyphAdvance;

    /**
     * @brief 面の半透明フィールドを描画する
     * @param face 描画する面
     * @param transform 親Transform
     * @param centerY 展開済み領域の中心Y
     * @param bottomY 展開済み領域の下端Y
     * @param height 展開済み領域の高さ
     * @param color RGBA色
     * @param drawPart Primitive描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawField(Stage3BarrierFace face, const BossModelTransform& transform,
        float centerY, float bottomY, float height, const float color[4], DrawPart& drawPart) {
        switch (face) {
        case Stage3BarrierFace::Front:
        case Stage3BarrierFace::Rear:
            Part(transform, drawPart,
                {(face == Stage3BarrierFace::Front ? -1.0f : 1.0f) * BarrierHalfLength, centerY, 0.0f},
                {FieldThickness, height, BarrierHalfWidth * 2.0f}, color);
            break;
        case Stage3BarrierFace::Left:
        case Stage3BarrierFace::Right:
            Part(transform, drawPart,
                {0.0f, centerY, (face == Stage3BarrierFace::Left ? -1.0f : 1.0f) * BarrierHalfWidth},
                {BarrierHalfLength * 2.0f, height, FieldThickness}, color);
            break;
        case Stage3BarrierFace::Bottom:
            Part(transform, drawPart, {0.0f, bottomY, 0.0f},
                {BarrierHalfLength * 2.0f, FieldThickness, BarrierHalfWidth * 2.0f}, color);
            break;
        }
    }

    /**
     * @brief 面の四辺へ発光フレームを描画する
     * @param face 描画する面
     * @param transform 親Transform
     * @param centerY 展開済み領域の中心Y
     * @param bottomY 展開済み領域の下端Y
     * @param height 展開済み領域の高さ
     * @param color RGBA色
     * @param drawPart Primitive描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawFrame(Stage3BarrierFace face, const BossModelTransform& transform,
        float centerY, float bottomY, float height, const float color[4], DrawPart& drawPart) {
        if (face == Stage3BarrierFace::Front || face == Stage3BarrierFace::Rear) {
            const float x = (face == Stage3BarrierFace::Front ? -1.0f : 1.0f) * BarrierHalfLength;
            for (float y : {BarrierTopY, bottomY}) {
                Part(transform, drawPart, {x, y, 0.0f},
                    {FrameThickness, FrameThickness, BarrierHalfWidth * 2.0f}, color);
            }
            for (float z : {-BarrierHalfWidth, BarrierHalfWidth}) {
                Part(transform, drawPart, {x, centerY, z},
                    {FrameThickness, height, FrameThickness}, color);
            }
            return;
        }
        if (face == Stage3BarrierFace::Left || face == Stage3BarrierFace::Right) {
            const float z = (face == Stage3BarrierFace::Left ? -1.0f : 1.0f) * BarrierHalfWidth;
            for (float y : {BarrierTopY, bottomY}) {
                Part(transform, drawPart, {0.0f, y, z},
                    {BarrierHalfLength * 2.0f, FrameThickness, FrameThickness}, color);
            }
            for (float x : {-BarrierHalfLength, BarrierHalfLength}) {
                Part(transform, drawPart, {x, centerY, z},
                    {FrameThickness, height, FrameThickness}, color);
            }
            return;
        }

        // 下面は矩形外周を4本のフレームで囲う
        for (float z : {-BarrierHalfWidth, BarrierHalfWidth}) {
            Part(transform, drawPart, {0.0f, bottomY, z},
                {BarrierHalfLength * 2.0f, FrameThickness, FrameThickness}, color);
        }
        for (float x : {-BarrierHalfLength, BarrierHalfLength}) {
            Part(transform, drawPart, {x, bottomY, 0.0f},
                {FrameThickness, FrameThickness, BarrierHalfWidth * 2.0f}, color);
        }
    }

    /**
     * @brief 面上へPLAY BALLのドットマトリクス帯を描画する
     * @param face 描画する面
     * @param transform 親Transform
     * @param scrollOffset 文字列のスクロール列位置
     * @param centerY 展開済み領域の中心Y
     * @param alpha 合成済み透明度
     * @param pulse 点滅輝度
     * @param drawPart Primitive描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMarquee(Stage3BarrierFace face, const BossModelTransform& transform,
        float scrollOffset, float centerY, float alpha, float pulse, DrawPart& drawPart) {
        const bool endFace = face == Stage3BarrierFace::Front || face == Stage3BarrierFace::Rear;
        const float halfSpan = endFace ? BarrierHalfWidth : BarrierHalfLength;
        const float direction = face == Stage3BarrierFace::Rear || face == Stage3BarrierFace::Right ? -1.0f : 1.0f;
        const float stripColor[4] = {0.01f, 0.07f, 0.11f, 0.58f * alpha};
        const float dotColor[4] = {0.78f * pulse, 1.00f, 0.42f * pulse, 0.98f * alpha};
        const float stripY = centerY + BarrierHeight * 0.23f;

        // 読みやすい暗色帯をフィールド表面よりわずかに外側へ置く
        if (endFace) {
            const float x = (face == Stage3BarrierFace::Front ? -1.0f : 1.0f) *
                (BarrierHalfLength + MarqueeThickness);
            Part(transform, drawPart, {x, stripY, 0.0f},
                {MarqueeThickness, MarqueeHeight, BarrierHalfWidth * 2.0f}, stripColor);
        } else {
            const float z = (face == Stage3BarrierFace::Left ? -1.0f : 1.0f) *
                (BarrierHalfWidth + MarqueeThickness);
            Part(transform, drawPart, {0.0f, stripY, z},
                {BarrierHalfLength * 2.0f, MarqueeHeight, MarqueeThickness}, stripColor);
        }

        // 5x7ビット列を面幅へ収め、列単位の剰余で途切れなく周回させる
        const float columnStep = halfSpan * 2.0f / static_cast<float>(MarqueeColumnCount);
        float wrappedScroll = std::fmod(scrollOffset * direction, static_cast<float>(MarqueeColumnCount));
        if (wrappedScroll < 0.0f) wrappedScroll += static_cast<float>(MarqueeColumnCount);
        for (int characterIndex = 0; characterIndex < MarqueeCharacterCount; ++characterIndex) {
            for (int row = 0; row < GlyphHeight; ++row) {
                const std::uint8_t bits = GlyphRow(MarqueeText[characterIndex], row);
                for (int column = 0; column < GlyphWidth; ++column) {
                    if ((bits & (1 << (GlyphWidth - 1 - column))) == 0) continue;
                    float displayColumn = std::fmod(
                        static_cast<float>(characterIndex * GlyphAdvance + column) + wrappedScroll,
                        static_cast<float>(MarqueeColumnCount));
                    const float horizontal = -halfSpan + (displayColumn + 0.5f) * columnStep;
                    const float y = stripY + (3.0f - static_cast<float>(row)) * 0.115f;
                    if (endFace) {
                        const float x = (face == Stage3BarrierFace::Front ? -1.0f : 1.0f) *
                            (BarrierHalfLength + MarqueeThickness * 2.0f);
                        Part(transform, drawPart, {x, y, horizontal},
                            {MarqueeThickness, 0.085f, columnStep * 0.72f}, dotColor);
                    } else {
                        const float z = (face == Stage3BarrierFace::Left ? -1.0f : 1.0f) *
                            (BarrierHalfWidth + MarqueeThickness * 2.0f);
                        Part(transform, drawPart, {horizontal, y, z},
                            {columnStep * 0.72f, 0.085f, MarqueeThickness}, dotColor);
                    }
                }
            }
        }
    }

    /**
     * @brief 5x7文字の指定行を取得する
     * @param character ASCII文字
     * @param row 0以上6以下の行
     * @return 左端をbit4とする点灯ビット列
     */
    static constexpr std::uint8_t GlyphRow(char character, int row) {
        switch (character) {
        case 'P': { constexpr std::uint8_t rows[] = {30, 17, 17, 30, 16, 16, 16}; return rows[row]; }
        case 'L': { constexpr std::uint8_t rows[] = {16, 16, 16, 16, 16, 16, 31}; return rows[row]; }
        case 'A': { constexpr std::uint8_t rows[] = {14, 17, 17, 31, 17, 17, 17}; return rows[row]; }
        case 'Y': { constexpr std::uint8_t rows[] = {17, 17, 10, 4, 4, 4, 4}; return rows[row]; }
        case 'B': { constexpr std::uint8_t rows[] = {30, 17, 17, 30, 17, 17, 30}; return rows[row]; }
        case '!': { constexpr std::uint8_t rows[] = {4, 4, 4, 4, 4, 0, 4}; return rows[row]; }
        default: return 0;
        }
    }

    /**
     * @brief ローカル部品を親Transformへ合成してBoxを描画する
     * @param transform 親Transform
     * @param drawPart Primitive描画関数
     * @param localPosition モデルローカル座標
     * @param scale ローカル寸法
     * @param color RGBA色
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart,
        const Vector3& localPosition, const Vector3& scale, const float color[4]) {
        // Stage3ボスと同じYaw合成でローカル座標をワールドへ変換する
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 position {
            transform.position.x + (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z + (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
        drawPart(1, position, scale * transform.scale, color, transform.yaw, 0.0f);
    }
};

static_assert(Stage3BarrierCageView::FramePrimitiveCount == 20);
static_assert(Stage3BarrierCageView::FieldPrimitiveCount == 5);
