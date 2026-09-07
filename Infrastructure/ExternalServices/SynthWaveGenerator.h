#pragma once

#include "MMLParser.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Audio {

/** @brief カスタム波形のサンプルを提供するインターフェース */
class IWaveformProvider {
public:
    /**
     * @brief 波形プロバイダーを破棄する
     */
    virtual ~IWaveformProvider() = default;

    /**
     * @brief 波形サンプルを生成する
     * @param customWaveId カスタム波形ID
     * @param phase 発音位相
     * @param frequency 周波数
     * @param sampleRate サンプルレート
     * @param timeInNoteSec ノート内の経過時間
     * @param isNoteStart ノート開始時の場合true
     * @return 波形サンプル値
     */
    virtual float Sample(uint8_t customWaveId, float phase, float frequency, int sampleRate, float timeInNoteSec, bool isNoteStart) = 0;
};

/** @brief MMLシーケンスからPCM波形を生成するクラス */
class SynthWaveGenerator {
public:
    /**
     * @brief PCM波形ジェネレーターを生成する
     */
    SynthWaveGenerator();

    /**
     * @brief PCM波形ジェネレーターを破棄する
     */
    ~SynthWaveGenerator() = default;

    /**
     * @brief カスタム波形プロバイダーを設定する
     * @param provider 設定する波形プロバイダー
     */
    void SetWaveformProvider(std::shared_ptr<IWaveformProvider> provider) {
        customProvider_ = provider;
    }

    /**
     * @brief MMLシーケンスからPCMデータを生成する
     * @param sequence 生成元のMMLシーケンス
     * @param sampleRate サンプルレート
     * @return 生成した16bit PCMデータ
     */
    std::vector<int16_t> GeneratePCM(const MMLSequence& sequence, int sampleRate = 44100);

private:
    /**
     * @brief オシレーターのサンプル値を生成する
     * @param type 波形種別
     * @param customId カスタム波形ID
     * @param phase 発音位相
     * @param frequency 周波数
     * @param sampleRate サンプルレート
     * @param lfsrState ノイズ生成用状態
     * @param timeInNoteSec ノート内の経過時間
     * @param trackIndex トラック番号
     * @return オシレーターのサンプル値
     */
    float GenerateOscillatorSample(
        WaveformType type, uint8_t customId, float phase, float frequency, int sampleRate, uint32_t lfsrState, float timeInNoteSec, int trackIndex
    );

    std::shared_ptr<IWaveformProvider> customProvider_ = nullptr;
    float fadeTimeSec_ = 0.003f;
};

/** @brief ゲーム内SFC音源のPCMサンプルを提供するクラス */
class SFCPCMSampleProvider : public IWaveformProvider {
public:
    /**
     * @brief SFC PCMサンプルプロバイダーを生成する
     */
    SFCPCMSampleProvider();

    /**
     * @brief SFC PCMサンプルプロバイダーを破棄する
     */
    virtual ~SFCPCMSampleProvider() = default;

    /**
     * @brief SFC PCM波形のサンプル値を生成する
     * @param customWaveId カスタム波形ID
     * @param phase 発音位相
     * @param frequency 周波数
     * @param sampleRate サンプルレート
     * @param timeInNoteSec ノート内の経過時間
     * @param isNoteStart ノート開始時の場合true
     * @return 波形サンプル値
     */
    virtual float Sample(uint8_t customWaveId, float phase, float frequency, int sampleRate, float timeInNoteSec, bool isNoteStart) override;
};

} // namespace Audio
