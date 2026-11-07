#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <compressapi.h>
#pragma comment(lib, "cabinet.lib")
#include "SynthWaveGenerator.h"
#include "EmbeddedSamples.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <numbers>
#include <random>
#include <string>
#include <vector>
#ifdef FLOPPY_AUDIO_TESTS
#include <cassert>
#endif

namespace Audio {

constexpr float PI_F = 3.14159265358979323846f;

// --------------------------------------------------------------------------
// GM 音色テーブル & フォールバック
// --------------------------------------------------------------------------
static const char* GM_INSTRUMENT_NAMES[128] = {
    "AcousticGrandPiano", "BrightAcousticPiano", "ElectricGrandPiano", "HonkyTonkPiano",
    "ElectricPiano1", "ElectricPiano2", "Harpsichord", "Clavi",
    "Celesta", "Glockenspiel", "MusicBox", "Vibraphone",
    "Marimba", "Xylophone", "TubularBells", "Dulcimer",
    "DrawbarOrgan", "PercussiveOrgan", "RockOrgan", "ChurchOrgan",
    "ReedOrgan", "Accordion", "Harmonica", "TangoAccordion",
    "NylonAcousticGuitar", "SteelAcousticGuitar", "JazzElectricGuitar", "CleanElectricGuitar",
    "MutedElectricGuitar", "OverdrivenGuitar", "DistortionGuitar", "GuitarHarmonics",
    "AcousticBass", "FingerElectricBass", "PickElectricBass", "FretlessBass",
    "SlapBass1", "SlapBass2", "SynthBass1", "SynthBass2",
    "Violin", "Viola", "Cello", "Contrabass",
    "TremoloStrings", "PizzicatoStrings", "OrchestralHarp", "Timpani",
    "StringEnsemble1", "StringEnsemble2", "SynthStrings1", "SynthStrings2",
    "ChoirAahs", "VoiceOohs", "SynthVoice", "OrchestraHit",
    "Trumpet", "Trombone", "Tuba", "MutedTrumpet",
    "FrenchHorn", "BrassSection", "SynthBrass1", "SynthBrass2",
    "SopranoSax", "AltoSax", "TenorSax", "BaritoneSax",
    "Oboe", "EnglishHorn", "Bassoon", "Clarinet",
    "Piccolo", "Flute", "Recorder", "PanFlute",
    "BlownBottle", "Shakuhachi", "Whistle", "Ocarina",
    "LeadSquare", "LeadSawtooth", "LeadCalliope", "LeadChiff",
    "LeadCharang", "LeadVoice", "LeadFifths", "LeadBassLead",
    "PadNewAge", "PadWarm", "PadPolysynth", "PadChoir",
    "PadBowed", "PadMetallic", "PadHalo", "PadSweep",
    "FXRain", "FXSoundtrack", "FXCrystal", "FXAtmosphere",
    "FXBrightness", "FXGoblins", "FXEchoes", "FXSciFi",
    "Sitar", "Banjo", "Shamisen", "Koto",
    "Kalimba", "Bagpipe", "Fiddle", "Shanai",
    "TinkleBell", "Agogo", "SteelDrums", "Woodblock",
    "TaikoDrum", "MelodicTom", "SynthDrum", "ReverseCymbal",
    "GuitarFretNoise", "BreathNoise", "Seashore", "BirdTweet",
    "TelephoneRing", "Helicopter", "Applause", "Gunshot"
};

/**
 * @brief 未収録GM音色を近似音色のプログラム番号へ解決する
 * @param prog GM音色プログラム番号 (0-127)
 * @return 解決後のプログラム番号
 */
static int ResolveWavProgram(int prog) {
    // 類似する収録済み音色へのフォールバック対応表
    static const std::map<int, int> FALLBACK = {
        {1, 0}, {2, 0}, {3, 0}, {5, 4}, {17, 16}, {20, 19}, {22, 21}, {23, 21},
        {25, 24}, {26, 28}, {27, 28}, {31, 24}, {37, 36}, {39, 38}, {41, 40},
        {42, 40}, {43, 32}, {46, 15}, {49, 48}, {51, 50}, {63, 62}, {66, 65},
        {67, 65}, {69, 68}, {71, 68}, {72, 73}, {74, 73}, {77, 75}, {85, 54},
        {88, 94}, {89, 90}, {91, 52}, {92, 48}, {93, 90}, {95, 90}, {96, 94},
        {97, 94}, {98, 8}, {99, 94}, {100, 90}, {101, 83}, {102, 90}, {103, 80},
        {104, 107}, {106, 107}, {108, 12}, {110, 40}, {111, 68}, {112, 10}, {114, 11},
        {121, 73}, {122, 94}, {123, 78}, {124, 10}, {125, 90}, {126, 55}, {127, 55}
    };
    if (prog < 0 || prog >= 128) prog = 0;
    auto it = FALLBACK.find(prog);
    return (it != FALLBACK.end()) ? it->second : prog;
}

/**
 * @brief GM音色の基準MIDIノート番号を取得する
 * @param name 音色名
 * @return 基準MIDIノート番号
 */
static int GetGmRootMidi(const std::string& name) {
    // 各音色の原音キー設定
    static const std::map<std::string, int> ROOT_MAP = {
        {"AcousticBass", 36}, {"FingerElectricBass", 36}, {"PickElectricBass", 36}, {"FretlessBass", 36},
        {"SlapBass1", 36}, {"SlapBass2", 36}, {"SynthBass1", 36}, {"SynthBass2", 36},
        {"Contrabass", 36}, {"Tuba", 36}, {"PizzicatoStrings", 36}, {"TubularBells", 36}, {"Timpani", 36},
        {"Cello", 48}, {"Bassoon", 48}, {"TenorSax", 48}, {"BaritoneSax", 48}, {"Sitar", 48}, {"Shamisen", 48}, {"Shanai", 48},
        {"Viola", 55}, {"Trombone", 55}, {"FrenchHorn", 55}, {"AltoSax", 55}, {"EnglishHorn", 55},
        {"AcousticGrandPiano", 45}, {"BrightAcousticPiano", 45}, {"HonkyTonkPiano", 45},
        {"Celesta", 72}, {"Glockenspiel", 72}, {"MusicBox", 72}, {"Piccolo", 72}, {"Whistle", 72}, {"Kalimba", 72}, {"TinkleBell", 72}
    };
    auto it = ROOT_MAP.find(name);
    return (it != ROOT_MAP.end()) ? it->second : 60;
}

/**
 * @brief MIDIノート番号から対応するドラム音色名を解決する
 * @param midiNote MIDIノート番号
 * @return ドラム音色名
 */
static const char* ResolveDrumName(int midiNote) {
    // 各ノート番号を代表ドラム音色名へ分岐
    switch (midiNote) {
        case 35: case 36: return "Kick";
        case 38: case 39: case 40: return "Snare";
        case 41: case 43: return "TomLow";
        case 45: case 47: return "TomMid";
        case 48: case 50: return "TomHigh";
        case 42: case 44: return "HiHatClosed";
        case 46: return "HiHatOpen";
        case 49: case 52: case 55: case 57: return "Crash";
        case 51: case 53: case 59: return "Ride";
        case 54: case 58: return "Tambourine";
        case 37: case 75: return "Woodblock";
        case 56: case 67: case 68: case 71: case 72: return "Agogo";
        case 60: case 62: case 63: case 65: return "TomHigh";
        case 61: case 64: case 66: return "TomMid";
        case 69: case 70: case 73: case 74: return "HiHatClosed";
        case 79: return "SynthDrum";
        case 76: case 77: return "Woodblock";
        case 78: return "SynthDrum";
        case 81: return "TaikoDrum";
        case 82: return "MelodicTom";
        case 83: return "ReverseCymbal";
        default:
            if (midiNote <= 36) return "Kick";
            if (midiNote <= 40) return "Snare";
            if (midiNote <= 44) return "HiHatClosed";
            if (midiNote <= 47) return "TomLow";
            if (midiNote <= 48) return "TomHigh";
            if (midiNote <= 53) return "Ride";
            if (midiNote <= 59) return "Crash";
            if (midiNote >= 80) return "TaikoDrum";
            return "Woodblock";
    }
}

/**
 * @brief 音色名からワンショット再生すべき音色か判定する
 * @param name 音色名
 * @return ワンショット再生する場合true
 */
static bool IsOneShotSample(const std::string& name) {
    // 打楽器・効果音系の名前一覧
    static const std::vector<std::string> ONESHOTS = {
        "kick", "snare", "hihat", "crash", "ride", "clap", "cowbell", "tambourine",
        "tom", "taiko", "timpani", "woodblock", "agogo", "synthdrum", "reversecymbal",
        "gunshot", "applause", "helicopter", "telephonering", "birdtweet", "fretnoise",
        "breathnoise", "seashore", "orchestrahit", "pizzicato"
    };
    std::string lower = name;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (const auto& tok : ONESHOTS) {
        if (lower.find(tok) != std::string::npos) return true;
    }
    return false;
}

// --------------------------------------------------------------------------
// WAV サンプルデータ & ローダー
// --------------------------------------------------------------------------
struct WavSample {
    std::vector<float> data;
    int sampleRate = 8000;
    int rootMidi = 60;
    bool isOneShot = false;
    int loopStart = -1;
    int loopEnd = -1;

    // SFC ドライバ相当のエンベロープ
    float gain = 1.0f;
    float attackSec = 0.0f;
    float attackCurve = 1.0f;
    float decayT60 = 0.0f;
    float releaseSec = 0.003f;
    float detuneCents = 0.0f;
    bool ignoreNoteOff = false;
};

/**
 * @brief SFC実機相当の整数演算でBRRブロック列を復号する
 * @param brr BRRバイト列
 * @param bytes BRRバイト数
 * @param out 復号先floatバッファ
 * @return なし
 */
static void DecodeBrr(const uint8_t* brr, size_t bytes, std::vector<float>& out) {
    const size_t blocks = bytes / 9;
    out.resize(blocks * 16);
    int p1 = 0, p2 = 0;
    for (size_t b = 0; b < blocks; ++b) {
        const uint8_t* blk = brr + b * 9;
        const int shift = blk[0] >> 4;
        const int filter = (blk[0] >> 2) & 3;
        for (int i = 0; i < 16; ++i) {
            const uint8_t byteVal = blk[1 + i / 2];
            int s = ((i & 1) ? (byteVal & 0x0F) : (byteVal >> 4));
            s = (s ^ 8) - 8;
            s = (shift <= 12) ? ((s << shift) >> 1) : (s < 0 ? -2048 : 0);
            const int a = p1;
            const int c = p2 >> 1;
            switch (filter) {
                case 1: s += (a >> 1) + ((-a) >> 5); break;
                case 2: s += a - c + (c >> 4) + ((a * -3) >> 6); break;
                case 3: s += a - c + ((a * -13) >> 7) + ((c * 3) >> 4); break;
                default: break;
            }
            s = std::clamp(s, -32768, 32767);
            const int16_t stored = static_cast<int16_t>(s << 1);
            p2 = p1;
            p1 = stored;
            out[b * 16 + i] = static_cast<float>(stored) / 32768.0f;
        }
    }
}

/**
 * @brief SBK2形式のBRR音源バンクを解析してキャッシュへ格納する
 * @param data アーカイブ非圧縮データ
 * @param size データ長
 * @param cache 格納先キャッシュ
 * @return 正常に解析された場合true
 */
static bool LoadBrrBank(const uint8_t* data, size_t size, std::map<std::string, WavSample>& cache) {
    constexpr size_t HEADER = 80;
    const uint8_t* ptr = data + 4;
    const uint8_t* endPtr = data + size;
    if (ptr + 4 > endPtr) return false;
    uint32_t count = 0;
    std::memcpy(&count, ptr, 4);
    ptr += 4;

    std::vector<std::string> names;
    names.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (ptr + HEADER > endPtr) return false;
        char nameBuf[33] = {};
        std::memcpy(nameBuf, ptr, 32);
        uint32_t rate = 0, numSamples = 0, brrBytes = 0;
        int32_t loopStart = -1, aliasIndex = -1;
        int16_t root = 60;
        uint16_t flags = 0;
        float f[6] = {};
        std::memcpy(&rate, ptr + 32, 4);
        std::memcpy(&loopStart, ptr + 36, 4);
        std::memcpy(&numSamples, ptr + 40, 4);
        std::memcpy(&brrBytes, ptr + 44, 4);
        std::memcpy(&root, ptr + 48, 2);
        std::memcpy(&flags, ptr + 50, 2);
        std::memcpy(f, ptr + 52, sizeof(f));
        std::memcpy(&aliasIndex, ptr + 76, 4);
        ptr += HEADER;
        if (ptr + brrBytes > endPtr) return false;

        WavSample sample;
        // エイリアス指定時は復号済み波形データを共有
        if (aliasIndex >= 0 && static_cast<size_t>(aliasIndex) < names.size()) {
            sample.data = cache[names[aliasIndex]].data;
        } else {
            DecodeBrr(ptr, brrBytes, sample.data);
        }
        ptr += brrBytes;
        sample.data.resize(numSamples, 0.0f);
        sample.sampleRate = static_cast<int>(rate);
        sample.rootMidi = root;
        sample.gain = f[0];
        sample.attackSec = f[1];
        sample.attackCurve = f[2];
        sample.decayT60 = f[3];
        sample.releaseSec = f[4];
        sample.detuneCents = f[5];
        sample.ignoreNoteOff = (flags & 1) != 0;
        // ループ範囲設定 (終端はデータ終端)
        if (loopStart >= 0 && static_cast<uint32_t>(loopStart) < numSamples) {
            sample.loopStart = loopStart;
            sample.loopEnd = static_cast<int>(numSamples);
            sample.isOneShot = false;
        } else {
            sample.loopStart = -1;
            sample.loopEnd = -1;
            sample.isOneShot = true;
        }
        names.emplace_back(nameBuf);
        cache[names.back()] = std::move(sample);
    }
    return true;
}

/**
 * @brief サンプル1声をミックスする
 * @tparam AddFn サンプル加算関数の型
 * @param w 再生するWAVサンプル情報
 * @param step 読み進めるサンプル増分
 * @param volume 音量
 * @param start 開始サンプル位置
 * @param noteOff ノートオフ位置
 * @param ignoreNoteOff ノートオフを無視するか
 * @param sampleRate 出力サンプルレート
 * @param totalFrames 全フレーム数
 * @param addSample 出力バッファへのサンプル加算関数
 * @return なし
 */
template <typename AddFn>
static void MixSampleVoice(const WavSample& w, double step, float volume, size_t start, size_t noteOff,
                           bool ignoreNoteOff, int sampleRate, size_t totalFrames, const AddFn& addSample) {
    const size_t len = w.data.size();
    const bool looping = !w.isOneShot && w.loopStart >= 0 && w.loopEnd > w.loopStart;
    const double lEnd = looping ? (std::min)(static_cast<double>(w.loopEnd), static_cast<double>(len))
                                : static_cast<double>(len);
    const double lStart = looping ? static_cast<double>(w.loopStart) : 0.0;
    const double loopLen = lEnd - lStart;
    const size_t lStartIdx = static_cast<size_t>(lStart);
    const size_t lEndIdx = static_cast<size_t>(lEnd);
    const float sr = static_cast<float>(sampleRate);
    // 先頭のプチノイズを避けるため、立ち上がりに最低1msを確保する
    const float attackN = (std::max)(w.attackSec, 0.001f) * sr;
    const float decayMul = (w.decayT60 > 0.0f) ? std::pow(10.0f, -3.0f / (w.decayT60 * sr)) : 1.0f;
    const size_t releaseN = (std::max)(static_cast<size_t>(w.releaseSec * sr), static_cast<size_t>(1));
    // バッファ末尾で音が途切れないよう、リリースは遅くとも末尾の手前から始める
    const size_t bufferEndFade = totalFrames > releaseN ? totalFrames - releaseN : 0;
    const size_t releaseAt = (std::min)(ignoreNoteOff ? SIZE_MAX : noteOff, bufferEndFade);

    double pos = 0.0;
    float decayEnv = 1.0f;
    bool inLoop = false;
    // ループとエンベロープを適用しながら波形を合成
    for (size_t i = start; i + 1 < totalFrames; ++i) {
        if (pos >= lEnd) {
            if (!looping || loopLen <= 0.0) break;
            pos = lStart + std::fmod(pos - lStart, loopLen);
        }
        const size_t i0 = static_cast<size_t>(pos);
        const size_t i1 = i0 + 1;
        const float frac = static_cast<float>(pos - static_cast<double>(i0));
        const float s0 = w.data[i0];
        const float s1 = (looping && i1 >= lEndIdx) ? w.data[lStartIdx + (i1 - lEndIdx)]
                                                    : ((i1 < len) ? w.data[i1] : 0.0f);

        float env = 1.0f;
        const float t = static_cast<float>(i - start);
        const bool attacking = attackN > 0.0f && t < attackN;
        if (attacking) env = std::pow(t / attackN, w.attackCurve);
        if (looping && pos >= lStart) inLoop = true;
        if (inLoop && !attacking) decayEnv *= decayMul;
        if (decayEnv < 1e-4f) break;
        env *= decayEnv;
        if (i >= releaseAt) {
            const size_t r = i - releaseAt;
            if (r >= releaseN) break;
            const float x = 1.0f - static_cast<float>(r) / static_cast<float>(releaseN);
            env *= x * x;
        }
        addSample(i, (s0 + frac * (s1 - s0)) * volume * env);
        pos += step;
    }
}

class WavSampleManager {
public:
    /**
     * @brief シングルトンインスタンスを取得する
     * @return WavSampleManagerインスタンスへの参照
     */
    static WavSampleManager& Get() {
        static WavSampleManager instance;
        return instance;
    }

    /**
     * @brief 埋め込みアーカイブを展開してロードする
     * @return 展開に成功した場合true
     */
    bool EnsureEmbeddedLoaded() {
        if (embeddedLoaded_) return true;

        // LZMS展開器を生成
        DECOMPRESSOR_HANDLE hDecomp = nullptr;
        if (!CreateDecompressor(COMPRESS_ALGORITHM_LZMS, nullptr, &hDecomp)) {
            return false;
        }

        std::vector<uint8_t> uncompressed(EMBEDDED_SAMPLE_UNCOMPRESSED_SIZE);
        SIZE_T actualSize = 0;
        BOOL ok = Decompress(
            hDecomp,
            EMBEDDED_SAMPLE_ARCHIVE,
            EMBEDDED_SAMPLE_ARCHIVE_SIZE,
            uncompressed.data(),
            uncompressed.size(),
            &actualSize
        );
        CloseDecompressor(hDecomp);

        if (!ok || actualSize != EMBEDDED_SAMPLE_UNCOMPRESSED_SIZE) {
            return false;
        }

        const uint8_t* ptr = uncompressed.data();
        const uint8_t* endPtr = ptr + actualSize;
        if (ptr + 4 > endPtr) return false;

        // SBK2シグネチャの確認とバンク解析
        if (std::memcmp(ptr, "SBK2", 4) == 0) {
            embeddedLoaded_ = LoadBrrBank(ptr, actualSize, cache_);
            return embeddedLoaded_;
        }

        return false;
    }

    /**
     * @brief 埋め込みバンクにサンプルが存在するか確認する
     * @param baseName サンプル名
     * @return 存在する場合true
     */
    bool HasEmbeddedSample(const std::string& baseName) {
        EnsureEmbeddedLoaded();
        return cache_.find(baseName) != cache_.end();
    }

    /**
     * @brief 指定名のサンプルを取得する
     * @param baseName サンプル名
     * @return サンプル情報へのポインタ、存在しない場合nullptr
     */
    const WavSample* GetSample(const std::string& baseName) {
        // 1. コード埋め込みBRRバンクから優先読み込み
        EnsureEmbeddedLoaded();
        auto it = cache_.find(baseName);
        if (it != cache_.end()) {
            return &(it->second);
        }

        // 2. 外部ファイルから読み込み (フォールバック)
        WavSample sample;
        if (LoadSampleFile(baseName, sample)) {
            sample.rootMidi = GetGmRootMidi(baseName);
            sample.isOneShot = IsOneShotSample(baseName);
            cache_[baseName] = std::move(sample);
            return &(cache_[baseName]);
        }

        return nullptr;
    }

private:
    WavSampleManager() = default;
    std::map<std::string, WavSample> cache_;
    bool embeddedLoaded_ = false;

    /**
     * @brief 外部WAVファイルを検索して読み込む
     * @param baseName サンプル名
     * @param outSample 読み込み先サンプル情報
     * @return 読み込みに成功した場合true
     */
    bool LoadSampleFile(const std::string& baseName, WavSample& outSample) {
        // 外部WAV検索ディレクトリ
        static const std::vector<std::string> SEARCH_DIRS = {
            "Sound/PCMSamples/",
            "x64/Debug/Sound/PCMSamples/",
            "PCMSamples/",
            "Sound/",
        };
        static const std::vector<std::string> SUFFIXES = {
            "",
            "_8000Hz_4bit",
            "_8000Hz_8bit",
            "_4000Hz_8bit",
            "_SFC_8000Hz_4bit"
        };

        for (const auto& dir : SEARCH_DIRS) {
            for (const auto& suf : SUFFIXES) {
                std::string path = dir + baseName + suf + ".wav";
                if (ReadWavFile(path, outSample)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief WAVファイルのバイナリを解析してPCMを展開する
     * @param filePath ファイルパス
     * @param outSample 展開先サンプル情報
     * @return 解析に成功した場合true
     */
    bool ReadWavFile(const std::string& filePath, WavSample& outSample) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;

        char riff[4];
        file.read(riff, 4);
        if (std::memcmp(riff, "RIFF", 4) != 0) return false;

        uint32_t fileSize = 0;
        file.read(reinterpret_cast<char*>(&fileSize), 4);

        char wave[4];
        file.read(wave, 4);
        if (std::memcmp(wave, "WAVE", 4) != 0) return false;

        uint16_t audioFormat = 1;
        uint16_t numChannels = 1;
        uint32_t sampleRate = 8000;
        uint16_t bitsPerSample = 8;
        std::vector<uint8_t> rawData;
        int smplLoopStart = -1;
        int smplLoopEnd = -1;

        while (file) {
            char chunkId[4];
            uint32_t chunkSize = 0;
            if (!file.read(chunkId, 4)) break;
            if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;

            std::streampos chunkDataPos = file.tellg();

            if (std::memcmp(chunkId, "fmt ", 4) == 0) {
                file.read(reinterpret_cast<char*>(&audioFormat), 2);
                file.read(reinterpret_cast<char*>(&numChannels), 2);
                file.read(reinterpret_cast<char*>(&sampleRate), 4);
                uint32_t byteRate = 0;
                file.read(reinterpret_cast<char*>(&byteRate), 4);
                uint16_t blockAlign = 0;
                file.read(reinterpret_cast<char*>(&blockAlign), 2);
                file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
            } else if (std::memcmp(chunkId, "data", 4) == 0) {
                rawData.resize(chunkSize);
                file.read(reinterpret_cast<char*>(rawData.data()), chunkSize);
            } else if (std::memcmp(chunkId, "smpl", 4) == 0 && chunkSize >= 36) {
                file.seekg(28, std::ios::cur);
                uint32_t numLoops = 0;
                file.read(reinterpret_cast<char*>(&numLoops), 4);
                if (numLoops > 0 && chunkSize >= 36 + 24) {
                    file.seekg(8, std::ios::cur);
                    uint32_t lStart = 0, lEnd = 0;
                    file.read(reinterpret_cast<char*>(&lStart), 4);
                    file.read(reinterpret_cast<char*>(&lEnd), 4);
                    smplLoopStart = static_cast<int>(lStart);
                    smplLoopEnd = static_cast<int>(lEnd);
                }
            }

            file.seekg(chunkDataPos + static_cast<std::streamoff>(chunkSize + (chunkSize & 1)));
        }

        if (rawData.empty() || numChannels == 0) return false;

        outSample.sampleRate = static_cast<int>(sampleRate);
        if (bitsPerSample == 8) {
            size_t numFrames = rawData.size() / numChannels;
            outSample.data.resize(numFrames);
            for (size_t i = 0; i < numFrames; ++i) {
                float sum = 0.0f;
                for (size_t c = 0; c < numChannels; ++c) {
                    uint8_t u = rawData[i * numChannels + c];
                    sum += (static_cast<float>(u) - 128.0f) / 128.0f;
                }
                outSample.data[i] = sum / numChannels;
            }
        } else if (bitsPerSample == 16) {
            size_t numFrames = rawData.size() / (numChannels * 2);
            const int16_t* p16 = reinterpret_cast<const int16_t*>(rawData.data());
            outSample.data.resize(numFrames);
            for (size_t i = 0; i < numFrames; ++i) {
                float sum = 0.0f;
                for (size_t c = 0; c < numChannels; ++c) {
                    int16_t s = p16[i * numChannels + c];
                    sum += static_cast<float>(s) / 32768.0f;
                }
                outSample.data[i] = sum / numChannels;
            }
        } else {
            return false;
        }

        if (smplLoopStart >= 0 && smplLoopEnd > smplLoopStart && smplLoopEnd <= static_cast<int>(outSample.data.size())) {
            outSample.loopStart = smplLoopStart;
            outSample.loopEnd = smplLoopEnd;
        } else {
            outSample.loopStart = static_cast<int>(0.06f * outSample.sampleRate);
            outSample.loopEnd = static_cast<int>(std::min(outSample.data.size(), static_cast<size_t>(0.45f * outSample.sampleRate)));
        }

        return true;
    }
};

#ifdef FLOPPY_AUDIO_TESTS
/**
 * @brief BRR復号と埋め込みバンク読み込みおよび全音色解決を検証する
 * @return なし
 */
void RunSamplePlaybackChecks() {
    // 1. BRR 1ブロックが期待値どおり復号されることを検証
    // ヘッダ: 0x10 (shift=1, filter=0), 各nibble=1 (0x11が8バイト) -> s=1, (1<<1)>>1=1, stored=2, 2.0f/32768.0f
    const uint8_t testBrr[9] = { 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
    std::vector<float> decoded;
    DecodeBrr(testBrr, sizeof(testBrr), decoded);
    assert(decoded.size() == 16);
    for (size_t i = 0; i < 16; ++i) {
        assert(std::abs(decoded[i] - (2.0f / 32768.0f)) < 1e-6f);
    }

    // 2. バンクが正常に読み込めることを検証
    auto& mgr = WavSampleManager::Get();
    assert(mgr.EnsureEmbeddedLoaded());

    // 3. ゲームが要求する名前がすべて埋め込みバンクから解決されることを検証 (外部WAV不要)
    // ドラム全16種
    for (int note = 35; note <= 83; ++note) {
        const char* drumName = ResolveDrumName(note);
        assert(mgr.HasEmbeddedSample(drumName));
    }
    // GM全128音色
    for (int prog = 0; prog < 128; ++prog) {
        int resolvedProg = ResolveWavProgram(prog);
        const char* instName = GM_INSTRUMENT_NAMES[resolvedProg];
        assert(mgr.HasEmbeddedSample(instName));
    }
}
#endif

// --------------------------------------------------------------------------
// 従来の疑似合成音プロバイダー
// --------------------------------------------------------------------------
SFCPCMSampleProvider::SFCPCMSampleProvider() = default;

float SFCPCMSampleProvider::Sample(uint8_t instrumentId, float phase, float frequency, int /*sampleRate*/, float t, bool /*isNoteStart*/) {
    if (instrumentId == 0) {
        float hammerNoise = 0.0f;
        if (t < 0.10f) {
            float noiseEnv = std::exp(-t * 90.0f);
            hammerNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * noiseEnv * 0.4f;
        }

        float loopSec = 0.16f;
        float effectiveT = t;
        if (t > 0.12f) {
            effectiveT = 0.12f + std::fmod(t - 0.12f, loopSec);
        }

        float fund = std::sin(2.0f * PI_F * frequency * effectiveT);
        float h2 = 0.40f * std::sin(2.0f * PI_F * (frequency * 2.0f) * effectiveT);
        float h3 = 0.20f * std::sin(2.0f * PI_F * (frequency * 3.0f) * effectiveT);
        float env = std::exp(-t * 1.5f);

        return (fund + h2 + h3 + hammerNoise) * env;
    }

    if (instrumentId == 1) {
        float pickSnap = 0.0f;
        if (t < 0.08f) {
            float snapEnv = std::exp(-t * 160.0f);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float pluck = std::sin(2.0f * PI_F * 880.0f * t) * std::exp(-t * 100.0f);
            pickSnap = (noise * 0.7f + pluck * 0.4f) * snapEnv;
        }

        float loopSec = 0.14f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float sub = std::sin(2.0f * PI_F * frequency * effectiveT);
        float saw = (std::fmod(effectiveT * frequency, 1.0f) - 0.5f) * 0.4f;
        float env = std::exp(-t * 1.8f);

        return (sub + saw + pickSnap) * env;
    }

    if (instrumentId == 2) {
        float pickNoise = 0.0f;
        if (t < 0.06f) {
            pickNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 100.0f) * 0.3f;
        }

        float loopSec = 0.16f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float raw = (std::fmod(effectiveT * frequency, 1.0f) - 0.5f) + pickNoise;
        float dist = std::tanh(raw * 7.0f);
        if (dist > 0.4f) dist = 0.4f + (dist - 0.4f) * 0.2f;
        dist *= 1.8f;

        float env = std::exp(-t * 1.4f);
        return dist * env;
    }

    if (instrumentId == 3) {
        float pluckNoise = 0.0f;
        if (t < 0.08f) {
            pluckNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 110.0f) * 0.4f;
        }

        float loopSec = 0.17f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float fund = std::sin(2.0f * PI_F * frequency * effectiveT);
        float bodyRes = 0.30f * std::sin(2.0f * PI_F * (frequency * 1.5f) * effectiveT);
        float env = std::exp(-t * 2.2f);

        return (fund + bodyRes + pluckNoise) * env;
    }

    if (instrumentId == 5) {
        // ブラス (アタックで滑らかに最大音量になり、ロングトーンも美しく伸びる)
        float brassAttack = std::sin(PI_F * 0.5f * std::min(1.0f, t / 0.06f));
        float saw1 = (phase - 0.5f) * 2.0f;
        float saw2 = (std::fmod(phase * 1.003f, 1.0f) - 0.5f) * 2.0f;
        float env = brassAttack * std::exp(-t * 0.25f);

        return (saw1 + saw2) * 0.4f * env;
    }

    if (instrumentId == 6) {
        // ストリングス / リード
        float stringsAttack = std::sin(PI_F * 0.5f * std::min(1.0f, t / 0.12f));
        float saw1 = (phase - 0.5f) * 2.0f;
        float saw2 = (std::fmod(phase * 1.005f, 1.0f) - 0.5f) * 2.0f;
        float env = stringsAttack * std::exp(-t * 0.20f);

        return (saw1 + saw2) * 0.35f * env;
    }

    if (instrumentId == 8 || instrumentId == 7) {
        // キックドラム / タム (ノート周波数 frequency に応じて音高が変化する)
        float pitchScale = std::clamp(frequency / 110.0f, 0.3f, 4.0f);
        float pEnv = (240.0f * pitchScale) * std::exp(-t * 40.0f) + (50.0f * pitchScale);
        float click = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 180.0f) * 0.5f;
        float body = std::sin(2.0f * PI_F * pEnv * t);
        float env = std::exp(-t * 12.0f);
        return (body + click) * env * 2.5f;
    }

    if (instrumentId == 9) {
        // スネアドラム
        float pitchScale = std::clamp(frequency / 180.0f, 0.5f, 2.0f);
        float tone = std::sin(2.0f * PI_F * (320.0f * pitchScale * std::exp(-t * 25.0f)) * t) * std::exp(-t * 18.0f);
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 14.0f);
        return (tone * 0.8f + noise * 0.9f) * 2.0f;
    }

    if (instrumentId == 10) {
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 60.0f);
        return noise * env * 0.8f;
    }

    if (instrumentId == 11) {
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 10.0f);
        return noise * env * 0.8f;
    }

    if (instrumentId == 12) {
        float f1 = std::sin(2.0f * PI_F * 3420.0f * t);
        float f2 = std::sin(2.0f * PI_F * 5130.0f * t);
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 5.0f);
        return (noise * 0.6f + (f1 + f2) * 0.2f) * env;
    }

    if (instrumentId == 4) {
        if (frequency < 140.0f) {
            float pitchScale = std::clamp(frequency / 110.0f, 0.3f, 4.0f);
            float pEnv = (240.0f * pitchScale) * std::exp(-t * 40.0f) + (50.0f * pitchScale);
            float click = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 180.0f) * 0.5f;
            float body = std::sin(2.0f * PI_F * pEnv * t);
            float env = std::exp(-t * 12.0f);
            return (body + click) * env * 2.5f;
        } 
        else if (frequency >= 140.0f && frequency < 220.0f) {
            float pitchScale = std::clamp(frequency / 180.0f, 0.5f, 2.0f);
            float tone = std::sin(2.0f * PI_F * (320.0f * pitchScale * std::exp(-t * 25.0f)) * t) * std::exp(-t * 18.0f);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 14.0f);
            return (tone * 0.8f + noise * 0.9f) * 2.0f;
        }
        else if (frequency >= 220.0f && frequency < 350.0f) {
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 60.0f);
            return noise * env * 0.8f;
        }
        else if (frequency >= 350.0f && frequency < 500.0f) {
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 10.0f);
            return noise * env * 0.8f;
        }
        else {
            float f1 = std::sin(2.0f * PI_F * 3420.0f * t);
            float f2 = std::sin(2.0f * PI_F * 5130.0f * t);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 5.0f);
            return (noise * 0.6f + (f1 + f2) * 0.2f) * env;
        }
    }

    return std::sin(2.0f * PI_F * frequency * t);
}

SynthWaveGenerator::SynthWaveGenerator() {
    customProvider_ = std::make_shared<SFCPCMSampleProvider>();
}

std::vector<int16_t> SynthWaveGenerator::GeneratePCM(const MMLSequence& sequence, int sampleRate, bool stereo, bool normalizeBgm) {
    if (sampleRate <= 0 || sequence.tracks.empty() || sequence.maxDurationSec <= 0.0) {
        return {};
    }

    size_t totalSamples = static_cast<size_t>(sequence.maxDurationSec * sampleRate);
    if (totalSamples == 0) {
        return {};
    }

    const size_t channels = stereo ? 2 : 1;
    std::vector<float> mixBuffer(totalSamples * channels, 0.0f);
    int fadeSamples = std::max(1, static_cast<int>(fadeTimeSec_ * sampleRate));
    auto& sampleMgr = WavSampleManager::Get();

    int trackIndex = 0;
    for (const auto& track : sequence.tracks) {
        uint32_t lfsr = 0x7FFF;

        for (const auto& event : track.events) {
            size_t startSample = static_cast<size_t>(event.startTimeSec * sampleRate);
            size_t durationSamples = static_cast<size_t>(event.durationSec * sampleRate);

            if (startSample >= totalSamples || durationSamples == 0) {
                continue;
            }

            // 主旋律・低音・打楽器を中央に残し、他のトラックを控えめに左右へ配置する
            constexpr float Pans[] = {0.0f, -0.2f, 0.2f, -0.35f, 0.35f};
            const bool bass = event.waveType == WaveformType::Custom && event.customWaveId >= 32 && event.customWaveId <= 39;
            const float pan = stereo && !event.isDrum && !bass && event.frequency >= 130 ? Pans[trackIndex % 5] : 0;
            const auto addSample = [&](size_t frame, float value) {
                mixBuffer[frame * channels] += value * (1 - pan);
                if (stereo) mixBuffer[frame * channels + 1] += value * (1 + pan);
            };

            // 1. WAV サンプルによる PCM 再生 (Custom 音色 または ドラム)
            if (event.waveType == WaveformType::Custom || event.isDrum) {
                std::string sampleName;
                if (event.isDrum) {
                    sampleName = ResolveDrumName(event.drumMidiNote);
                } else {
                    int prog = ResolveWavProgram(event.customWaveId);
                    sampleName = GM_INSTRUMENT_NAMES[prog];
                }

                const WavSample* wav = sampleMgr.GetSample(sampleName);
                if (wav && !wav->data.empty()) {
                    float rootFreq = 440.0f * std::pow(2.0f, (wav->rootMidi - 69) / 12.0f);
                    float pitchRatio = event.isDrum ? 1.0f : (event.frequency / rootFreq);
                    const double step = static_cast<double>(wav->sampleRate) / static_cast<double>(sampleRate) * pitchRatio;
                    const float vol = event.volume * wav->gain;
                    const bool ignoreOff = event.isDrum || wav->ignoreNoteOff;
                    const size_t noteOffSample = startSample + durationSamples;

                    if (wav->detuneCents > 0.0f) {
                        // 合奏の厚み: 中央 + 上下にずらした 3 声 (非対称比率と位相遅延)
                        const double up = std::pow(2.0, wav->detuneCents * 0.5 / 1200.0);
                        const double down = std::pow(2.0, -wav->detuneCents * 0.31 / 1200.0);
                        const size_t lag1 = static_cast<size_t>(0.0011 * sampleRate);
                        const size_t lag2 = static_cast<size_t>(0.0023 * sampleRate);
                        MixSampleVoice(*wav, step, vol * 0.58f, startSample, noteOffSample, ignoreOff, sampleRate, totalSamples, addSample);
                        if (startSample + lag1 < totalSamples) {
                            MixSampleVoice(*wav, step * up, vol * 0.46f, startSample + lag1, noteOffSample, ignoreOff, sampleRate, totalSamples, addSample);
                        }
                        if (startSample + lag2 < totalSamples) {
                            MixSampleVoice(*wav, step * down, vol * 0.46f, startSample + lag2, noteOffSample, ignoreOff, sampleRate, totalSamples, addSample);
                        }
                    } else {
                        MixSampleVoice(*wav, step, vol, startSample, noteOffSample, ignoreOff, sampleRate, totalSamples, addSample);
                    }
                    continue;
                }
            }

            // 2. WAV が見つからない場合 または チップ波形（Square / Pulse / Tri / Noise）の合成
            size_t endSample = std::min(totalSamples, startSample + durationSamples);
            int noteFadeSamples = std::min(fadeSamples, static_cast<int>(durationSamples / 2));
            float phase = 0.0f;
            float phaseIncrement = event.frequency / static_cast<float>(sampleRate);

            for (size_t s = startSample; s < endSample; ++s) {
                size_t sampleIndexInNote = s - startSample;
                float timeInNoteSec = static_cast<float>(sampleIndexInNote) / static_cast<float>(sampleRate);

                // Envelope (Attack & Release)
                float env = 1.0f;
                if (sampleIndexInNote < static_cast<size_t>(noteFadeSamples)) {
                    env = static_cast<float>(sampleIndexInNote) / static_cast<float>(noteFadeSamples);
                }
                // 合成音も最終サンプルまでに無音へ戻す
                const size_t remaining = endSample - 1 - s;
                if (remaining < static_cast<size_t>(noteFadeSamples)) {
                    env *= static_cast<float>(remaining) / static_cast<float>(noteFadeSamples);
                }

                if (event.waveType == WaveformType::Noise) {
                    uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
                    lfsr = (lfsr >> 1) | (bit << 14);
                }

                float oscSample = GenerateOscillatorSample(
                    event.waveType, event.customWaveId, phase, event.frequency, sampleRate, lfsr, timeInNoteSec, trackIndex
                );

                addSample(s, oscSample * event.volume * env);

                phase += phaseIncrement;
                if (phase >= 1.0f) {
                    phase -= std::floor(phase);
                }
            }
        }
        trackIndex++;
    }

    std::vector<int16_t> pcmBuffer(mixBuffer.size());
    float trackCountFactor = std::sqrt(static_cast<float>(std::max<size_t>(1, sequence.tracks.size())));
    float scale = 0.55f / trackCountFactor;

    // 曲全体に同じ倍率を掛け、フレーズの強弱と左右の音量比を保つ
    if (normalizeBgm) {
        // ponytail: 400ms区間のRMSで簡易補正する、聴感の厳密な一致が必要ならLUFS計測へ置換する
        const size_t blockSize = std::max<size_t>(1, sampleRate * 2 / 5) * channels;
        std::vector<double> energies;
        double energySum = 0;
        float peak = 0;
        for (size_t start = 0; start < mixBuffer.size(); start += blockSize) {
            const size_t end = std::min(start + blockSize, mixBuffer.size());
            double energy = 0;
            for (size_t i = start; i < end; ++i) {
                energy += static_cast<double>(mixBuffer[i]) * mixBuffer[i];
                peak = std::max(peak, std::abs(mixBuffer[i]));
            }
            energy /= end - start;
            if (energy > 1e-8) {
                energies.push_back(energy);
                energySum += energy;
            }
        }
        // 無音と平均より10dB以上小さい区間を除外し、増幅とピークに上限を設ける
        if (!energies.empty()) {
            const double gate = energySum / energies.size() * 0.1;
            double gatedEnergy = 0;
            size_t count = 0;
            for (double energy : energies) {
                if (energy >= gate) { gatedEnergy += energy; ++count; }
            }
            scale = std::min({ 4.0f, static_cast<float>(0.1 / std::sqrt(gatedEnergy / count)), 0.8f / peak });
        }
    }

    for (size_t i = 0; i < mixBuffer.size(); ++i) {
        float sample = mixBuffer[i] * scale;
        sample = normalizeBgm ? sample : std::tanh(sample);
        pcmBuffer[i] = static_cast<int16_t>(sample * (normalizeBgm ? 32767.0f : 30000.0f));
    }

    return pcmBuffer;
}

float SynthWaveGenerator::GenerateOscillatorSample(
    WaveformType type, uint8_t customId, float phase, float frequency, int sampleRate, uint32_t lfsrState, float timeInNoteSec, int /*trackIndex*/) 
{
    uint8_t instrumentId = customId;
    if (customProvider_) {
        return customProvider_->Sample(instrumentId, phase, frequency, sampleRate, timeInNoteSec, (timeInNoteSec == 0.0f));
    }

    switch (type) {
        case WaveformType::Square:
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case WaveformType::Pulse25:
            return (phase < 0.25f) ? 1.0f : -1.0f;
        case WaveformType::Pulse12_5:
            return (phase < 0.125f) ? 1.0f : -1.0f;
        case WaveformType::Triangle:
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case WaveformType::Sawtooth:
            return 2.0f * phase - 1.0f;
        case WaveformType::Noise:
            return (lfsrState & 1) ? 1.0f : -1.0f;
        default:
            return (phase < 0.5f) ? 1.0f : -1.0f;
    }
}

} // namespace Audio
