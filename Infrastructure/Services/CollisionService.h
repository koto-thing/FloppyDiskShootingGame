#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "../../Domain/Entities/Collider.h"
#include "../../Domain/Entities/CircleCollider.h"
#include "../../Domain/Entities/AABBCollider.h"

class CollisionService {
public:
    /**
     * @brief 衝突判定対象へコライダーを登録する
     * @param collider 登録するコライダー
     */
    void RegisterCollider(const std::shared_ptr<Collider>& collider);
    /**
     * @brief 指定したコライダーを登録対象から外す
     * @param collider 解除するコライダー
     */
    void UnregisterCollider(const Collider* collider);

    /** @brief 登録済みコライダーの衝突状態を更新する */
    void Tick();
    /** @brief 登録済みコライダーと衝突状態をすべて消去する */
    void Clear();

private:
    enum class CollisionEvent { Enter, Stay, Exit };

    struct CollisionPair {
        const Collider* first = nullptr;
        const Collider* second = nullptr;

        /** @brief 空の衝突ペアを生成する */
        CollisionPair() = default;
        /**
         * @brief 2つのコライダーから順序付きペアを生成する
         * @param lhs 左側のコライダー
         * @param rhs 右側のコライダー
         */
        CollisionPair(const Collider* lhs, const Collider* rhs);
        /**
         * @brief 衝突ペアの一致を判定する
         * @param other 比較対象の衝突ペア
         * @return 同一のコライダー組み合わせならtrue
         */
        bool operator==(const CollisionPair& other) const {
            return first == other.first && second == other.second;
        }
    };

    struct CollisionPairHash {
        /**
         * @brief 衝突ペアのハッシュ値を計算する
         * @param pair ハッシュ対象の衝突ペア
         * @return ハッシュ値
         */
        std::size_t operator()(const CollisionPair& pair) const;
    };

    /** @brief 互いのレイヤーマスクが衝突を許可しているか判定する */
    bool CanCollide(const Collider& a, const Collider& b) const;
    /** @brief コライダー種別に応じた交差判定を実行する */
    bool CheckCollision(const Collider& a, const Collider& b) const;
    /** @brief 円コライダー同士の交差を判定する */
    bool CheckCircleCircle(const CircleCollider& a, const CircleCollider& b) const;
    /** @brief 軸平行境界箱同士の交差を判定する */
    bool CheckAABBAABB(const AABBCollider& a, const AABBCollider& b) const;
    /** @brief 円コライダーと軸平行境界箱の交差を判定する */
    bool CheckCircleAABB(const CircleCollider& circle, const AABBCollider& aabb) const;
    /** @brief 登録中のコライダーから有効な共有所有権を取得する */
    std::shared_ptr<Collider> FindLiveCollider(const Collider* collider) const;
    /** @brief 衝突イベントを両方のGameObjectへ通知する */
    void Dispatch(const CollisionPair& pair, CollisionEvent event) const;
    
    std::vector<std::weak_ptr<Collider>> m_colliders;
    std::unordered_set<CollisionPair, CollisionPairHash> m_previousPairs;
    std::unordered_set<CollisionPair, CollisionPairHash> m_currentPairs;
};
