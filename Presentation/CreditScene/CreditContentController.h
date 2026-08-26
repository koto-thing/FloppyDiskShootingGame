#pragma once

#include "CreditSceneContent.h"
#include "../../Domain/Entities/Component.h"
#include "../../Engine/Math/Vector3.h"

/**
 * @brief クレジット表示用GameObjectのスクロールを制御するコンポーネント
 * @details GameObjectのTransformを下から上へ移動させ、保持したクレジット内容の表示位置を制御します
 */
class CreditContentController : public Component {
public:
    /**
     * @brief クレジット表示コントローラーを生成します
     */
    CreditContentController();

    /**
     * @brief クレジット表示コントローラーを破棄します
     */
    ~CreditContentController() override = default;

    /**
     * @brief GameObjectにアタッチされた後の初期位置を記録します
     * @param renderer 描画サービスの参照
     */
    void Initialize(D3D12RenderingService& renderer) override;

    /**
     * @brief クレジットのスクロール状態を更新します
     */
    void Tick() override;

    /**
     * @brief クレジットの表示内容を設定します
     * @param creditContent 表示するクレジット内容
     */
    void SetCreditContent(CreditSceneContent creditContent);

    /**
     * @brief 設定されているクレジットの表示内容を取得します
     * @return クレジットの表示内容
     */
    const CreditSceneContent& GetCreditContent() const;

    /**
     * @brief クレジットのスクロール速度を設定します
     * @param scrollSpeed 1秒あたりに上方向へ移動する距離
     */
    void SetScrollSpeed(float scrollSpeed);

    /**
     * @brief クレジットを開始位置に戻します
     */
    void ResetCredit();

    /**
     * @brief クレジットの終了位置まで到達したかを取得します
     * @return 終了位置に到達している場合はtrue
     */
    bool IsCreditEnd() const;

private:
    /**
     * @brief クレジットを上方向へ1フレーム分移動します
     */
    void MoveCredit();

    CreditSceneContent m_creditContent;
    Vector3 m_initialPosition;
    float m_scrollSpeed;
    float m_creditEndOffset;
    float m_scrollOffset;
    bool m_isCreditEnd;
};
