#include "AudioService.h"
#include "MMLParser.h"
#include "MMLData.h"
#include "SynthWaveGenerator.h"

#include <windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <xapofx.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <map>
#ifdef FLOPPY_AUDIO_TESTS
#include <cassert>
#include <cstdio>
#endif

#pragma comment(lib, "xaudio2.lib")

AudioService* AudioService::s_instance = nullptr;

// SEボイスインスタンス管理構造体
struct SEVoiceInstance {
    IXAudio2SourceVoice* voice = nullptr;
    std::vector<int16_t> pcmBuffer;
    bool inUse = false;
};

// BGMは切り替え中だけ旧曲と新曲のボイスを同時に保持する
struct BGMVoiceInstance {
    IXAudio2SourceVoice* voice = nullptr;
    float gain = 0.0f;
    bool stopping = false;

    /**
     * @brief 現在の音量から120ms相当の速度でフェードを進める
     * @param seconds 前回更新からの実時間
     * @return フェードアウトを完了した場合true
     */
    bool AdvanceFade(float seconds) {
        gain = std::clamp(gain + (stopping ? -1.0f : 1.0f) * (std::max)(0.0f, seconds) / 0.12f, 0.0f, 1.0f);
        return stopping && gain == 0.0f;
    }
};

/**
 * @brief 指定した用途の範囲内だけで空き枠または置換する枠を選ぶ
 * @param pool 再生ボイス一覧
 * @param begin 用途の先頭インデックス
 * @param end 用途の終端インデックス
 * @param cursor 満杯時に次に置換するインデックス
 * @return 選択したインデックス
 */
static size_t SelectPlaybackSlot(const std::vector<SEVoiceInstance>& pool, size_t begin, size_t end, size_t& cursor) {
    for (size_t i = begin; i < end; ++i) {
        if (!pool[i].inUse) return i;
    }
    const size_t selected = cursor;
    cursor = begin + (cursor - begin + 1) % (end - begin);
    return selected;
}

// AudioService内部実装構造体
struct AudioService::Impl {
    IXAudio2* xAudio2 = nullptr;
    IXAudio2MasteringVoice* masteringVoice = nullptr;

    // サブミックスボイス (BGM / SE 用)
    IXAudio2SubmixVoice* bgmSubmixVoice = nullptr;
    IXAudio2SubmixVoice* seSubmixVoice = nullptr;

    // BGM用ソースボイス
    std::vector<BGMVoiceInstance> bgmVoices;
    std::map<std::string, std::vector<int16_t>> bgmCache;
    std::chrono::steady_clock::time_point lastBgmUpdate = std::chrono::steady_clock::now();

    // SE用ボイスプールとキャッシュ
    std::vector<SEVoiceInstance> seVoicePool;
    size_t nextSeVoiceIdx = 0;
    static constexpr size_t MAX_SE_VOICES = 32;
    // ponytail: セリフは同時4音まで保護し、それ以上が必要になったら台詞キューへ拡張する
    static constexpr size_t MAX_DIALOGUE_VOICES = 4;
    size_t nextDialogueIdx = MAX_SE_VOICES;
    std::map<Audio::SfxrPreset, std::vector<int16_t>> presetCache;

    // 音量設定
    float masterVolume = 1.0f;
    float bgmVolume = 1.0f;
    float seVolume = 1.0f;

    // リミッターおよび個別音量設定
    bool limiterEnabled = true;
    std::map<Audio::SfxrPreset, float> presetVolumes;

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

    // BGM・SE・セリフを合算した最終出力に標準リミッターを適用する
    IUnknown* limiter = nullptr;
    FXMASTERINGLIMITER_PARAMETERS parameters = { FXMASTERINGLIMITER_DEFAULT_RELEASE, FXMASTERINGLIMITER_DEFAULT_LOUDNESS };
    if (FAILED(CreateFX(__uuidof(FXMasteringLimiter), &limiter, &parameters, sizeof(parameters)))) {
        Shutdown();
        return false;
    }
    XAUDIO2_VOICE_DETAILS details{};
    impl->masteringVoice->GetVoiceDetails(&details);
    XAUDIO2_EFFECT_DESCRIPTOR descriptor = { limiter, impl->limiterEnabled, details.InputChannels };
    XAUDIO2_EFFECT_CHAIN chain = { 1, &descriptor };
    const HRESULT limiterResult = impl->masteringVoice->SetEffectChain(&chain);
    limiter->Release();
    if (FAILED(limiterResult)) {
        Shutdown();
        return false;
    }

    // BGM用サブミックスボイスの作成
    if (FAILED(impl->xAudio2->CreateSubmixVoice(&impl->bgmSubmixVoice, 2, 44100))) {
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
    impl->seVoicePool.resize(Impl::MAX_SE_VOICES + Impl::MAX_DIALOGUE_VOICES);
    for (size_t i = 0; i < impl->seVoicePool.size(); ++i) {
        XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, impl->seSubmixVoice };
        XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };
        if (FAILED(impl->xAudio2->CreateSourceVoice(&impl->seVoicePool[i].voice, &impl->waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList))) {
            Shutdown();
            return false;
        }
    }

    // SEプリセットの事前キャッシュ生成 (ゲームプレイ中のリアルタイム波形生成CPU負荷と定期的なフリーズを解消)
    impl->presetCache[Audio::SfxrPreset::LaserShoot] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::LaserShoot), 44100);
    impl->presetCache[Audio::SfxrPreset::Explosion] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::Explosion), 44100);
    impl->presetCache[Audio::SfxrPreset::HitHurt] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::HitHurt), 44100);
    impl->presetCache[Audio::SfxrPreset::BlipSelect] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::BlipSelect), 44100);

    // ponytail: 全14曲を起動時に同期生成し約190MiB保持する、曲数が増えたら章単位の先読みに切り替える
    bool prepared = PreloadMMLBGM(std::string(MMLData::GetTitleBgm()));
    for (int stage = 1; stage <= 6; ++stage) {
        prepared &= PreloadMMLBGM(std::string(MMLData::GetStageBgm(stage)));
        prepared &= PreloadMMLBGM(std::string(MMLData::GetBossBgm(stage)));
    }
    prepared &= PreloadMMLBGM(std::string(MMLData::GetEndingBgm()));
    if (!prepared) {
        Shutdown();
        return false;
    }
    impl->lastBgmUpdate = std::chrono::steady_clock::now();

    return true;
}

void AudioService::Shutdown() {
    // 終了時だけ即座に破棄し、XAudio2が参照するPCMはボイス破棄後に解放する
    for (auto& instance : impl->bgmVoices) instance.voice->DestroyVoice();
    impl->bgmVoices.clear();
    impl->bgmCache.clear();
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
    impl->nextSeVoiceIdx = 0;
    impl->nextDialogueIdx = Impl::MAX_SE_VOICES;
    impl->presetCache.clear();
    impl->presetVolumes.clear();

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

    // ポーズ中も実時間でフェードを進め、無音になった旧曲のボイスを回収する
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - impl->lastBgmUpdate).count();
    impl->lastBgmUpdate = now;
    for (auto it = impl->bgmVoices.begin(); it != impl->bgmVoices.end();) {
        const bool fadedOut = it->AdvanceFade(elapsed);
        // 曲の自然終了では残響を切らず、停止要求のフェードが完了してから破棄する
        if (fadedOut) {
            it->voice->DestroyVoice();
            it = impl->bgmVoices.erase(it);
        } else {
            it->voice->SetVolume(it->gain);
            ++it;
        }
    }

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

void AudioService::SetPresetVolume(Audio::SfxrPreset preset, float volume) {
    // プリセット音量の設定
    impl->presetVolumes[preset] = std::clamp(volume, 0.0f, 2.0f);
}

float AudioService::GetPresetVolume(Audio::SfxrPreset preset) const {
    // プリセット音量の取得
    auto it = impl->presetVolumes.find(preset);
    if (it != impl->presetVolumes.end()) {
        return it->second;
    }
    return 1.0f;
}

void AudioService::SetLimiterEnabled(bool enabled) {
    // 初期化後は最終出力のエフェクトも切り替える
    if (impl->masteringVoice) {
        const HRESULT result = enabled ? impl->masteringVoice->EnableEffect(0) : impl->masteringVoice->DisableEffect(0);
        if (FAILED(result)) return;
    }
    // リミッター有効状態の設定
    impl->limiterEnabled = enabled;
}

bool AudioService::IsLimiterEnabled() const {
    // リミッター有効状態の取得
    return impl->limiterEnabled;
}

// --- BGM再生 ---

void AudioService::StopBGM() {
    // 現在の音量を維持したままフェード方向だけを切り替える
    Update();
    for (auto& instance : impl->bgmVoices) instance.stopping = true;
}

/**
 * @brief MMLを一度だけステレオPCMへ生成する
 * @param mml MML文字列
 * @return 有効なPCMをキャッシュできた場合true
 */
bool AudioService::PreloadMMLBGM(const std::string& mml) {
    if (mml.empty()) return false;
    if (impl->bgmCache.contains(mml)) return true;
    Audio::MMLParser parser;
    Audio::SynthWaveGenerator generator;
    auto pcm = generator.GeneratePCM(parser.Parse(mml), 44100, true, true);
    if (pcm.empty()) return false;
    impl->bgmCache.emplace(mml, std::move(pcm));
    return true;
}

void AudioService::PlayMMLBGM(const std::string& mml, bool loop) {
    if (!impl->xAudio2 || !impl->bgmSubmixVoice) return;

    // 新曲の準備に失敗しても現在のBGMを継続し、キャッシュのPCMを直接参照する
    if (!PreloadMMLBGM(mml)) return;
    const auto& pcm = impl->bgmCache.at(mml);
    IXAudio2SourceVoice* voice = nullptr;
    impl->bgmVoices.reserve(impl->bgmVoices.size() + 1);

    // BGMサブミックスボイスへのルーティング設定
    XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, impl->bgmSubmixVoice };
    XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };

    // BGMだけステレオにし、SEと音声のモノラル形式を維持する
    WAVEFORMATEX bgmFormat = impl->waveFormat;
    bgmFormat.nChannels = 2;
    bgmFormat.nBlockAlign = 4;
    bgmFormat.nAvgBytesPerSec = bgmFormat.nSamplesPerSec * bgmFormat.nBlockAlign;
    if (FAILED(impl->xAudio2->CreateSourceVoice(&voice, &bgmFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList))) {
        return;
    }

    // OS標準リバーブを薄く適用し、曲を停止したときは残響もボイスと一緒に破棄する
    IUnknown* reverb = nullptr;
    if (SUCCEEDED(XAudio2CreateReverb(&reverb))) {
        XAUDIO2_EFFECT_DESCRIPTOR effect = {reverb, TRUE, 2};
        XAUDIO2_EFFECT_CHAIN chain = {1, &effect};
        if (SUCCEEDED(voice->SetEffectChain(&chain))) {
            XAUDIO2FX_REVERB_I3DL2_PARAMETERS room = XAUDIO2FX_I3DL2_PRESET_SMALLROOM;
            room.WetDryMix = 8.0f;
            room.DecayTime = 0.45f;
            XAUDIO2FX_REVERB_PARAMETERS parameters{};
            ReverbConvertI3DL2ToNative(&room, &parameters);
            if (FAILED(voice->SetEffectParameters(0, &parameters, sizeof(parameters)))) {
                voice->DisableEffect(0);
                OutputDebugStringA("BGM reverb parameters failed; using dry playback\n");
            }
        } else {
            OutputDebugStringA("BGM reverb chain failed; using dry playback\n");
        }
        reverb->Release();
    } else {
        OutputDebugStringA("BGM reverb unavailable; using dry playback\n");
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(pcm.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(pcm.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    // 新曲は無音から開始し、旧曲のフェードアウトと同時に音量を上げる
    if (FAILED(voice->SetVolume(0.0f)) || FAILED(voice->SubmitSourceBuffer(&buffer)) || FAILED(voice->Start(0))) {
        voice->DestroyVoice();
        return;
    }
    StopBGM();
    impl->bgmVoices.push_back({voice, 0.0f, false});
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
        file.open("Sound/" + filePath);
    }
    if (!file.is_open()) {
        file.open("Sound/mml/" + filePath);
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

void AudioService::PlaySE(const Audio::SfxrParams& params, float volume) {
    // パラメータからPCM波形を生成して再生
    std::vector<int16_t> pcm = Audio::SfxrGenerator::GeneratePCM(params, 44100);
    PlaySE(pcm, volume);
}

void AudioService::PlaySE(Audio::SfxrPreset preset, float volume) {
    // プリセット基準音量の乗算
    float baseVolume = GetPresetVolume(preset);
    float finalRequestedVolume = volume * baseVolume;

    // キャッシュ確認と再生
    auto it = impl->presetCache.find(preset);
    if (it != impl->presetCache.end()) {
        PlaySE(it->second, finalRequestedVolume);
    } else {
        Audio::SfxrParams params = Audio::SfxrParams::CreatePreset(preset);
        std::vector<int16_t> pcm = Audio::SfxrGenerator::GeneratePCM(params, 44100);
        impl->presetCache[preset] = pcm;
        PlaySE(pcm, finalRequestedVolume);
    }
}

void AudioService::PlaySE(const std::vector<int16_t>& pcmBuffer, float volume) {
    PlayPCM(pcmBuffer, volume, false);
}

/**
 * @brief セリフ専用枠でPCMを再生する
 * @param pcmBuffer モノラル44100Hzの16bit PCMデータ
 * @param volume 個別音量
 * @return なし
 */
void AudioService::PlayVoice(const std::vector<int16_t>& pcmBuffer, float volume) {
    PlayPCM(pcmBuffer, volume, true);
}

/**
 * @brief 用途に対応する再生枠へPCMを登録する
 * @param pcmBuffer モノラル44100Hzの16bit PCMデータ
 * @param volume 個別音量
 * @param dialogue セリフ用の再生枠を使う場合true
 * @return なし
 */
void AudioService::PlayPCM(const std::vector<int16_t>& pcmBuffer, float volume, bool dialogue) {
    if (!impl->xAudio2 || !impl->seSubmixVoice || pcmBuffer.empty()) return;

    // 通常SEとセリフの範囲を分け、通常SEはセリフの枠を置換しない
    const size_t begin = dialogue ? Impl::MAX_SE_VOICES : 0;
    const size_t end = dialogue ? impl->seVoicePool.size() : Impl::MAX_SE_VOICES;
    if (begin >= end || end > impl->seVoicePool.size()) return;
    size_t& cursor = dialogue ? impl->nextDialogueIdx : impl->nextSeVoiceIdx;
    SEVoiceInstance* targetInstance = &impl->seVoicePool[SelectPlaybackSlot(impl->seVoicePool, begin, end, cursor)];

    // 空きボイスがない場合はラウンドロビンで再利用
    if (targetInstance->inUse) {
        if (targetInstance->voice) {
            targetInstance->voice->Stop(0);
            targetInstance->voice->FlushSourceBuffers();
        }
        targetInstance->inUse = false;
    }

    if (!targetInstance || !targetInstance->voice) return;

    // 個別音量を保持し、ピーク制御は最終出力で行う
    float finalVolume = std::clamp(volume, 0.0f, 1.0f);

    // ボイス音量の適用
    targetInstance->voice->SetVolume(finalVolume);

    // バッファの登録と再生開始
    targetInstance->pcmBuffer = pcmBuffer;

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(targetInstance->pcmBuffer.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(targetInstance->pcmBuffer.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (SUCCEEDED(targetInstance->voice->SubmitSourceBuffer(&buffer)) && SUCCEEDED(targetInstance->voice->Start(0))) {
        targetInstance->inUse = true;
    } else {
        // 登録・開始失敗時もPCMの参照を解除して再利用可能な状態へ戻す
        targetInstance->voice->Stop(0);
        targetInstance->voice->FlushSourceBuffers();
        targetInstance->pcmBuffer.clear();
    }
}

void AudioService::StopAllSE() {
    // 全SEボイスの再生停止とバッファ破棄
    for (auto& instance : impl->seVoicePool) {
        if (instance.inUse && instance.voice) {
            instance.voice->Stop(0, XAUDIO2_COMMIT_NOW);
            instance.voice->FlushSourceBuffers();
            instance.inUse = false;
            instance.pcmBuffer.clear();
        }
    }
}

void AudioService::PlayMMLSE(const std::string& mml, float volume) {
    // MMLを一度だけPCMへ合成してSEボイスプールから再生
    Audio::MMLParser parser;
    const Audio::MMLSequence sequence = parser.Parse(mml);
    Audio::SynthWaveGenerator generator;
    PlaySE(generator.GeneratePCM(sequence, 44100), volume);
}

#ifdef FLOPPY_AUDIO_TESTS
/**
 * @brief BGMの再利用・フェードとセリフ専用枠を検証する
 * @return なし
 */
void RunAudioServiceTests() {
    // 満杯でも用途の境界を越えて枠を奪わず、範囲内で循環する
    constexpr size_t effects = AudioService::Impl::MAX_SE_VOICES;
    constexpr size_t total = effects + AudioService::Impl::MAX_DIALOGUE_VOICES;
    std::vector<SEVoiceInstance> slots(total);
    for (auto& slot : slots) slot.inUse = true;
    size_t seCursor = 0;
    size_t voiceCursor = effects;
    for (size_t i = 0; i < total * 3; ++i) {
        assert(SelectPlaybackSlot(slots, 0, effects, seCursor) < effects);
        const auto voiceSlot = SelectPlaybackSlot(slots, effects, total, voiceCursor);
        assert(voiceSlot >= effects && voiceSlot < total);
    }
    slots[effects + 1].inUse = false;
    assert(SelectPlaybackSlot(slots, effects, total, voiceCursor) == effects + 1);
    assert(SelectPlaybackSlot(slots, 0, effects, seCursor) < effects);

    // 途中で停止要求を出しても現在の音量から連続して下がる
    BGMVoiceInstance fading;
    assert(!fading.AdvanceFade(0.03f));
    assert(std::abs(fading.gain - 0.25f) < 0.0001f);
    fading.stopping = true;
    assert(!fading.AdvanceFade(0.015f));
    assert(std::abs(fading.gain - 0.125f) < 0.0001f);
    assert(fading.AdvanceFade(0.12f));
    assert(fading.gain == 0);

    // 先読みは音声デバイスなしでも動作し、同一曲のPCMは再生成しない
    AudioService audio;
    audio.SetMasterVolume(0);
    assert(!audio.PreloadMMLBGM(""));
    assert(!audio.PreloadMMLBGM("MML@;"));
    const std::string melody = "MML@t120@0o4c4;";
    assert(audio.PreloadMMLBGM(melody));
    const auto* cached = audio.impl->bgmCache.at(melody).data();
    assert(audio.PreloadMMLBGM(melody));
    assert(audio.impl->bgmCache.size() == 1 && audio.impl->bgmCache.at(melody).data() == cached);
    audio.impl->bgmCache.clear();
    const auto started = std::chrono::steady_clock::now();
    assert(audio.PreloadMMLBGM(std::string(MMLData::GetTitleBgm())));
    for (int stage = 1; stage <= 6; ++stage) {
        assert(audio.PreloadMMLBGM(std::string(MMLData::GetStageBgm(stage))));
        assert(audio.PreloadMMLBGM(std::string(MMLData::GetBossBgm(stage))));
    }
    assert(audio.PreloadMMLBGM(std::string(MMLData::GetEndingBgm())));
    assert(audio.impl->bgmCache.size() == 14);
    size_t bytes = 0;
    for (const auto& [name, pcm] : audio.impl->bgmCache) bytes += pcm.size() * sizeof(int16_t);
    std::printf("BGM preload: %.3fs, %.1f MiB PCM, 14 scores\n",
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(), bytes / 1048576.0);

    // デバイスがある環境ではXAudio2も無音・処理停止状態で検証する
    if (audio.Initialize()) {
        audio.impl->xAudio2->StopEngine();
        BOOL limiterState = FALSE;
        audio.impl->masteringVoice->GetEffectState(0, &limiterState);
        assert(limiterState && audio.IsLimiterEnabled());
        audio.SetLimiterEnabled(false);
        audio.impl->masteringVoice->GetEffectState(0, &limiterState);
        assert(!limiterState && !audio.IsLimiterEnabled());
        audio.SetLimiterEnabled(true);
        audio.impl->masteringVoice->GetEffectState(0, &limiterState);
        assert(limiterState && audio.IsLimiterEnabled());
        const std::vector<int16_t> silence(44100);
        audio.PlayVoice(silence);
        const auto* protectedPcm = audio.impl->seVoicePool[effects].pcmBuffer.data();
        for (int i = 0; i < 100; ++i) audio.PlaySE(silence);
        assert(audio.impl->seVoicePool[effects].inUse);
        assert(audio.impl->seVoicePool[effects].pcmBuffer.data() == protectedPcm);
        for (size_t i = effects + 1; i < total; ++i) assert(!audio.impl->seVoicePool[i].inUse);

        // 無効な曲は現在の再生を止めず、正常な切り替えだけ旧曲をフェードアウトする
        audio.PlayMMLBGM(melody);
        assert(audio.impl->bgmVoices.size() == 1);
        audio.PlayMMLBGM("MML@;");
        assert(audio.impl->bgmVoices.size() == 1 && !audio.impl->bgmVoices.front().stopping);
        audio.impl->lastBgmUpdate -= std::chrono::milliseconds(150);
        audio.Update();
        assert(audio.impl->bgmVoices.front().gain == 1);
        audio.PlayMMLBGM(std::string(MMLData::GetTitleBgm()));
        assert(audio.impl->bgmVoices.size() == 2 && audio.impl->bgmVoices.front().stopping);
        audio.impl->lastBgmUpdate -= std::chrono::milliseconds(150);
        audio.Update();
        assert(audio.impl->bgmVoices.size() == 1 && audio.impl->bgmVoices.front().gain == 1);
        audio.StopBGM();
        audio.impl->lastBgmUpdate -= std::chrono::milliseconds(150);
        audio.Update();
        assert(audio.impl->bgmVoices.empty());
        std::puts("XAudio2 silent playback checks passed");
    } else {
        std::puts("XAudio2 device unavailable; hardware playback checks skipped");
    }
    audio.Shutdown();
    assert(audio.impl->bgmVoices.empty() && audio.impl->bgmCache.empty());
    assert(audio.impl->seVoicePool.empty());
}
#endif
