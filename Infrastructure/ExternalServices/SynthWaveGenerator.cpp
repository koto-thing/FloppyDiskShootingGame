#include "SynthWaveGenerator.h"
#include "WavSamples.h"
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

static int ResolveWavProgram(int prog) {
    static const std::map<int, int> FALLBACK = {
        {1, 0}, {2, 0}, {3, 0}, {5, 4}, {17, 16}, {20, 19}, {22, 21}, {23, 21},
        {25, 24}, {26, 28}, {27, 28}, {31, 24}, {37, 36}, {39, 38}, {41, 40},
        {42, 40}, {43, 32}, {46, 15}, {49, 48}, {51, 50}, {63, 62}, {66, 65},
        {67, 65}, {69, 68}, {71, 68}, {72, 73}, {74, 73}, {77, 75}, {85, 54},
        {88, 94}, {89, 90}, {91, 52}, {92, 48}, {93, 90}, {95, 90}, {96, 119},
        {97, 94}, {98, 8}, {99, 94}, {100, 90}, {101, 83}, {102, 90}, {103, 80},
        {104, 107}, {106, 107}, {108, 12}, {110, 40}, {111, 68}, {112, 10}, {114, 11},
        {121, 120}, {122, 119}, {123, 78}, {124, 10}, {125, 116}, {126, 55}, {127, 55}
    };
    if (prog < 0 || prog >= 128) prog = 0;
    auto it = FALLBACK.find(prog);
    return (it != FALLBACK.end()) ? it->second : prog;
}

static int GetGmRootMidi(const std::string& name) {
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

static const char* ResolveDrumName(int midiNote) {
    switch (midiNote) {
        case 35: case 36: return "Kick";
        case 37: case 38: case 39: case 40: return "Snare";
        case 41: case 43: return "TomLow";
        case 45: case 47: return "TomMid";
        case 48: case 50: return "TomHigh";
        case 42: case 44: return "HiHatClosed";
        case 46: return "HiHatOpen";
        case 49: case 55: case 57: return "Crash";
        case 51: case 53: case 59: return "Ride";
        case 54: return "Tambourine";
        case 56: case 58: return "Woodblock";
        case 67: case 68: return "Agogo";
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

static bool IsOneShotSample(const std::string& name) {
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
    int loopStart = 0;
    int loopEnd = 0;
    mutable std::map<std::pair<int, bool>, std::vector<float>> playbackCache;

    /**
     * @brief ループを接続し、移調先のナイキスト周波数に合わせた波形を取得する
     * @param step 出力1サンプルあたりに進む入力サンプル数
     * @param loop ループ再生する場合true
     * @return 元波形またはキャッシュされた再生用波形
     */
    const std::vector<float>& GetPlaybackData(double step, bool loop) const {
        if (step <= 1.0 && !loop) return data;
        // ponytail: フィルターは移調比1/16刻みで共有し、より厳密な帯域が必要なら刻みを細かくする
        const int rateKey = static_cast<int>(std::ceil(std::max(1.0, step) * 16));
        const auto key = std::make_pair(rateKey, loop);
        if (const auto found = playbackCache.find(key); found != playbackCache.end()) return found->second;

        // 入力は保持し、最大4msのクロスフェードをループ長を変えずに適用する
        std::vector<float> prepared = data;
        if (loop) {
            const int fade = std::min({sampleRate / 250, (loopEnd - loopStart) / 4, loopStart});
            for (int i = 0; i < fade; ++i) {
                prepared[loopEnd - fade + i] = std::lerp(data[loopEnd - fade + i],
                    data[loopStart - fade + i], static_cast<float>(i + 1) / fade);
            }
        }

        // 高い移調だけ事前に63タップの窓付きsincで帯域制限し、発音中の計算を増やさない
        if (rateKey > 16) {
            constexpr int Radius = 31;
            std::array<double, Radius * 2 + 1> kernel{};
            const double cutoff = 0.9 * 16 / rateKey;
            double sum = 0;
            for (int tap = -Radius; tap <= Radius; ++tap) {
                const double sinc = tap == 0 ? cutoff : std::sin(std::numbers::pi * cutoff * tap) / (std::numbers::pi * tap);
                const double window = 0.42 + 0.5 * std::cos(std::numbers::pi * tap / Radius) +
                    0.08 * std::cos(2 * std::numbers::pi * tap / Radius);
                kernel[tap + Radius] = sinc * window;
                sum += kernel[tap + Radius];
            }
            const auto input = prepared;
            for (int i = 0; i < static_cast<int>(input.size()); ++i) {
                double value = 0;
                for (int tap = -Radius; tap <= Radius; ++tap) {
                    int index = i + tap;
                    // 持続部分は周期境界で畳み込み、アタックとワンショットは端点を延長する
                    if (loop && i >= loopStart && i < loopEnd) {
                        const int length = loopEnd - loopStart;
                        index = loopStart + ((index - loopStart) % length + length) % length;
                    }
                    value += input[std::clamp(index, 0, static_cast<int>(input.size()) - 1)] * kernel[tap + Radius];
                }
                prepared[i] = static_cast<float>(value / sum);
            }
        }
        return playbackCache.emplace(key, std::move(prepared)).first->second;
    }
};

#ifdef FLOPPY_AUDIO_TESTS
/**
 * @brief 既知の二音を使い、移調時の帯域制限と原波形の保持を検証する
 * @return なし
 */
void RunSamplePlaybackChecks() {
    // 2倍速で折り返す高域だけを除去し、通過帯域の音量を維持する
    WavSample sample;
    for (int i = 0; i < 1200; ++i) {
        sample.data.push_back(static_cast<float>(std::sin(2 * std::numbers::pi * 0.1 * i) +
            std::sin(2 * std::numbers::pi * 0.4 * i)));
    }
    const auto original = sample.data;
    const auto& filtered = sample.GetPlaybackData(2, false);
    double lowAmplitude = 0;
    double highAmplitude = 0;
    for (int i = 100; i < 1100; ++i) {
        lowAmplitude += filtered[i] * std::sin(2 * std::numbers::pi * 0.1 * i) / 500;
        highAmplitude += filtered[i] * std::sin(2 * std::numbers::pi * 0.4 * i) / 500;
    }
    assert(std::abs(lowAmplitude - 1) < 0.01);
    assert(std::abs(highAmplitude) < 0.01);
    assert(sample.data == original);
    assert(&filtered == &sample.GetPlaybackData(2, false));
    assert(&sample.data == &sample.GetPlaybackData(1, false));
}
#endif

class WavSampleManager {
public:
    static WavSampleManager& Get() {
        static WavSampleManager instance;
        return instance;
    }

    const WavSample* GetSample(const std::string& baseName) {
        auto it = cache_.find(baseName);
        if (it != cache_.end()) {
            return &(it->second);
        }

        WavSample sample;
        // 1. コード埋め込み圧縮サンプルから優先読み込み
        const auto* wav = WavSamples::Find(baseName);
        if (wav && ReadPackedSample(*wav, sample)) {
            sample.rootMidi = GetGmRootMidi(baseName);
            sample.isOneShot = IsOneShotSample(baseName);
            cache_[baseName] = std::move(sample);
            return &(cache_[baseName]);
        }

        // 2. 外部ファイルから読み込み (フォールバック)
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

    /**
     * @brief 埋め込み差分圧縮PCMを再生用サンプルへ復号する
     * @param packed 圧縮PCMサンプル
     * @param outSample 復号先
     * @return 復号に成功した場合true
     */
    bool ReadPackedSample(const WavSamples::WavData& packed, WavSample& outSample) {
        std::vector<std::uint8_t> pcm;
        if (!WavSamples::Decode(packed, pcm)) return false;

        // 8bit符号なしPCMをミキサー用floatへ変換する
        outSample.sampleRate = 8000;
        outSample.data.resize(pcm.size());
        std::transform(pcm.begin(), pcm.end(), outSample.data.begin(), [](std::uint8_t value) {
            return (static_cast<float>(value) - 128.0f) / 128.0f;
        });

        // WAVに記録されていたループ範囲を復元する
        if (packed.loopStart >= 0 && packed.loopEnd > packed.loopStart &&
            packed.loopEnd <= static_cast<int>(outSample.data.size())) {
            outSample.loopStart = packed.loopStart;
            outSample.loopEnd = packed.loopEnd;
        } else {
            outSample.loopStart = static_cast<int>(0.06f * outSample.sampleRate);
            outSample.loopEnd = static_cast<int>(std::min(
                outSample.data.size(), static_cast<std::size_t>(0.45f * outSample.sampleRate)));
        }
        return true;
    }

    bool LoadSampleFile(const std::string& baseName, WavSample& outSample) {
        static const std::vector<std::string> SEARCH_DIRS = {
            "Sound/PCMSamples/",
            "x64/Debug/Sound/PCMSamples/",
            "PCMSamples/",
            "Sound/",
            "../MakePCMSample/wav samples/",
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
                    bool oneShot = event.isDrum || wav->isOneShot;
                    float rootFreq = 440.0f * std::pow(2.0f, (wav->rootMidi - 69) / 12.0f);
                    float pitchRatio = event.isDrum ? 1.0f : (event.frequency / rootFreq);
                    double step = pitchRatio * (static_cast<double>(wav->sampleRate) / sampleRate);
                    if (step <= 0.0f) step = 1.0f;

                    size_t playSamples = durationSamples;
                    if (oneShot) {
                        playSamples = static_cast<size_t>(std::ceil(wav->data.size() / static_cast<double>(step)));
                    }

                    size_t endSample = std::min(totalSamples, startSample + playSamples);
                    double pos = 0.0;
                    bool looped = false;
                    const bool hasLoop = !oneShot && wav->loopEnd > wav->loopStart;
                    const auto& playbackData = wav->GetPlaybackData(step, hasLoop);
                    int fadeAttack = std::min(fadeSamples, static_cast<int>(durationSamples / 4));
                    int fadeRelease = std::min(static_cast<int>((oneShot ? 0.002f : 0.015f) * sampleRate),
                        static_cast<int>((endSample - startSample) / 2));

                    for (size_t s = startSample; s < endSample; ++s) {
                        size_t sampleIndexInNote = s - startSample;
                        float env = 1.0f;
                        if (sampleIndexInNote < static_cast<size_t>(fadeAttack)) {
                            env = static_cast<float>(sampleIndexInNote) / static_cast<float>(fadeAttack);
                        }
                        // 実際の再生末尾を0へ落とし、打楽器の終端や曲末の切断音も抑える
                        const size_t remaining = endSample - 1 - s;
                        if (remaining < static_cast<size_t>(fadeRelease)) {
                            env *= static_cast<float>(remaining) / static_cast<float>(fadeRelease);
                        }

                        // 補間の参照点もループ内へ折り返し、ループ外の波形の混入を防ぐ
                        const auto readSample = [&](int index) {
                            if (hasLoop && (index >= wav->loopEnd || (looped && index < wav->loopStart))) {
                                const int length = wav->loopEnd - wav->loopStart;
                                index = wav->loopStart + ((index - wav->loopStart) % length + length) % length;
                            }
                            return playbackData[std::clamp(index, 0, static_cast<int>(playbackData.size()) - 1)];
                        };
                        const int index = static_cast<int>(pos);
                        const float frac = static_cast<float>(pos - index);
                        const float p0 = readSample(index - 1);
                        const float p1 = readSample(index);
                        const float p2 = readSample(index + 1);
                        const float p3 = readSample(index + 2);
                        // 帯域制限済みの波形を4点補間する
                        const float wavVal = p1 + 0.5f * frac * (p2 - p0 + frac *
                            (2 * p0 - 5 * p1 + 4 * p2 - p3 + frac * (3 * (p1 - p2) + p3 - p0)));

                        addSample(s, wavVal * event.volume * env * 0.7f);

                        pos += step;
                        if (hasLoop) {
                            if (pos >= static_cast<float>(wav->loopEnd)) {
                                float loopLen = static_cast<float>(wav->loopEnd - wav->loopStart);
                                pos = wav->loopStart + std::fmod(pos - wav->loopStart, loopLen);
                                looped = true;
                            }
                        } else {
                            if (pos >= static_cast<float>(wav->data.size())) {
                                break;
                            }
                        }
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
