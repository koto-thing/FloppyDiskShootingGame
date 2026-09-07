#include "../Engine/Scene/ObjectPool.h"

#include <stdexcept>

namespace {
struct PooledValue {
    int value = 0;
    bool active = false;
};

/** @brief テスト条件を検証する */
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

/** @brief オブジェクトプールの再利用と上限を検証する */
void RunObjectPoolTests() {
    // 容量上限までの生成を検証する
    ObjectPool<PooledValue> pool(2);
    Require(pool.Capacity() == 2, "Pool capacity must be fixed");
    PooledValue* first = pool.Spawn();
    first->active = true;
    PooledValue* second = pool.Spawn();
    Require(first != nullptr && second != nullptr, "Pool must spawn within capacity");
    second->active = true;
    Require(pool.Spawn() == nullptr, "Pool must reject spawns at capacity");
    Require(pool.ActiveCount() == 2, "Pool active count must match spawned objects");

    // 無効化した要素の再利用を検証する
    first->active = false;
    PooledValue* reused = pool.Spawn();
    Require(reused == first, "Inactive object must be reused without allocation");
    reused->active = true;
    Require(pool.Despawn(*reused), "Despawn must release a spawned object");
    Require(!reused->active, "Despawn must clear active state");

    pool.Reset();
    Require(pool.ActiveCount() == 0, "Reset must release all objects");
}
