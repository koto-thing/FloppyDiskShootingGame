#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

#include "Color.h"
#include "IRenderBackend.h"
#include "../Geometry/Circle.h"
#include "../Geometry/Rect.h"
#include "../Math/Vector2.h"
#include "Camera2D.h"
#include "Camera3D.h"

// Windows.hのDrawTextマクロがRendererの公開API名を置換しないようにする
#ifdef DrawText
#undef DrawText
#endif

/**
 * @brief Rendererがフレーム内に記録する描画コマンド
 */
struct RenderCommand {
    enum class Type {
        Circle,
        Rect,
        Primitive3D,
        Text,
        Pipeline,
        SetCamera,
        ResetCamera
    };

    Type type = Type::Rect;
    Circle circle {};
    Rect rect {};
    Primitive3D primitive {};
    Vector2 position {};
    float size = 0.0f;
    float characterSpacing = 0.0f;
    ColorF color = ColorF::White();
    PipelineId pipeline = PipelineId::Object;
    CameraMatrices cameraMatrices {};
    Viewport viewport {};
    std::array<char, 128> text {};
    std::size_t textLength = 0;
};

/**
 * @brief ゲームコード向けの軽量な描画ファサード
 *
 * 描画順を固定容量のコマンド領域へ記録し、DirectX 12の型を公開しない
 * Circle、Rect、Textの座標は画面中央を原点とするNDC座標で、表示範囲は概ね-1から1
 */
class Renderer {
public:
    static constexpr std::size_t MaxCommands = 4096;

    Renderer() = default;
    explicit Renderer(IRenderBackend& backend) : m_backend(&backend) {}

    /** @brief フレームの描画記録を開始する */
    void BeginFrame();
    /** @brief 円を描画コマンドとして記録する */
    void Draw(const Circle& circle, const ColorF& color);
    /** @brief 矩形を描画コマンドとして記録する */
    void Draw(const Rect& rect, const ColorF& color);
    /** @brief 型付き3Dプリミティブを描画コマンドとして記録する */
    void Draw(const Primitive3D& primitive);
    /**
     * @brief 文字を描画コマンドとして記録する
     * @param characterSpacing 文字ごとに追加する字間
     */
    void DrawText(std::string_view text, const Vector2& position, float size, const ColorF& color,
                  float characterSpacing = 0.0f);
    /** @brief 型付きパイプライン切り替えを記録する */
    void SetPipeline(PipelineId pipeline);
    /** @brief 2Dカメラを遅延設定する */
    void SetCamera(const Camera2D& camera);
    /** @brief 3Dカメラを遅延設定する */
    void SetCamera(const Camera3D& camera);
    /** @brief カメラを解除してUI座標系へ戻す */
    void ResetCamera();
    /** @brief 記録済みコマンドを登録順にバックエンドへ送る */
    void Flush();
    /** @brief フレームの描画を終了する */
    void EndFrame();

    /** @brief 記録済みコマンド数を取得する */
    std::size_t CommandCount() const { return m_commandCount; }
    /** @brief 容量超過で破棄したコマンド数を取得する */
    std::size_t DroppedCommandCount() const { return m_droppedCommandCount; }
    /** @brief 現フレームで容量超過が発生したか取得する */
    bool HasOverflowed() const { return m_overflowed; }
    /** @brief バックエンドの画面幅を取得する */
    int Width() const { return m_backend == nullptr ? 0 : m_backend->Width(); }
    /** @brief バックエンドの画面高さを取得する */
    int Height() const { return m_backend == nullptr ? 0 : m_backend->Height(); }
    /** @brief バックエンドの画面アスペクト比を取得する */
    float AspectRatio() const { return m_backend == nullptr ? 1.0f : m_backend->AspectRatio(); }
    /** @brief 記録済みコマンドを取得する */
    const RenderCommand& Command(std::size_t index) const { return m_commands[index]; }

private:
    RenderCommand* TryAppend(RenderCommand::Type type);

    IRenderBackend* m_backend = nullptr;
    std::unique_ptr<RenderCommand[]> m_commands = std::make_unique<RenderCommand[]>(MaxCommands);
    std::size_t m_commandCount = 0;
    std::size_t m_droppedCommandCount = 0;
    bool m_overflowed = false;
    bool m_flushed = false;
};
