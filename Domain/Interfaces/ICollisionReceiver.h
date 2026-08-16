#pragma once

class Collider;

class ICollisionReceiver {
public:
    virtual ~ICollisionReceiver() = default;

    /**
     * @brief コライダーが衝突した際に呼び出されるコールバック関数
     * @param self 衝突したコライダー自身
     * @param other 衝突した相手のコライダー
     */
    virtual void OnCollisionEnter(
        Collider& self,
        Collider& other
    ) {
        
    }

    /**
     * @brief コライダーが衝突中に呼び出されるコールバック関数
     * @param self 衝突したコライダー自身
     * @param other 衝突した相手のコライダー
     */
    virtual void OnCollisionStay(
        Collider& self,
        Collider& other
    ) {
        
    }

    /**
     * @brief コライダーが衝突を終了した際に呼び出されるコールバック関数
     * @param self 衝突したコライダー自身
     * @param other 衝突した相手のコライダー
     */
    virtual void OnCollisionExit(
        Collider& self,
        Collider& other
    ) {
        
    }
};
