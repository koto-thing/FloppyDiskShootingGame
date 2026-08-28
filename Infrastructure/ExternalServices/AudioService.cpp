#include "AudioService.h"
#include "MMLParser.h"
#include "SynthWaveGenerator.h"

#include <windows.h>
#include <xaudio2.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "xaudio2.lib")

AudioService* AudioService::s_instance = nullptr;

// SEボイスインスタンス管理構造体
struct SEVoiceInstance {
    IXAudio2SourceVoice* voice = nullptr;
    std::vector<int16_t> pcmBuffer;
    bool inUse = false;
};

// AudioService内部実装構造体
struct AudioService::Impl {
    IXAudio2* xAudio2 = nullptr;
    IXAudio2MasteringVoice* masteringVoice = nullptr;

    // サブミックスボイス (BGM / SE 用)
    IXAudio2SubmixVoice* bgmSubmixVoice = nullptr;
    IXAudio2SubmixVoice* seSubmixVoice = nullptr;

    // BGM用ソースボイス
    IXAudio2SourceVoice* bgmVoice = nullptr;
    std::vector<int16_t> bgmPcmBuffer;

    // SE用ボイスプールとキャッシュ
    std::vector<SEVoiceInstance> seVoicePool;
    size_t nextSeVoiceIdx = 0;
    static constexpr size_t MAX_SE_VOICES = 32;
    std::map<Audio::SfxrPreset, std::vector<int16_t>> presetCache;

    // 音量設定
    float masterVolume = 1.0f;
    float bgmVolume = 1.0f;
    float seVolume = 1.0f;

    WAVEFORMATEX waveFormat = {};

    Impl() {
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 1;
        waveFormat.nSamplesPerSec = 44100;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    }
};

AudioService::AudioService() : impl(new Impl()) {
    if (!s_instance) {
        s_instance = this;
    }
}

AudioService::~AudioService() {
    Shutdown();
    if (s_instance == this) {
        s_instance = nullptr;
    }
    delete impl;
}

AudioService& AudioService::Get() {
    static AudioService dummy;
    if (s_instance) {
        return *s_instance;
    }
    return dummy;
}

bool AudioService::Initialize() {
    s_instance = this;

    // XAudio2オブジェクトの作成
    if (FAILED(XAudio2Create(&impl->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        return false;
    }

    // マスタリングボイスの作成
    if (FAILED(impl->xAudio2->CreateMasteringVoice(&impl->masteringVoice))) {
        return false;
    }

    // BGM用サブミックスボイスの作成
    if (FAILED(impl->xAudio2->CreateSubmixVoice(&impl->bgmSubmixVoice, 1, 44100))) {
        return false;
    }

    // SE用サブミックスボイスの作成
    if (FAILED(impl->xAudio2->CreateSubmixVoice(&impl->seSubmixVoice, 1, 44100))) {
        return false;
    }

    // 初期音量の設定
    impl->masteringVoice->SetVolume(impl->masterVolume);
    impl->bgmSubmixVoice->SetVolume(impl->bgmVolume);
    impl->seSubmixVoice->SetVolume(impl->seVolume);

    // SEソースボイスの事前生成 (ゲームプレイ中のボイス生成・メモリ割り当てオーバーヘッドを排除)
    impl->seVoicePool.resize(Impl::MAX_SE_VOICES);
    for (size_t i = 0; i < Impl::MAX_SE_VOICES; ++i) {
        XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, impl->seSubmixVoice };
        XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };
        impl->xAudio2->CreateSourceVoice(&impl->seVoicePool[i].voice, &impl->waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList);
    }

    // SEプリセットの事前キャッシュ生成 (ゲームプレイ中のリアルタイム波形生成CPU負荷と定期的なフリーズを解消)
    impl->presetCache[Audio::SfxrPreset::LaserShoot] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::LaserShoot), 44100);
    impl->presetCache[Audio::SfxrPreset::Explosion] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::Explosion), 44100);
    impl->presetCache[Audio::SfxrPreset::HitHurt] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::HitHurt), 44100);
    impl->presetCache[Audio::SfxrPreset::BlipSelect] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::BlipSelect), 44100);

    return true;
}

void AudioService::Shutdown() {
    StopBGM();
    StopAllSE();

    // SEボイスの破棄
    for (auto& instance : impl->seVoicePool) {
        if (instance.voice) {
            instance.voice->Stop(0, XAUDIO2_COMMIT_NOW);
            instance.voice->FlushSourceBuffers();
            instance.voice->DestroyVoice();
            instance.voice = nullptr;
        }
    }
    impl->seVoicePool.clear();
    impl->presetCache.clear();

    // サブミックスボイスの破棄
    if (impl->seSubmixVoice) {
        impl->seSubmixVoice->DestroyVoice();
        impl->seSubmixVoice = nullptr;
    }
    if (impl->bgmSubmixVoice) {
        impl->bgmSubmixVoice->DestroyVoice();
        impl->bgmSubmixVoice = nullptr;
    }

    // マスタリングボイスおよびXAudio2オブジェクトの開放
    if (impl->masteringVoice) {
        impl->masteringVoice->DestroyVoice();
        impl->masteringVoice = nullptr;
    }
    if (impl->xAudio2) {
        impl->xAudio2->StopEngine();
        impl->xAudio2->Release();
        impl->xAudio2 = nullptr;
    }

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void AudioService::Update() {
    if (!impl->xAudio2) return;

    // 演奏終了したSEボイスの回収
    for (auto& instance : impl->seVoicePool) {
        if (instance.inUse && instance.voice) {
            XAUDIO2_VOICE_STATE state;
            instance.voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued == 0) {
                instance.voice->Stop(0);
                instance.voice->FlushSourceBuffers();
                instance.inUse = false;
                instance.pcmBuffer.clear();
            }
        }
    }
}

// --- ミキサー機能 (音量調整) ---

void AudioService::SetMasterVolume(float volume) {
    impl->masterVolume = std::clamp(volume, 0.0f, 1.0f);
    if (impl->masteringVoice) {
        impl->masteringVoice->SetVolume(impl->masterVolume);
    }
}

float AudioService::GetMasterVolume() const {
    return impl->masterVolume;
}

void AudioService::SetBGMVolume(float volume) {
    impl->bgmVolume = std::clamp(volume, 0.0f, 1.0f);
    if (impl->bgmSubmixVoice) {
        impl->bgmSubmixVoice->SetVolume(impl->bgmVolume);
    }
}

float AudioService::GetBGMVolume() const {
    return impl->bgmVolume;
}

void AudioService::SetSEVolume(float volume) {
    impl->seVolume = std::clamp(volume, 0.0f, 1.0f);
    if (impl->seSubmixVoice) {
        impl->seSubmixVoice->SetVolume(impl->seVolume);
    }
}

float AudioService::GetSEVolume() const {
    return impl->seVolume;
}

// --- BGM再生 ---

void AudioService::StopBGM() {
    if (impl->bgmVoice) {
        impl->bgmVoice->Stop(0, XAUDIO2_COMMIT_NOW);
        impl->bgmVoice->FlushSourceBuffers();
        impl->bgmVoice->DestroyVoice();
        impl->bgmVoice = nullptr;
    }
    impl->bgmPcmBuffer.clear();
}

void AudioService::PlayMMLBGM(const std::string& mml, bool loop) {
    if (!impl->xAudio2 || !impl->bgmSubmixVoice) return;

    StopBGM();

    Audio::MMLParser parser;
    Audio::MMLSequence sequence = parser.Parse(mml);

    Audio::SynthWaveGenerator generator;
    impl->bgmPcmBuffer = generator.GeneratePCM(sequence, 44100);

    if (impl->bgmPcmBuffer.empty()) return;

    // BGMサブミックスボイスへのルーティング設定
    XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, impl->bgmSubmixVoice };
    XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };

    if (FAILED(impl->xAudio2->CreateSourceVoice(&impl->bgmVoice, &impl->waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList))) {
        return;
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(impl->bgmPcmBuffer.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(impl->bgmPcmBuffer.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    if (SUCCEEDED(impl->bgmVoice->SubmitSourceBuffer(&buffer))) {
        impl->bgmVoice->Start(0);
    }
}

bool AudioService::PlayMMLBGMFromFile(const std::string& filePath, bool loop) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        file.open("../" + filePath);
    }
    if (!file.is_open()) {
        file.open("mml/" + filePath);
    }
    if (!file.is_open()) {
        return false;
    }
    std::string mml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (mml.empty()) {
        return false;
    }

    PlayMMLBGM(mml, loop);
    return true;
}

// --- SEワンショット再生 ---

void AudioService::PlaySE(const Audio::SfxrParams& params) {
    std::vector<int16_t> pcm = Audio::SfxrGenerator::GeneratePCM(params, 44100);
    PlaySE(pcm);
}

void AudioService::PlaySE(Audio::SfxrPreset preset) {
    auto it = impl->presetCache.find(preset);
    if (it != impl->presetCache.end()) {
        PlaySE(it->second);
    } else {
        Audio::SfxrParams params = Audio::SfxrParams::CreatePreset(preset);
        std::vector<int16_t> pcm = Audio::SfxrGenerator::GeneratePCM(params, 44100);
        impl->presetCache[preset] = pcm;
        PlaySE(pcm);
    }
}

void AudioService::PlaySE(const std::vector<int16_t>& pcmBuffer) {
    if (!impl->xAudio2 || !impl->seSubmixVoice || pcmBuffer.empty()) return;

    // 空きボイスの検索
    SEVoiceInstance* targetInstance = nullptr;
    for (auto& instance : impl->seVoicePool) {
        if (!instance.inUse) {
            targetInstance = &instance;
            break;
        }
    }

    // 空きボイスがない場合はラウンドロビンで再利用
    if (!targetInstance && !impl->seVoicePool.empty()) {
        targetInstance = &impl->seVoicePool[impl->nextSeVoiceIdx];
        impl->nextSeVoiceIdx = (impl->nextSeVoiceIdx + 1) % impl->seVoicePool.size();
        if (targetInstance->voice) {
            targetInstance->voice->Stop(0);
            targetInstance->voice->FlushSourceBuffers();
        }
        targetInstance->inUse = false;
    }

    if (!targetInstance || !targetInstance->voice) return;

    targetInstance->pcmBuffer = pcmBuffer;
    targetInstance->inUse = true;

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(targetInstance->pcmBuffer.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(targetInstance->pcmBuffer.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (SUCCEEDED(targetInstance->voice->SubmitSourceBuffer(&buffer))) {
        targetInstance->voice->Start(0);
    }
}

void AudioService::StopAllSE() {
    for (auto& instance : impl->seVoicePool) {
        if (instance.inUse && instance.voice) {
            instance.voice->Stop(0, XAUDIO2_COMMIT_NOW);
            instance.voice->FlushSourceBuffers();
            instance.inUse = false;
            instance.pcmBuffer.clear();
        }
    }
}

void AudioService::PlayMMLSE(const std::string& mml) {
    // MMLを一度だけPCMへ合成してSEボイスプールから再生する
    Audio::MMLParser parser;
    const Audio::MMLSequence sequence = parser.Parse(mml);
    Audio::SynthWaveGenerator generator;
    PlaySE(generator.GeneratePCM(sequence, 44100));
}
