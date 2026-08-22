#pragma once

#include <string_view>

#include "../Geometry/Circle.h"
#include "../Geometry/Rect.h"
#include "../Math/Matrix4x4.h"
#include "../Math/Vector3.h"
#include "CameraMatrices.h"
#include "Viewport.h"
#include "Color.h"

/**
 * @brief Rendererが利用する描画パイプラインの識別子
 */
enum class PipelineId {
    Object,
    Background,
    SpellCircle
};

/** @brief Rendererへ渡す3D形状の種類 */
enum class PrimitiveShape { Plate, Box, Sphere, Cylinder, Cone, Prism, Sprite2D };

/** @brief 形状、変換、色をまとめた3D描画命令 */
struct Primitive3D {
    PrimitiveShape shape = PrimitiveShape::Box;
    Matrix4x4 wvpMatrix = Matrix4x4::Identity;
    Vector3 scale = Vector3::One;
    ColorF color = ColorF::White();
    float rotationAngle = 0.0f;
};

/**
 * @brief Rendererの記録済みコマンドを実行するバックエンド境界
 *
 * このインターフェースはゲームコードへGPU APIの型を公開しない
 */
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    /** @brief バックエンドのフレーム開始処理を行う */
    virtual void BeginFrame() = 0;
    /** @brief 円をNDC座標系へ描画する */
    virtual void DrawCircle(const Circle& circle, const ColorF& color) = 0;
    /** @brief 矩形をNDC座標系へ描画する */
    virtual void DrawRect(const Rect& rect, const ColorF& color) = 0;
    /** @brief 型付き3Dプリミティブを描画する */
    virtual void DrawPrimitive3D(const Primitive3D& primitive) { (void)primitive; }
    /** @brief 文字をNDC座標系へ描画する */
    virtual void DrawTextCommand(std::string_view text, const Vector2& position, float size, const ColorF& color) = 0;
    /** @brief パイプラインを切り替える */
    virtual void SetPipeline(PipelineId pipeline) = 0;
    /** @brief カメラ行列とViewportを描画状態へ設定する */
    virtual void SetCamera(const CameraMatrices& matrices, const Viewport& viewport) { (void)matrices; (void)viewport; }
    /** @brief カメラを解除してUI描画状態へ戻す */
    virtual void ResetCamera() {}
    /** @brief バックエンドのフレーム終了処理を行う */
    virtual void EndFrame() = 0;
    /** @brief 描画領域の幅を取得する */
    virtual int Width() const { return 0; }
    /** @brief 描画領域の高さを取得する */
    virtual int Height() const { return 0; }
    /** @brief 描画領域のアスペクト比を取得する */
    virtual float AspectRatio() const { return 1.0f; }
};
