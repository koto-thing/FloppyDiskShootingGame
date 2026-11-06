#include "AudioService.h"
#include "MMLParser.h"
#include "MMLData.h"
#include "SynthWaveGenerator.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#ifdef FLOPPY_AUDIO_TESTS
#include <cassert>
#endif

AudioService* AudioService::s_instance = nullptr;

struct AudioService::Impl {
    struct Effect {
        std::vector<int16_t> pcm;
        std::size_t position = 0;
        float volume = 1.0f;
        bool active = false;
    };
    struct Music {
        const std::vector<int16_t>* pcm = nullptr;
        std::size_t position = 0;
        float gain = 0.0f;
        bool loop = true;
        bool stopping = false;
        std::array<float, 1901> delayLeft{};
        std::array<float, 2111> delayRight{};
        std::size_t delayPosition = 0;
        std::size_t remainingTail = 22050;
    };
    SDL_AudioDeviceID device = 0;
    std::array<Effect, 36> effects{};
    std::size_t nextEffect = 0;
    std::size_t nextDialogue = 32;
    // ponytail: 120ms以内の曲切替は4曲まで重ね、超過時は最小音量を再利用する、必要なら動的プールへ拡張する
    std::array<Music, 4> music{};
    Music* progressMusic = nullptr;
    Effect* progressEffect = nullptr;
    std::map<std::string, std::vector<int16_t>> bgmCache;
    std::map<Audio::SfxrPreset, std::vector<int16_t>> presetCache;
    std::map<Audio::SfxrPreset, float> presetVolumes;
    float masterVolume = 1.0f;
    float bgmVolume = 1.0f;
    float seVolume = 1.0f;
    bool limiterEnabled = true;
    float limiterGain = 1.0f;

    /** @brief 音声スレッドが参照するデータの変更を開始する @return なし */
    void Lock() const { if (device) SDL_LockAudioDevice(device); }
    /** @brief 音声スレッドが参照するデータの変更を完了する @return なし */
    void Unlock() const { if (device) SDL_UnlockAudioDevice(device); }

    /** @brief SEとセリフの用途を維持して空き枠を選ぶ @param dialogue セリフの場合true @return 再生枠 */
    std::size_t SelectSlot(bool dialogue) {
        const std::size_t begin = dialogue ? 32 : 0;
        const std::size_t end = dialogue ? effects.size() : 32;
        for (std::size_t i = begin; i < end; ++i) if (!effects[i].active) return i;
        auto& cursor = dialogue ? nextDialogue : nextEffect;
        const auto selected = cursor;
        cursor = begin + (cursor - begin + 1) % (end - begin);
        return selected;
    }

    /** @brief 44100HzステレオのBGMとSEを合成する @param output 出力先 @param frames サンプルフレーム数 @return なし */
    void Mix(float* output, std::size_t frames) {
        // コールバック内で割当・解放を行わず固定プールだけを進める
        for (std::size_t frame = 0; frame < frames; ++frame) {
            float left = 0.0f, right = 0.0f;
            for (auto& voice : music) {
                if (!voice.pcm) continue;
                voice.gain = std::clamp(voice.gain + (voice.stopping ? -1.0f : 1.0f) / (44100.0f * 0.12f), 0.0f, 1.0f);
                if (voice.stopping && voice.gain == 0.0f) { voice.pcm = nullptr; continue; }
                if (voice.position + 1 >= voice.pcm->size() && voice.loop) voice.position = 0;
                float l = 0.0f, r = 0.0f;
                if (voice.position + 1 < voice.pcm->size()) {
                    l = (*voice.pcm)[voice.position++] / 32768.0f;
                    r = (*voice.pcm)[voice.position++] / 32768.0f;
                } else if (voice.remainingTail == 0) {
                    voice.pcm = nullptr;
                    continue;
                } else {
                    // 自然終了では残響の減衰を待ってから枠を戻す
                    --voice.remainingTail;
                }
                // ponytail: OS固有の小部屋リバーブを短い2本の残響で近似、音色一致が必要なら専用DSPへ置換する
                auto& dl = voice.delayLeft[voice.delayPosition % voice.delayLeft.size()];
                auto& dr = voice.delayRight[voice.delayPosition % voice.delayRight.size()];
                const float wetLeft = dl, wetRight = dr;
                dl = l + wetRight * 0.35f;
                dr = r + wetLeft * 0.35f;
                ++voice.delayPosition;
                left += (l + wetLeft * 0.08f) * voice.gain * bgmVolume;
                right += (r + wetRight * 0.08f) * voice.gain * bgmVolume;
            }
            for (auto& voice : effects) {
                if (!voice.active) continue;
                const float value = voice.pcm[voice.position++] / 32768.0f * voice.volume * seVolume;
                left += value;
                right += value;
                if (voice.position == voice.pcm.size()) voice.active = false;
            }
            // 最終合算のピークへ即時追従し、約50msで元の音量へ戻す
            left *= masterVolume;
            right *= masterVolume;
            const float peak = std::max(std::abs(left), std::abs(right));
            const float target = peak > 0.98f ? 0.98f / peak : 1.0f;
            limiterGain = std::min(target, limiterGain + 1.0f / 2205.0f);
            const float gain = limiterEnabled ? limiterGain : 1.0f;
            output[frame * 2] = std::clamp(left * gain, -1.0f, 1.0f);
            output[frame * 2 + 1] = std::clamp(right * gain, -1.0f, 1.0f);
        }
    }

    /** @brief SDLへステレオfloat音声を渡す @param user 内部状態 @param stream 出力先 @param bytes 出力バイト数 @return なし */
    static void Callback(void* user, Uint8* stream, int bytes) {
        SDL_memset(stream, 0, bytes);
        static_cast<Impl*>(user)->Mix(reinterpret_cast<float*>(stream), static_cast<std::size_t>(bytes) / (sizeof(float) * 2));
    }
};

/** @brief 再生サービスを生成する @return なし */
AudioService::AudioService() : impl(new Impl()) { if (!s_instance) s_instance = this; }
/** @brief デバイスの停止後にPCMを解放する @return なし */
AudioService::~AudioService() { Shutdown(); if (s_instance == this) s_instance = nullptr; delete impl; }
/** @brief 共通サービスを取得する @return サービス */
AudioService& AudioService::Get() { static AudioService fallback; return s_instance ? *s_instance : fallback; }

/** @brief 出力デバイスと既存の合成音源を初期化する @return 成功時true */
bool AudioService::Initialize() {
    if (impl->device) return true;
    s_instance = this;
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
    SDL_AudioSpec wanted{};
    wanted.freq = 44100;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = Impl::Callback;
    wanted.userdata = impl;
    // 実機の周波数差はSDLの変換へ任せ、合成側の44100Hzを維持する
    impl->device = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
    if (!impl->device) return false;
    for (const auto preset : {Audio::SfxrPreset::LaserShoot, Audio::SfxrPreset::Explosion, Audio::SfxrPreset::HitHurt, Audio::SfxrPreset::BlipSelect}) {
        impl->presetCache[preset] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(preset), 44100);
    }
    // ponytail: 既存版同様に全14曲を同期生成して約190MiB保持、曲追加時は章別先読みへ変更する
    bool ready = PreloadMMLBGM(std::string(MMLData::GetTitleBgm()));
    for (int stage = 1; stage <= 6; ++stage) {
        ready &= PreloadMMLBGM(std::string(MMLData::GetStageBgm(stage)));
        ready &= PreloadMMLBGM(std::string(MMLData::GetBossBgm(stage)));
    }
    ready &= PreloadMMLBGM(std::string(MMLData::GetEndingBgm()));
    if (!ready) { Shutdown(); return false; }
    SDL_PauseAudioDevice(impl->device, 0);
    return true;
}

/** @brief 音声スレッド停止後に参照先を解放する @return なし */
void AudioService::Shutdown() {
    if (impl->device) SDL_CloseAudioDevice(impl->device);
    impl->device = 0;
    impl->progressMusic = nullptr;
    impl->progressEffect = nullptr;
    for (auto& voice : impl->music) voice.pcm = nullptr;
    impl->effects = {};
    impl->bgmCache.clear();
    impl->presetCache.clear();
    impl->nextEffect = 0;
    impl->nextDialogue = 32;
    impl->limiterGain = 1.0f;
}
/** @brief 再生位置とフェードは音声コールバックで更新する @return なし */
void AudioService::Update() {}

/** @brief マスター音量を設定する @param volume 0から1 @return なし */
void AudioService::SetMasterVolume(float volume) { impl->Lock(); impl->masterVolume = std::isfinite(volume) ? std::clamp(volume, 0.0f, 1.0f) : 0.0f; impl->Unlock(); }
/** @brief マスター音量を取得する @return 設定値 */
float AudioService::GetMasterVolume() const { return impl->masterVolume; }
/** @brief BGM音量を設定する @param volume 0から1 @return なし */
void AudioService::SetBGMVolume(float volume) { impl->Lock(); impl->bgmVolume = std::isfinite(volume) ? std::clamp(volume, 0.0f, 1.0f) : 0.0f; impl->Unlock(); }
/** @brief BGM音量を取得する @return 設定値 */
float AudioService::GetBGMVolume() const { return impl->bgmVolume; }
/** @brief SE音量を設定する @param volume 0から1 @return なし */
void AudioService::SetSEVolume(float volume) { impl->Lock(); impl->seVolume = std::isfinite(volume) ? std::clamp(volume, 0.0f, 1.0f) : 0.0f; impl->Unlock(); }
/** @brief SE音量を取得する @return 設定値 */
float AudioService::GetSEVolume() const { return impl->seVolume; }
/** @brief 最終出力リミッターを切り替える @param enabled 有効時true @return なし */
void AudioService::SetLimiterEnabled(bool enabled) { impl->Lock(); impl->limiterEnabled = enabled; impl->Unlock(); }
/** @brief リミッター設定を取得する @return 有効時true */
bool AudioService::IsLimiterEnabled() const { return impl->limiterEnabled; }
/** @brief プリセット基準音量を設定する @param preset 音源 @param volume 0から2 @return なし */
void AudioService::SetPresetVolume(Audio::SfxrPreset preset, float volume) { impl->presetVolumes[preset] = std::isfinite(volume) ? std::clamp(volume, 0.0f, 2.0f) : 0.0f; }
/** @brief プリセット基準音量を取得する @param preset 音源 @return 設定値 */
float AudioService::GetPresetVolume(Audio::SfxrPreset preset) const { const auto found = impl->presetVolumes.find(preset); return found == impl->presetVolumes.end() ? 1.0f : found->second; }

/** @brief MMLを既存合成器で一度だけ生成する @param mml 原譜 @return 有効なPCM生成時true */
bool AudioService::PreloadMMLBGM(const std::string& mml) {
    if (mml.empty()) return false;
    if (impl->bgmCache.contains(mml)) return true;
    Audio::MMLParser parser;
    Audio::SynthWaveGenerator generator;
    auto pcm = generator.GeneratePCM(parser.Parse(mml), 44100, true, true);
    if (pcm.size() < 2 || pcm.size() % 2 != 0) return false;
    impl->bgmCache.emplace(mml, std::move(pcm));
    return true;
}

/** @brief 新曲へ120msでクロスフェードする @param mml 原譜 @param loop ループする場合true @return なし */
void AudioService::PlayMMLBGM(const std::string& mml, bool loop) {
    if (!impl->device || !PreloadMMLBGM(mml)) return;
    Impl::Music prepared{};
    prepared.pcm = &impl->bgmCache.at(mml);
    prepared.loop = loop;
    impl->Lock();
    auto* target = &impl->music[0];
    for (auto& voice : impl->music) {
        voice.stopping = true;
        if (!voice.pcm || (target->pcm && voice.gain < target->gain)) target = &voice;
    }
    *target = std::move(prepared);
    impl->progressMusic = target;
    impl->progressEffect = nullptr;
    impl->Unlock();
}

/** @brief 原譜ファイルを読み込んで再生する @param filePath パス @param loop ループ時true @return 読込成功時true */
bool AudioService::PlayMMLBGMFromFile(const std::string& filePath, bool loop) {
    for (const char* prefix : {"", "../", "mml/", "Sound/", "Sound/mml/"}) {
        std::ifstream file(std::string(prefix) + filePath);
        if (!file) continue;
        const std::string mml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (mml.empty() || !impl->device || !PreloadMMLBGM(mml)) return false;
        PlayMMLBGM(mml, loop);
        return true;
    }
    return false;
}
/** @brief 現在のBGMを短くフェードアウトする @return なし */
void AudioService::StopBGM() { impl->Lock(); impl->progressMusic = nullptr; for (auto& voice : impl->music) voice.stopping = true; impl->Unlock(); }
/** @brief 合成パラメーターをSEとして再生する @param params 音色 @param volume 個別音量 @return なし */
void AudioService::PlaySE(const Audio::SfxrParams& params, float volume) { PlaySE(Audio::SfxrGenerator::GeneratePCM(params, 44100), volume); }
/** @brief プリセットをキャッシュして再生する @param preset 音色 @param volume 個別音量 @return なし */
void AudioService::PlaySE(Audio::SfxrPreset preset, float volume) {
    if (!impl->presetCache.contains(preset)) impl->presetCache[preset] = Audio::SfxrGenerator::GeneratePCM(Audio::SfxrParams::CreatePreset(preset), 44100);
    PlaySE(impl->presetCache.at(preset), volume * GetPresetVolume(preset));
}
/** @brief MMLの効果音を再生する @param mml 原譜 @param volume 個別音量 @return なし */
void AudioService::PlayMMLSE(const std::string& mml, float volume) { Audio::MMLParser parser; Audio::SynthWaveGenerator generator; PlaySE(generator.GeneratePCM(parser.Parse(mml), 44100), volume); }
/** @brief PCM効果音を再生する @param pcmBuffer モノラル44100Hz音源 @param volume 個別音量 @return なし */
void AudioService::PlaySE(const std::vector<int16_t>& pcmBuffer, float volume) { PlayPCM(pcmBuffer, volume, false); }
/** @brief 専用4枠でセリフを再生する @param pcmBuffer モノラル44100Hz音源 @param volume 個別音量 @return なし */
void AudioService::PlayVoice(const std::vector<int16_t>& pcmBuffer, float volume) { PlayPCM(pcmBuffer, volume, true); }

/** @brief 音声スレッドと競合せず用途別枠へ登録する @param pcmBuffer 音源 @param volume 個別音量 @param dialogue セリフ時true @return なし */
void AudioService::PlayPCM(const std::vector<int16_t>& pcmBuffer, float volume, bool dialogue) {
    if (!impl->device || pcmBuffer.empty()) return;
    // コピーと古いバッファの解放は音声デバイスのロック外で行う
    auto prepared = pcmBuffer;
    impl->Lock();
    auto& voice = impl->effects[impl->SelectSlot(dialogue)];
    voice.pcm.swap(prepared);
    voice.position = 0;
    voice.volume = std::isfinite(volume) ? std::clamp(volume, 0.0f, 1.0f) : 0.0f;
    voice.active = true;
    impl->progressEffect = &voice;
    impl->progressMusic = nullptr;
    impl->Unlock();
}
/** @brief 全SEとセリフを停止する @return なし */
void AudioService::StopAllSE() { impl->Lock(); impl->progressEffect = nullptr; for (auto& voice : impl->effects) voice.active = false; impl->Unlock(); }

/** @brief 最後に再生を開始した音声の進捗を取得する @return 0から1、停止時は0 */
float AudioService::GetPlaybackProgress() const {
    // 音声コールバックと同じロックで位置とバッファ長を一緒に読む
    impl->Lock();
    float progress = 0.0f;
    if (const auto* music = impl->progressMusic) {
        progress = music->pcm ? static_cast<float>(music->position) / music->pcm->size() : 1.0f;
    } else if (const auto* effect = impl->progressEffect; effect && !effect->pcm.empty()) {
        progress = static_cast<float>(effect->position) / effect->pcm.size();
    }
    impl->Unlock();
    return std::clamp(progress, 0.0f, 1.0f);
}

#ifdef FLOPPY_AUDIO_TESTS
/** @brief オフライン合成でループ・終了・フェード・枠分離・ピークを検証する @return なし */
void RunAudioServiceTests() {
    AudioService service;
    auto& mixer = *service.impl;
    const std::vector<int16_t> stereo{8192, -8192, 16384, -16384};
    auto& song = mixer.music[0];
    song.pcm = &stereo;
    song.gain = 1.0f;
    std::array<float, 16> output{};
    // コールバックの実サンプル位置、自然終了、停止を確認する
    assert(service.GetPlaybackProgress() == 0.0f);
    mixer.progressMusic = &song;
    mixer.Mix(output.data(), 1);
    assert(service.GetPlaybackProgress() == 0.5f);
    mixer.Mix(output.data(), 2);
    assert(service.GetPlaybackProgress() == 0.5f);
    mixer.Mix(output.data(), 8);
    assert(output[0] > 0.0f && output[1] < 0.0f && song.pcm != nullptr);
    song.loop = false;
    mixer.Mix(output.data(), 8);
    assert(song.pcm != nullptr);
    for (int i = 0; i < 2800; ++i) mixer.Mix(output.data(), 8);
    assert(song.pcm == nullptr);
    assert(service.GetPlaybackProgress() == 1.0f);
    service.StopBGM();
    assert(service.GetPlaybackProgress() == 0.0f);
    song.pcm = &stereo;
    song.loop = true;
    song.stopping = true;
    for (int i = 0; i < 700; ++i) mixer.Mix(output.data(), 8);
    assert(song.pcm == nullptr);
    // 飽和してもSEはセリフの専用枠へ侵入しない
    for (auto& effect : mixer.effects) { effect.pcm.assign(8, 32767); effect.active = true; }
    for (int i = 0; i < 100; ++i) { assert(mixer.SelectSlot(false) < 32); assert(mixer.SelectSlot(true) >= 32); }
    mixer.Mix(output.data(), 8);
    for (float value : output) assert(std::isfinite(value) && std::abs(value) <= 0.981f);
    for (const auto& effect : mixer.effects) assert(!effect.active);
    mixer.progressEffect = &mixer.effects[32];
    assert(service.GetPlaybackProgress() == 1.0f);
    service.StopAllSE();
    assert(service.GetPlaybackProgress() == 0.0f);
    service.SetMasterVolume(0.0f);
    song = {};
    song.pcm = &stereo;
    mixer.Mix(output.data(), 8);
    for (float value : output) assert(value == 0.0f);
    assert(!service.PreloadMMLBGM(""));
    assert(service.PreloadMMLBGM("t120o4l8cde"));
    const auto* cached = mixer.bgmCache.begin()->second.data();
    assert(service.PreloadMMLBGM("t120o4l8cde"));
    assert(cached == mixer.bgmCache.begin()->second.data());
}
#endif
