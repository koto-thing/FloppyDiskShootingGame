#pragma once

#include <memory>
#include <map>
#include <functional>
#include "../Interfaces/IScene.h"

class Renderer;

/**
 * @brief シーン管理を行うクラス
 * @tparam Key シーンを識別するためのキーの型
 * @tparam SharedData シーン間で共有されるデータの型
 */
template <typename Key, typename SharedData>
class SceneManager {
public:
    /**
     * @brief SceneManagerクラスのコンストラクタ
     */
    SceneManager() : m_sharedData {} {}

    /**
     * @brief SceneManagerクラスのデストラクタ
     */
    ~SceneManager() = default;
    
    /**
     * @brief 管理対象にシーンを追加する
     * @tparam SceneType 追加するシーンクラスの型
     * @param key シーンを識別するためのキー
     */
    template <typename SceneType>
    void AddScene(const Key& key) {
        m_factories[key] = []() { return std::make_unique<SceneType>(); };
    }

    template <typename SceneType>
    void addScene(const Key& key) { AddScene<SceneType>(key); }
    
    /**
     * @brief 初期シーンを設定し、シーンの実行を開始する
     * @param firstSceneKey 最初のシーンの識別キー
     */
    void Initialize(const Key& firstSceneKey) {
        ApplyScene(firstSceneKey);
    }

    void init(const Key& firstSceneKey) { Initialize(firstSceneKey); }
    
    /**
     * @brief 現在実行中のシーンの入力処理を呼び出す
     */
    void ProcessInput() {
        if (m_currentScene)
            m_currentScene->ProcessInput();
    }
    
    /**
     * @brief 現在実行中のシーンの固定更新処理を呼び出し、シーン遷移の要求がある場合は遷移処理を行う
     */
    void Tick() {
        if (!m_currentScene)
            return;
        
        m_currentScene->Tick();
        
        if (m_currentScene->m_nextSceneRequest.has_value()) {
            m_pendingSceneRequest = m_currentScene->m_nextSceneRequest;
            m_currentScene->m_nextSceneRequest.reset();
        }
    }

    /** @brief フレーム境界で保留中のシーン遷移を反映する */
    void CommitTransitions() {
        if (!m_pendingSceneRequest.has_value()) return;
        const Key nextKey = *m_pendingSceneRequest;
        m_pendingSceneRequest.reset();
        ApplyScene(nextKey);
    }
    
    /** @brief 現在実行中のシーンをRenderer経由で描画する */
    void Render(Renderer& renderer) {
        if (m_currentScene) m_currentScene->Render(renderer);
    }

    /** @brief 現在のシーンを終了する */
    void Shutdown() {
        if (m_currentScene) m_currentScene->Shutdown();
        m_currentScene.reset();
        m_pendingSceneRequest.reset();
    }
    
    /**
     * @brief シーン間で共有されるデータの参照を取得する
     * @return SharedData& 共有データへの参照
     */
    SharedData& getSharedData() { return m_sharedData; }
    
private:
    /**
     * @brief 指定されたシーンへ遷移する
     * @param nextKey 遷移先シーンの識別キー
     */
    void ApplyScene(const Key& nextKey) {
        auto it = m_factories.find(nextKey);
        if (it == m_factories.end())
            return;
        
        if (m_currentScene) m_currentScene->Shutdown();
        m_currentScene.reset();
        
        // 新しいシーンを生成して初期化
        m_currentScene = it->second();
        m_currentScene->m_sharedData = &m_sharedData;
        m_currentScene->Initialize();
    }
    
    SharedData m_sharedData;                                                              //　シーン間で共有されるデータ
    std::unique_ptr<IScene<Key, SharedData>> m_currentScene;                              // 現在のアクティブなシーン
    std::map<Key, std::function<std::unique_ptr<IScene<Key, SharedData>>()>> m_factories; // シーンファクトリのマップ
    std::optional<Key> m_pendingSceneRequest;
};
