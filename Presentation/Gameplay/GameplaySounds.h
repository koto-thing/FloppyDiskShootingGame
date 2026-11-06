#pragma once

#include <algorithm>
#include "../../Infrastructure/ExternalServices/AudioService.h"

// 本編とギャラリーで同じ合成処理と音量を使う
namespace GameplaySounds {
/** @brief 自機ショット音を再生する @param audio 再生先、未初期化ならnullptr @return なし */
inline void PlayShotSound(AudioService* audio) {
    // 自機ショットの音量調整 (0.0f ~ 1.0f)
    constexpr float PlayerShotVolume = 0.2f;
    if (audio) audio->PlayMMLSE("t240 o6 l32 v7 c>c", PlayerShotVolume);
}

/** @brief 被弾音を再生する @param audio 再生先、未初期化ならnullptr @return なし */
inline void PlayHitSound(AudioService* audio) {
    if (audio) audio->PlayMMLSE("t180 o4 l32 v10 g e c");
}

/** @brief 敵のエネルギー弾発射音を再生する @param audio 再生先、未初期化ならnullptr @return なし */
inline void PlayEnemyShotSound(AudioService* audio) {
    if (!audio) return;

    // 敵通常弾・大型弾の発砲音
    static const std::vector<int16_t> pcm = [] {
        // パルス成分の生成
        Audio::SfxrParams pulse;
        pulse.waveType = Audio::SfxrWaveType::Square;
        pulse.squareDuty = 0.30f;
        pulse.attackTime = 0.0f;
        pulse.sustainTime = 0.025f;
        pulse.decayTime = 0.075f;
        pulse.startFrequency = 0.72f;
        pulse.minFrequency = 0.18f;
        pulse.slide = -0.65f;
        pulse.masterVolume = 0.60f;
        const std::vector<int16_t> pcmPulse = Audio::SfxrGenerator::GeneratePCM(pulse, 44100);

        // 鋸波成分の生成
        Audio::SfxrParams beam;
        beam.waveType = Audio::SfxrWaveType::Sawtooth;
        beam.attackTime = 0.002f;
        beam.sustainTime = 0.060f;
        beam.decayTime = 0.160f;
        beam.startFrequency = 0.58f;
        beam.minFrequency = 0.12f;
        beam.slide = -0.50f;
        beam.masterVolume = 0.72f;
        const std::vector<int16_t> pcmBeam = Audio::SfxrGenerator::GeneratePCM(beam, 44100);

        // 2つの波形を加算合成
        const size_t totalSamples = (std::max)(pcmPulse.size(), pcmBeam.size());
        std::vector<int16_t> mixed(totalSamples, 0);
        for (size_t i = 0; i < totalSamples; ++i) {
            int32_t sample = 0;
            if (i < pcmPulse.size()) sample += pcmPulse[i];
            if (i < pcmBeam.size()) sample += pcmBeam[i];
            mixed[i] = static_cast<int16_t>(std::clamp(sample, -32760, 32760));
        }
        return mixed;
    }();
    audio->PlaySE(pcm, 1.20f);
}

/** @brief ミサイル発射音を再生する @param audio 再生先、未初期化ならnullptr @return なし */
inline void PlayMissileLaunchSound(AudioService* audio) {
    if (!audio) return;

    // 低域を含まない高周波サイン波を緩く下降させて鋭い噴射音を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Sine;
    sound.attackTime = 0.015f;
    sound.sustainTime = 0.34f;
    sound.decayTime = 0.26f;
    sound.startFrequency = 2.20f;
    sound.minFrequency = 1.35f;
    sound.slide = -0.08f;
    sound.masterVolume = 0.46f;
    audio->PlaySE(sound);
}

/** @brief ボス機銃音を再生する @param audio 再生先、未初期化ならnullptr @return なし */
inline void PlayBossMachineGunSound(AudioService* audio) {
    if (!audio) return;

    // 通常敵の射撃音
    static const std::vector<int16_t> pcm = [] {
        // アタックパルスノイズ成分の生成
        Audio::SfxrParams crack;
        crack.waveType = Audio::SfxrWaveType::Noise;
        crack.attackTime = 0.0f;
        crack.sustainTime = 0.018f;
        crack.decayTime = 0.045f;
        crack.startFrequency = 0.75f;
        crack.minFrequency = 0.25f;
        crack.slide = -0.70f;
        crack.masterVolume = 0.65f;
        const std::vector<int16_t> pcmCrack = Audio::SfxrGenerator::GeneratePCM(crack, 44100);

        // 鋸波成分の生成
        Audio::SfxrParams core;
        core.waveType = Audio::SfxrWaveType::Sawtooth;
        core.attackTime = 0.0f;
        core.sustainTime = 0.045f;
        core.decayTime = 0.120f;
        core.startFrequency = 0.54f;
        core.minFrequency = 0.14f;
        core.slide = -0.58f;
        core.masterVolume = 0.75f;
        const std::vector<int16_t> pcmCore = Audio::SfxrGenerator::GeneratePCM(core, 44100);

        // 2つの波形を加算合成
        const size_t totalSamples = (std::max)(pcmCrack.size(), pcmCore.size());
        std::vector<int16_t> mixed(totalSamples, 0);
        for (size_t i = 0; i < totalSamples; ++i) {
            int32_t sample = 0;
            if (i < pcmCrack.size()) sample += pcmCrack[i];
            if (i < pcmCore.size()) sample += pcmCore[i];
            mixed[i] = static_cast<int16_t>(std::clamp(sample, -32760, 32760));
        }
        return mixed;
    }();
    audio->PlaySE(pcm, 1.25f);
}

}
