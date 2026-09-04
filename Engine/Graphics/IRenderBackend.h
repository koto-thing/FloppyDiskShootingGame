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
    SpellCircle,
    Model3D
};

/** @brief Rendererへ渡す3D形状の種類 */
enum class PrimitiveShape { Plate, Box, Sphere, Cylinder, Cone, Prism, Sprite2D };

/**
 * @brief 旧モデルの形状番号を現在のPrimitiveShapeへ変換する
 * @param shape 0=Plate、1=Box、2=Cylinder、3=Cone、4=Prism、5=Sphereの旧番号
 * @return 対応するPrimitiveShape
 */
constexpr PrimitiveShape PrimitiveShapeFromLegacyIndex(int shape) {
    return shape == 0 ? PrimitiveShape::Plate :
        shape == 1 ? PrimitiveShape::Box :
        shape == 2 ? PrimitiveShape::Cylinder :
        shape == 3 ? PrimitiveShape::Cone :
        shape == 4 ? PrimitiveShape::Prism : PrimitiveShape::Sphere;
}

/** @brief 形状、変換、色をまとめた3D描画命令 */
struct Primitive3D {
    PrimitiveShape shape = PrimitiveShape::Box;
    Matrix4x4 wvpMatrix = Matrix4x4::Identity;
    Vector3 scale = Vector3::One;
    ColorF color = ColorF::White();
    float rotationAngle = 0.0f;
};

/** @brief プロシージャル弾の描画情報 */
struct PlayerShotVisual {
    Vector2 position {};
    Vector2 size {};
    float direction = 0.0f;
    float time = 0.0f;
    int type = 0;
    float depth = 0.0f;
    bool depthTest = false;
};

/** @brief HLSLで描画する爆発エフェクトの描画情報 */
struct ExplosionVisual {
    Matrix4x4 wvpMatrix = Matrix4x4::Identity;
    float progress = 0.0f;
    int effectType = 0;
};

/** @brief HLSLで描画するレールガン軌跡の描画情報 */
struct RailgunVisual {
    Matrix4x4 wvpMatrix = Matrix4x4::Identity;
    float progress = 0.0f;
    int effectType = 0;
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
    /** @brief HLSLで生成する弾を描画する */
    virtual void DrawPlayerShot(const PlayerShotVisual& shot) { (void)shot; }
    /** @brief HLSLで生成する爆発エフェクトを描画する */
    virtual void DrawExplosion(const ExplosionVisual& explosion) { (void)explosion; }
    /** @brief HLSLで生成するレールガン軌跡を描画する */
    virtual void DrawRailgun(const RailgunVisual& railgun) { (void)railgun; }
    /**
     * @brief 文字をNDC座標系へ描画する
     * @param characterSpacing 文字ごとに追加する字間
     */
    virtual void DrawTextCommand(std::string_view text, const Vector2& position, float size, const ColorF& color,
                                 float characterSpacing) = 0;
    /** @brief パイプラインを切り替える */
    virtual void SetPipeline(PipelineId pipeline) = 0;
    /** @brief カメラ行列とViewportを描画状態へ設定する */
    virtual void SetCamera(const CameraMatrices& matrices, const Viewport& viewport) { (void)matrices; (void)viewport; }
    /** @brief カメラを解除してUI描画状態へ戻す */
    virtual void ResetCamera() {}
    /** @brief バックエンドのフレーム終了処理を行う */
    virtual void EndFrame() = 0;
    /** @brief 低解像度のレトロ映像効果を切り替える */
    virtual void SetRetroEffectEnabled(bool enabled) { (void)enabled; }
    /** @brief 描画領域の幅を取得する */
    virtual int Width() const { return 0; }
    /** @brief 描画領域の高さを取得する */
    virtual int Height() const { return 0; }
    /** @brief 描画領域のアスペクト比を取得する */
    virtual float AspectRatio() const { return 1.0f; }
};
