#pragma once

#include <memory>
#include <map>
#include <functional>
#include "../Interfaces/IScene.h"
#include "../../Infrastructure/ExternalServices/D3D12Renderer.h"

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
    void addScene(const Key& key) {
        m_factories[key] = []() { return std::make_unique<SceneType>(); };
    }
    
    /**
     * @brief 初期シーンを設定し、シーンの実行を開始する
     * @param firstSceneKey 最初のシーンの識別キー
     */
    void init(const Key& firstSceneKey) {
        changeScene(firstSceneKey);
    }
    
    /**
     * @brief 現在実行中のシーンの入力処理を呼び出す
     */
    void ProcessInput() {
        if (m_currentScene)
            m_currentScene->ProcessInput();
    }
    
    /**
     * @brief 現在実行中のシーンの更新処理を呼び出し、シーン遷移の要求がある場合は遷移処理を行う
     */
    void Tick() {
        if (!m_currentScene)
            return;
        
        m_currentScene->Tick();
        
        // シーン遷移の要求があれば、次のシーンへ遷移
        if (m_currentScene->m_nextSceneRequest.has_value()) {
            Key nextKey = m_currentScene->m_nextSceneRequest.value();
            changeScene(nextKey);
        }
    }
    
    /**
     * @brief 現在実行中のシーンの描画処理を呼び出す
     * @param renderer DirectX 12 レンダラーの参照
     */
    void Render(D3D12Renderer& renderer) {
        if (m_currentScene)
            m_currentScene->Render(renderer);
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
    void changeScene(const Key& nextKey) {
        auto it = m_factories.find(nextKey);
        if (it == m_factories.end())
            return;
        
        // 現在のシーンを破棄
        m_currentScene.reset();
        
        // 新しいシーンを生成して初期化
        m_currentScene = it->second();
        m_currentScene->m_sharedData = &m_sharedData;
        m_currentScene->Initialize();
    }
    
    SharedData m_sharedData;                                                              //　シーン間で共有されるデータ
    std::unique_ptr<IScene<Key, SharedData>> m_currentScene;                              // 現在のアクティブなシーン
    std::map<Key, std::function<std::unique_ptr<IScene<Key, SharedData>>()>> m_factories; // シーンファクトリのマップ
};
