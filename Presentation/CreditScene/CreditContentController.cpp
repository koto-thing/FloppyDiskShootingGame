#include "CreditContentController.h"

#include "../../Domain/Entities/GameObject.h"
#include "../../Engine/Time/Time.h"

#include <utility>

/**
 * @brief クレジット表示コントローラーを生成する
 */
CreditContentController::CreditContentController()
    : m_initialPosition(Vector3::Zero),
      m_scrollSpeed(0.1f),
      m_creditEndOffset(2.0f),
      m_scrollOffset(0.0f),
      m_isCreditEnd(false) {
}

/**
 * @brief GameObjectにアタッチされた後の初期位置を記録する
 * @param renderer 描画サービスの参照
 */
void CreditContentController::Initialize(D3D12RenderingService& renderer) {
    // このコンポーネントは描画サービスを直接使用しない
    (void)renderer;

    // スクロール開始位置として所属GameObjectの現在位置を保存
    m_initialPosition = gameObject().GetPosition();
}

/**
 * @brief クレジットのスクロール状態を更新する
 */
void CreditContentController::Tick() {
    // 終了後はGameObjectを移動しません
    if (m_isCreditEnd) return;

    // GameObjectとともにクレジットを上方向へ移動
    MoveCredit();
    m_isCreditEnd = IsCreditEnd();
}

/**
 * @brief クレジットの表示内容を設定する
 * @param creditContent 表示するクレジット内容
 */
void CreditContentController::SetCreditContent(CreditSceneContent creditContent) {
    // 内容を差し替えた時点でスクロール状態を初期化
    m_creditContent = std::move(creditContent);

    // 先頭が下端外から上端外へ抜ける距離を内容の長さから算出
    m_creditEndOffset = m_creditContent.GetScrollLength() + 2.2f;
    ResetCredit();
}

/**
 * @brief 設定されているクレジットの表示内容を取得する
 * @return クレジットの表示内容
 */
const CreditSceneContent& CreditContentController::GetCreditContent() const {
    return m_creditContent;
}

/**
 * @brief クレジットのスクロール速度を設定する
 * @param scrollSpeed 1秒あたりに上方向へ移動する距離
 */
void CreditContentController::SetScrollSpeed(float scrollSpeed) {
    m_scrollSpeed = scrollSpeed;
}

/**
 * @brief クレジットを開始位置に戻す
 */
void CreditContentController::ResetCredit() {
    // GameObjectが未設定の場合は次の初期化まで状態のみを戻す
    if (GetGameObject() != nullptr) {
        gameObject().SetPosition(m_initialPosition);
    }

    m_scrollOffset = 0.0f;
    m_isCreditEnd = false;
}

/**
 * @brief クレジットの終了位置まで到達したかを取得
 * @return 終了位置に到達している場合はtrue
 */
bool CreditContentController::IsCreditEnd() const {
    return m_scrollOffset >= m_creditEndOffset;
}

/**
 * @brief クレジットを上方向へ1フレーム分移動
 */
void CreditContentController::MoveCredit() {
    // 経過時間に応じて所属GameObjectのY座標を上方向へ移動
    const float moveDistance = m_scrollSpeed * Time::deltaTime;
    transform().Translate(Vector3::Up * moveDistance);

    // 終了判定用に開始位置からの移動距離を蓄積
    m_scrollOffset += moveDistance;
}
