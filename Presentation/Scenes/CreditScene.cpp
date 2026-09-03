#include "CreditScene.h"

#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Time/Time.h"
#include "../../Infrastructure/ExternalServices/InputService.h"
#include "../Common/SpaceBackground.h"

namespace {
constexpr float MeteorGravity = -0.55f;
constexpr float MeteorBounceSpeed = 0.43f;
constexpr float DefaultMeteorRadius = 0.12f;
constexpr unsigned int MeteorMaximumShrinkLevel = 4;

/**
 * @brief 縮小段階に対応する隕石半径を取得する
 * @param shrinkLevel 0から4までの縮小段階
 * @return 指定段階まで縮小した半径
 */
constexpr float MeteorRadius(unsigned int shrinkLevel) {
    return DefaultMeteorRadius * (1.0f - static_cast<float>(shrinkLevel) * 0.12f);
}

static_assert(MeteorRadius(0) == DefaultMeteorRadius &&
    MeteorRadius(MeteorMaximumShrinkLevel) < DefaultMeteorRadius * 0.53f);

/**
 * @brief 現在のクライアント領域に対応するUI入力を取得する
 * @return NDC座標へ変換済みのUI入力状態
 */
UIInputState CurrentUIInput() {
    int width = 1280;
    int height = 720;
    if (const HWND window = GetForegroundWindow()) {
        RECT rect {};
        GetClientRect(window, &rect);
        if (rect.right - rect.left > 0) width = rect.right - rect.left;
        if (rect.bottom - rect.top > 0) height = rect.bottom - rect.top;
    }
    return UIInput::Current(width, height);
}
}

CreditScene::~CreditScene() {
    Dispose();
}

/**
 * @brief クレジット用GameObjectとスクロールコンポーネントを生成する
 */
void CreditScene::Initialize() {
    // 左下にタイトルシーンへ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "BACK TO TITLE"
    );
    m_backButton->SetOnClick([this]() { changeScene(SceneType::Title); });

    // クレジット全体を移動させるGameObjectを生成する
    m_creditObject = std::make_unique<GameObject>();
    m_creditObject->SetName("CreditContent");

    // GameObjectへクレジットのスクロールコンポーネントを追加する
    m_creditController = m_creditObject->AddComponent<CreditContentController>();
    m_creditController->SetCreditContent(CreditSceneContent::CreateDefault());
    m_creditController->SetScrollSpeed(0.1f);

    // クレジットの先頭を画面下部の外側に配置する
    m_creditObject->SetPosition({ 0.0f, -1.1f, 0.0f });

    // 隕石を画面上部から時間差で落下させる
    for (std::size_t i = 0; i < m_meteors.size(); ++i) {
        ResetMeteor(i, true);
    }
}

/**
 * @brief クレジットシーンの入力を処理する
 */
void CreditScene::ProcessInput() {
    const UIInputState input = CurrentUIInput();

    // 左下のタイトルへ戻るボタンにマウス入力を渡す
    if (m_backButton != nullptr) {
        m_backButton->Update(input);
    }

    // 押した瞬間に手前の隕石を小さくして跳ね返す
    if (input.primaryPressed &&
        (m_backButton == nullptr || !m_backButton->Bounds().Contains(input.position))) {
        HitMeteor(input.position);
    }

    // Escapeキーでタイトル画面へ戻る
    if (InputService::IsKeyPressed(VK_ESCAPE)) {
        changeScene(SceneType::Title);
    }
}

/**
 * @brief クレジット用GameObjectを更新する
 */
void CreditScene::Tick() {
    // 所属コンポーネントのTickによりクレジットを上方向へスクロールする
    if (m_creditObject != nullptr) {
        m_creditObject->Tick();
    }

    // 隕石を重力で落下させる
    UpdateMeteors();

    // 最後まで流れたらタイトル画面へ戻る
    if (m_creditController != nullptr && m_creditController->IsCreditEnd()) {
        changeScene(SceneType::Title);
    }
}

/**
 * @brief クレジットシーンで生成したオブジェクトを解放する
 */
void CreditScene::Dispose() {
    // GameObjectが所有するコンポーネントを先に終了する
    if (m_creditObject != nullptr) {
        m_creditObject->Dispose();
    }

    m_creditController.reset();
    m_creditObject.reset();
    m_backButton.reset();
}

/**
 * @brief クレジットを現在のスクロール位置で描画する
 * @param renderer 描画コマンドを記録するRenderer
 */
void CreditScene::Render(Renderer& renderer) {
    // 初期化前または終了後は描画しない
    if (m_creditObject == nullptr || m_creditController == nullptr) return;

    // ゆっくり明滅する星空をクレジットの背面へ描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);

    // クレジット文字の背面へ落下中の隕石を描画する
    RenderMeteors(renderer);

    // クレジットの描画は専用プレゼンターへ委譲する
    m_creditPresenter.Render(renderer, *m_creditObject, m_creditController->GetCreditContent());

    // 左下のタイトルへ戻るボタンを描画する
    if (m_backButton != nullptr) {
        m_backButton->Render(renderer);
    }
}

/**
 * @brief 指定した隕石を画面上部へ再配置する
 * @param index 再配置する隕石の添字
 * @param initial trueの場合は初期表示用に落下開始位置をずらす
 */
void CreditScene::ResetMeteor(std::size_t index, bool initial) {
    // 軽量な線形合同法で配置と落下速度を散らす
    const auto random01 = [this]() {
        m_meteorRandomState = m_meteorRandomState * 1664525u + 1013904223u;
        return static_cast<float>(m_meteorRandomState >> 8) / 16777215.0f;
    };

    Meteor& meteor = m_meteors[index];
    meteor.position.x = -0.88f + random01() * 1.76f;
    meteor.position.y = 1.15f + random01() * 0.35f +
        (initial ? static_cast<float>(index) * 0.85f : 0.0f);
    meteor.velocity = { -0.025f + random01() * 0.05f, -0.12f - random01() * 0.12f };
    const unsigned int randomShrinkLevel = static_cast<unsigned int>(random01() * 5.0f);
    meteor.shrinkLevel = randomShrinkLevel > MeteorMaximumShrinkLevel ?
        MeteorMaximumShrinkLevel : randomShrinkLevel;
    meteor.radius = MeteorRadius(meteor.shrinkLevel);
}

/** @brief 全隕石の落下と跳ね返りを更新する */
void CreditScene::UpdateMeteors() {
    // 画面下へ抜けた隕石は上部へ戻して循環させる
    for (std::size_t i = 0; i < m_meteors.size(); ++i) {
        Meteor& meteor = m_meteors[i];
        meteor.velocity.y += MeteorGravity * Time::fixedDeltaTime;
        meteor.position += meteor.velocity * Time::fixedDeltaTime;
        if (meteor.position.y + meteor.radius < -1.05f) ResetMeteor(i, false);
    }
}

/**
 * @brief 指定位置にある隕石を小さくして上へ跳ね返す
 * @param position クリックしたNDC座標
 */
void CreditScene::HitMeteor(const Vector2& position) {
    // 描画順が手前の隕石から1個だけ反応させる
    for (std::size_t i = m_meteors.size(); i-- > 0;) {
        Meteor& meteor = m_meteors[i];
        if (!Circle { meteor.position, meteor.radius }.Contains(position)) continue;

        // 4段階まで縮んだ隕石は消滅させ、画面外から次の隕石を待機させる
        if (meteor.shrinkLevel >= MeteorMaximumShrinkLevel) {
            ResetMeteor(i, false);
            return;
        }

        // クリックごとに1段階縮小して上方向へ跳ね返す
        ++meteor.shrinkLevel;
        meteor.radius = MeteorRadius(meteor.shrinkLevel);
        meteor.velocity.y = MeteorBounceSpeed;
        return;
    }
}

/**
 * @brief 全隕石を描画する
 * @param renderer 描画コマンドを記録するRenderer
 */
void CreditScene::RenderMeteors(Renderer& renderer) const {
    constexpr ColorF MeteorColor {0.34f, 0.23f, 0.16f, 1.0f};
    constexpr ColorF MeteorLightColor {0.52f, 0.34f, 0.20f, 1.0f};
    constexpr ColorF MeteorShadowColor {0.19f, 0.12f, 0.10f, 1.0f};
    constexpr ColorF CraterColor {0.10f, 0.06f, 0.05f, 1.0f};

    // 塗りつぶし矩形を重ねて低解像度の隕石シルエットを表現する
    for (const Meteor& meteor : m_meteors) {
        const float radius = meteor.radius;
        renderer.Draw(Rect {meteor.position + Vector2 {0.0f, radius * 0.58f},
            {radius * 1.05f, radius * 0.40f}}, MeteorLightColor);
        renderer.Draw(Rect {meteor.position + Vector2 {-radius * 0.12f, radius * 0.25f},
            {radius * 1.65f, radius * 0.48f}}, MeteorColor);
        renderer.Draw(Rect {meteor.position,
            {radius * 1.95f, radius * 0.58f}}, MeteorColor);
        renderer.Draw(Rect {meteor.position + Vector2 {radius * 0.08f, -radius * 0.36f},
            {radius * 1.55f, radius * 0.46f}}, MeteorShadowColor);
        renderer.Draw(Rect {meteor.position + Vector2 {-radius * 0.12f, -radius * 0.68f},
            {radius * 0.82f, radius * 0.28f}}, MeteorShadowColor);
        renderer.Draw(Rect {meteor.position + Vector2 {-radius * 0.38f, radius * 0.17f},
            {radius * 0.30f, radius * 0.24f}}, CraterColor);
        renderer.Draw(Rect {meteor.position + Vector2 {radius * 0.32f, -radius * 0.22f},
            {radius * 0.22f, radius * 0.18f}}, CraterColor);
    }
}
