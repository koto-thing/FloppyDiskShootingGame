#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
#include <string>
#endif

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
 * @brief テキストを画面内の代表的な位置へ配置する基準点
 *
 * Centerは画面中央、BottomCenterは画面下中央を表す
 */
enum class ScreenAlign {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/** @brief テキスト用の画面内配置基準 */
using TextAlign = ScreenAlign;
/** @brief 矩形用の画面内配置基準 */
using RectAlign = ScreenAlign;

/**
 * @brief Rendererがフレーム内に記録する描画コマンド
 */
struct RenderCommand {
    enum class Type {
        Circle,
        Rect,
        Primitive3D,
        PlayerShot,
        Explosion,
        Railgun,
        Text,
        Pipeline,
        SetCamera,
        ResetCamera
    };

    Type type = Type::Rect;
    Circle circle {};
    Rect rect {};
    Primitive3D primitive {};
    PlayerShotVisual playerShot {};
    ExplosionVisual explosion {};
    RailgunVisual railgun {};
    Vector2 position {};
    float size = 0.0f;
    float characterSpacing = 0.0f;
    ColorF color = ColorF::White();
    PipelineId pipeline = PipelineId::Object;
    CameraMatrices cameraMatrices {};
    Viewport viewport {};
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    static constexpr std::size_t TextCapacity = 512;
    std::array<char, TextCapacity> text {};
#else
    std::array<char, 128> text {};
#endif
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
    // Floppy版のビットマップ文字が隣接しないよう既定の字間を設ける
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    static constexpr float DefaultCharacterSpacing = 0.0f;
#else
    static constexpr float DefaultCharacterSpacing = 0.003f;
#endif

    /** @brief バックエンドなしの描画ファサードを生成する */
    Renderer() = default;
    /**
     * @brief 描画バックエンド付きの描画ファサードを生成する
     * @param backend 描画バックエンド
     */
    explicit Renderer(IRenderBackend& backend) : m_backend(&backend) {}

#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    /**
     * @brief ゲーム側の表示言語変換処理を登録する
     * @param translator UTF-8文字列を翻訳する関数、nullptrで無効化
     * @return なし
     */
    void SetTextTranslator(std::string (*translator)(std::string_view)) { m_textTranslator = translator; }
    /**
     * @brief 計測と描画に使う翻訳済み文字列を取得する
     * @param text 元のUTF-8文字列
     * @return 翻訳後の文字列
     */
    std::string ResolveText(std::string_view text) const {
        return m_textTranslator ? m_textTranslator(text) : std::string(text);
    }
#endif

    /** @brief フレームの描画記録を開始する */
    void BeginFrame();
    /** @brief 円を描画コマンドとして記録する */
    void Draw(const Circle& circle, const ColorF& color);
    /** @brief 矩形を描画コマンドとして記録する */
    void Draw(const Rect& rect, const ColorF& color);
    /**
     * @brief 矩形を画面内の代表的な位置へ描画コマンドとして記録する
     * @param rect sizeは矩形の大きさ、positionは配置位置からのNDC座標オフセット
     * @param alignment 矩形全体を配置する画面上の位置
     */
    void Draw(const Rect& rect, RectAlign alignment, const ColorF& color);
    /**
     * @brief 画面内の配置基準から矩形の境界を生成する
     * @param size 矩形のNDC座標上の大きさ
     * @param alignment 矩形全体を配置する画面上の位置
     * @param offset 配置位置からのNDC座標オフセット
     */
    static Rect CreateAlignedRect(const Vector2& size, RectAlign alignment,
                                  const Vector2& offset = Vector2::Zero);
    /** @brief 型付き3Dプリミティブを描画コマンドとして記録する */
    void Draw(const Primitive3D& primitive);
    /** @brief プロシージャル弾を描画コマンドとして記録する */
    void DrawPlayerShot(const PlayerShotVisual& shot);
    /** @brief プロシージャル爆発エフェクトを描画コマンドとして記録する */
    void DrawExplosion(const ExplosionVisual& explosion);
    /** @brief プロシージャルレールガン軌跡を描画コマンドとして記録する */
    void DrawRailgun(const RailgunVisual& railgun);
    /**
     * @brief 文字を描画コマンドとして記録する
     * @param characterSpacing 文字ごとに追加する字間
     */
    void DrawText(std::string_view text, const Vector2& position, float size, const ColorF& color,
                  float characterSpacing = DefaultCharacterSpacing);
    /**
     * @brief テキストを画面内の代表的な位置へ描画コマンドとして記録する
     * @param alignment テキスト全体を配置する画面上の位置
     * @param offset 配置位置からのNDC座標オフセット
     * @param characterSpacing 文字ごとに追加する字間
     */
    void DrawText(std::string_view text, TextAlign alignment, float size, const ColorF& color,
                  const Vector2& offset = Vector2::Zero, float characterSpacing = DefaultCharacterSpacing);
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
    /** @brief 低解像度のレトロ映像効果を切り替える */
    void SetRetroEffectEnabled(bool enabled) {
        if (m_backend != nullptr) m_backend->SetRetroEffectEnabled(enabled);
    }

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
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    /**
     * @brief 翻訳を重複させず表示文字列の描画コマンドを記録する
     * @param text 翻訳済み文字列
     * @param position 先頭文字の中心座標
     * @param size 文字の半サイズ
     * @param color 文字色
     * @param characterSpacing 追加の字間
     * @return なし
     */
    void RecordTextCommand(std::string_view text, const Vector2& position, float size, const ColorF& color,
                           float characterSpacing);
    std::string (*m_textTranslator)(std::string_view) = nullptr;
#endif
    /**
     * @brief 描画コマンドを記録領域へ追加する
     * @param type 追加するコマンド種別
     * @return 追加したコマンド、容量超過時はnullptr
     */
    RenderCommand* TryAppend(RenderCommand::Type type);
    /**
     * @brief 文字列の配置基準から先頭文字位置を計算する
     * @param text 描画する文字列
     * @param alignment 配置基準
     * @param size 文字サイズ
     * @param characterSpacing 文字間隔
     * @return 先頭文字の座標
     */
    Vector2 CalculateTextPosition(std::string_view text, TextAlign alignment, float size,
                                  float characterSpacing) const;

    IRenderBackend* m_backend = nullptr;
    std::unique_ptr<RenderCommand[]> m_commands = std::make_unique<RenderCommand[]>(MaxCommands);
    std::size_t m_commandCount = 0;
    std::size_t m_droppedCommandCount = 0;
    bool m_overflowed = false;
    bool m_flushed = false;
};
