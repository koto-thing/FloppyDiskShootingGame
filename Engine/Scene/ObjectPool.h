#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @brief 固定容量のオブジェクトプール
 *
 * コンテナ領域は生成時に一度だけ確保し、SpawnとDespawnで要素を再利用する
 * activeメンバーを持つ型ではfalseになった要素も自動的に再利用可能と判定する
 */
template <typename T>
class ObjectPool {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    explicit ObjectPool(std::size_t capacity) : m_items(capacity), m_used(capacity, false) {}

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) noexcept = default;
    ObjectPool& operator=(ObjectPool&&) noexcept = default;

    /** @brief 未使用要素を取得する。上限到達時はnullptrを返す */
    T* Spawn() {
        for (std::size_t index = 0; index < m_items.size(); ++index) {
            if (m_used[index] && HasBecomeInactive(m_items[index])) m_used[index] = false;
            if (m_used[index]) continue;
            m_used[index] = true;
            m_items[index] = T{};
            return &m_items[index];
        }
        return nullptr;
    }

    /** @brief要素を使用済み状態から解放する */
    bool Despawn(T& object) {
        const std::size_t index = IndexOf(object);
        if (index >= m_items.size() || !m_used[index]) return false;
        m_used[index] = false;
        SetInactive(m_items[index]);
        return true;
    }

    /** @brief 全要素を初期状態へ戻す */
    void Reset() {
        for (std::size_t index = 0; index < m_items.size(); ++index) {
            m_items[index] = T{};
            m_used[index] = false;
        }
    }

    /** @brief 容量を取得する */
    std::size_t Capacity() const { return m_items.size(); }
    /** @brief 使用中要素数を取得する */
    std::size_t ActiveCount() const {
        std::size_t count = 0;
        for (std::size_t index = 0; index < m_items.size(); ++index) {
            if (m_used[index] && !HasBecomeInactive(m_items[index])) ++count;
        }
        return count;
    }

    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    T& operator[](std::size_t index) { return m_items[index]; }
    const T& operator[](std::size_t index) const { return m_items[index]; }

private:
    template <typename U>
    static constexpr bool HasActiveMember = requires(const U& value) { value.active; };

    static bool HasBecomeInactive(const T& object) {
        if constexpr (HasActiveMember<T>) {
            return !object.active;
        } else {
            return false;
        }
    }

    static void SetInactive(T& object) {
        if constexpr (HasActiveMember<T>) object.active = false;
    }

    std::size_t IndexOf(const T& object) const {
        const T* first = m_items.data();
        const T* pointer = &object;
        if (pointer < first || pointer >= first + m_items.size()) return m_items.size();
        return static_cast<std::size_t>(pointer - first);
    }

    std::vector<T> m_items;
    std::vector<bool> m_used;
};
