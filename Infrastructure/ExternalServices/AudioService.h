#pragma once

#include "SfxrGenerator.h"

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief オーディオ管理サービス（BGM/SE再生、ミキサー機能、SFXR合成機能）
 */
class AudioService {
public:
    AudioService();
    ~AudioService();

    /**
     * @brief グローバルインスタンスの取得
     * @return AudioServiceインスタンスへの参照
     */
    static AudioService& Get();

    /**
     * @brief 初期化処理
     * @return 成功した場合true
     */
    bool Initialize();

    /**
     * @brief 終了処理
     */
    void Shutdown();

    /**
     * @brief フレーム更新（終了したSEボイスのクリーンアップなど）
     */
    void Update();

    // --- ミキサー機能 (音量調整) ---

    /**
     * @brief マスター音量の設定
     * @param volume 音量 (0.0 ~ 1.0)
     */
    void SetMasterVolume(float volume);

    /**
     * @brief マスター音量の取得
     * @return 音量 (0.0 ~ 1.0)
     */
    float GetMasterVolume() const;

    /**
     * @brief BGM音量の設定
     * @param volume 音量 (0.0 ~ 1.0)
     */
    void SetBGMVolume(float volume);

    /**
     * @brief BGM音量の取得
     * @return 音量 (0.0 ~ 1.0)
     */
    float GetBGMVolume() const;

    /**
     * @brief SE音量の設定
     * @param volume 音量 (0.0 ~ 1.0)
     */
    void SetSEVolume(float volume);

    /**
     * @brief SE音量の取得
     * @return 音量 (0.0 ~ 1.0)
     */
    float GetSEVolume() const;

    // --- BGM再生 ---

    /**
     * @brief MMLによるBGM再生
     * @param mml MML文字列
     * @param loop ループ再生するかどうか
     */
    void PlayMMLBGM(const std::string& mml, bool loop = true);

    /**
     * @brief MMLによるSEワンショット再生
     * @param mml MML文字列
     * @param volume 個別音量 (0.0 ~ 2.0)
     */
    void PlayMMLSE(const std::string& mml, float volume = 1.0f);

    /**
     * @brief ファイルからのMML BGM再生
     * @param filePath MMLファイルパス
     * @param loop ループ再生するかどうか
     * @return 成功した場合true
     */
    bool PlayMMLBGMFromFile(const std::string& filePath, bool loop = true);

    /**
     * @brief BGM停止
     */
    void StopBGM();

    // --- SEワンショット再生 (同時発音対応) ---

    /**
     * @brief SFXR合成パラメータによるSEワンショット再生
     * @param params 合成パラメータ
     * @param volume 個別音量 (0.0 ~ 1.0)
     */
    void PlaySE(const Audio::SfxrParams& params, float volume = 1.0f);

    /**
     * @brief SFXRプリセットによるSEワンショット再生
     * @param preset プリセット種別
     * @param volume 個別音量 (0.0 ~ 1.0)
     */
    void PlaySE(Audio::SfxrPreset preset, float volume = 1.0f);

    /**
     * @brief Raw PCMバッファによるSEワンショット再生
     * @param pcmBuffer 16bit PCMデータ
     * @param volume 個別音量 (0.0 ~ 1.0)
     */
    void PlaySE(const std::vector<int16_t>& pcmBuffer, float volume = 1.0f);

    /**
     * @brief プリセットごとの基準音量を設定
     * @param preset プリセット種別
     * @param volume 音量 (0.0 ~ 2.0)
     */
    void SetPresetVolume(Audio::SfxrPreset preset, float volume);

    /**
     * @brief プリセットごとの基準音量を取得
     * @param preset プリセット種別
     * @return 音量 (0.0 ~ 2.0)
     */
    float GetPresetVolume(Audio::SfxrPreset preset) const;

    /**
     * @brief 同時発音リミッターの有効状態を設定
     * @param enabled 有効にする場合true
     */
    void SetLimiterEnabled(bool enabled);

    /**
     * @brief 同時発音リミッターの有効状態を取得
     * @return 有効な場合true
     */
    bool IsLimiterEnabled() const;

    /**
     * @brief すべてのSE発音を停止
     */
    void StopAllSE();

private:
    struct Impl;
    Impl* impl;

    static AudioService* s_instance;
};
