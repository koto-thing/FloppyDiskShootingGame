#pragma once

#include <vector>
#include <cstdint>

namespace Audio {

/**
 * @brief SFXR波形タイプ
 */
enum class SfxrWaveType {
    Square = 0,
    Sawtooth = 1,
    Sine = 2,
    Noise = 3
};

/**
 * @brief SFXRプリセット種別
 */
enum class SfxrPreset {
    LaserShoot,
    Explosion,
    HitHurt,
    BlipSelect
};

/**
 * @brief SFXR効果音合成パラメータ構造体
 */
struct SfxrParams {
    SfxrWaveType waveType = SfxrWaveType::Square;

    // ADSR エンベロープ
    float attackTime = 0.0f;
    float sustainTime = 0.3f;
    float decayTime = 0.4f;

    // 周波数スイープ
    float startFrequency = 0.3f;
    float minFrequency = 0.0f;
    float slide = 0.0f;

    // デューティサイクル (矩形波用)
    float squareDuty = 0.5f;

    // 全体音量
    float masterVolume = 0.5f;

    /**
     * @brief 指定プリセットのデフォルトパラメータを生成
     * @param preset プリセット種別
     * @return 合成用パラメータ
     */
    static SfxrParams CreatePreset(SfxrPreset preset);
};

/**
 * @brief SFXR合成エンジンクラス
 */
class SfxrGenerator {
public:
    /**
     * @brief SFXRパラメータから16-bit 44.1kHz モノラルPCM波形をリアルタイム合成する
     * @param params 合成パラメータ
     * @param sampleRate サンプルレート (デフォルト 44100Hz)
     * @return 合成されたPCMバッファ
     */
    static std::vector<int16_t> GeneratePCM(const SfxrParams& params, int sampleRate = 44100);
};

} // namespace Audio
