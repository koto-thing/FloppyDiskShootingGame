#pragma once

#include <optional>

template <typename Key, typename SharedData>
class SceneManager;

/**
 * @brief シーンのインターフェースクラス
 * @tparam Key シーンを識別するためのキーの型
 * @tparam SharedData シーン間で共有されるデータの型
 */
template <typename Key, typename SharedData>
class IScene {
public:
    virtual ~IScene() = default;
    
    /**
     * @brief シーンの初期化処理を行う\n 
     * シーン遷移時に一度だけ呼ばれる
     */
    virtual void Initialize() = 0;

    /**
     * @brief ユーザー入力の処理を行う\n
     * ゲームループの毎フレーム呼ばれる
     */
    virtual void ProcessInput() = 0;

    /**
     * @brief シーンの状態更新を行う\n
     * ゲームループの毎フレーム呼ばれる
     */
    virtual void Tick() = 0;

    /**
     * @brief シーンの描画処理を行う\n
     * ゲームループの毎フレーム呼ばれる
     * @param renderer DirectX 12 レンダラーの参照
     */
    virtual void Render(class D3D12Renderer& renderer) = 0;
    
protected:
    /**
     * @brief 共有データへの参照を取得
     * @return SharedData& 共有データへの参照
     */
    SharedData& getData() { return *m_sharedData; }

    /**
     * @brief 共有データへの参照を取得
     * @return const SharedData& 共有データへの参照
     */
    const SharedData& getData() const { return *m_sharedData; }
    
    /**
     * @brief 次のシーンへの遷移を要求
     * @param nextKey 遷移先シーンの識別キー
     */
    void changeScene(const Key& nextKey) { m_nextSceneRequest = nextKey; }
    
private:
    friend class SceneManager<Key, SharedData>;
    
    SharedData* m_sharedData = nullptr;                   // 共有データへのポインタ
    std::optional<Key> m_nextSceneRequest = std::nullopt; // 次のシーンへの遷移要求
};
