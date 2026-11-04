#pragma once

#include <memory>
#include "../../Engine/Graphics/IRenderBackend.h"

struct SDL_Window;

/** @brief VulkanとWebGL 2で既存の描画コマンドを実行する */
class PortableRenderingService final : public IRenderBackend {
public:
    /** @brief 描画サービスを生成する */
    PortableRenderingService();
    /** @brief GPUリソースを解放する */
    ~PortableRenderingService() override;
    /** @brief 描画を初期化する @param window SDLウィンドウ @param width 描画幅 @param height 描画高さ @return 成功時true */
    bool Initialize(SDL_Window* window, int width, int height);
    /** @brief GPUリソースを解放する */
    void Cleanup();
    /** @brief 次のフレームの描画寸法を設定する @param width 描画幅 @param height 描画高さ */
    void Resize(int width, int height);
    /** @brief フレームの記録を開始する */
    void BeginFrame() override;
    /** @brief 記録したフレームを描画する */
    void EndFrame() override;
    /** @brief NDC円を描画する @param circle 円 @param color 色 */
    void DrawCircle(const Circle& circle, const ColorF& color) override;
    /** @brief NDC矩形を描画する @param rect 矩形 @param color 色 */
    void DrawRect(const Rect& rect, const ColorF& color) override;
    /** @brief 3Dプリミティブを描画する @param primitive 描画情報 */
    void DrawPrimitive3D(const Primitive3D& primitive) override;
    /** @brief 弾を描画する @param shot 描画情報 */
    void DrawPlayerShot(const PlayerShotVisual& shot) override;
    /** @brief 爆発を描画する @param explosion 描画情報 */
    void DrawExplosion(const ExplosionVisual& explosion) override;
    /** @brief レールガン軌跡を描画する @param railgun 描画情報 */
    void DrawRailgun(const RailgunVisual& railgun) override;
    /** @brief ASCII文字を描画する @param text 文字列 @param position NDC位置 @param size 文字サイズ @param color 色 @param characterSpacing 字間 */
    void DrawTextCommand(std::string_view text, const Vector2& position, float size,
        const ColorF& color, float characterSpacing) override;
    /** @brief パイプラインを選ぶ @param pipeline パイプライン */
    void SetPipeline(PipelineId pipeline) override;
    /** @brief 描画領域を設定する @param matrices カメラ行列 @param viewport 描画領域 */
    void SetCamera(const CameraMatrices& matrices, const Viewport& viewport) override;
    /** @brief ワールドを拡大しHUDの描画を開始する */
    void ResetCamera() override;
    /** @brief レトロ効果を切り替える @param enabled 有効状態 */
    void SetRetroEffectEnabled(bool enabled) override;
    /** @brief 描画幅を取得する @return ピクセル数 */
    int Width() const override;
    /** @brief 描画高さを取得する @return ピクセル数 */
    int Height() const override;
    /** @brief 描画領域の縦横比を取得する @return 幅/高さ */
    float AspectRatio() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
