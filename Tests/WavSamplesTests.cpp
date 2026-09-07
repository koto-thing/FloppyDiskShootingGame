#include <cassert>
#include <cstdint>
#include <vector>

#include "../Infrastructure/ExternalServices/WavSamples.h"
#include "../Infrastructure/ExternalServices/WavSamplesSource.h"
#include "../Presentation/Gameplay/Voices/VoiceSamplesSource.h"
#include "../Presentation/Gameplay/Voices/VoiceDpcmDecoder.h"
#include "../Infrastructure/ExternalServices/SynthWaveGenerator.h"
#include "../Infrastructure/ExternalServices/MMLData.h"
#include "../Infrastructure/ExternalServices/MMLSource.h"
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace Audio { void RunSamplePlaybackChecks(); }
namespace Audio::PackedAudioSamples { void RunFailureChecks(); }

/**
 * @brief 埋め込みPCMの差分圧縮データを検証する
 * @return なし
 */
void RunWavSamplesTests() {
    Audio::PackedAudioSamples::RunFailureChecks();
    // 全40楽器の追加圧縮前バイト列と復元後バイト列が完全一致する
    static_assert(std::size(Audio::WavSamples::SAMPLES) == 40);
    static_assert(std::size(Audio::WavSamplesSource::SAMPLES) == 40);
    for (const auto& original : Audio::WavSamplesSource::SAMPLES) {
        const auto* sample = Audio::WavSamples::Find(original.name);
        assert(sample && sample->byteCount == original.byteCount);
        assert(sample->sampleCount == original.sampleCount && sample->loopStart == original.loopStart &&
            sample->loopEnd == original.loopEnd && sample->checksum == original.checksum);
        const auto* restored = Audio::PackedAudioSamples::Get(sample->offset, sample->byteCount);
        assert(restored && std::equal(original.data, original.data + original.byteCount, restored));
    }
    // 全14声も一致を検証し、既存ADPCM復号器まで通して長さを確認する
    const std::pair<VoiceSamples::ImaAdpcmSample, VoiceSamplesSource::ImaAdpcmSample> voices[] = {
        {VoiceSamples::tayamaDeath, VoiceSamplesSource::tayamaDeath},
        {VoiceSamples::eastsourceDeath, VoiceSamplesSource::eastsourceDeath},
        {VoiceSamples::momijiDeath, VoiceSamplesSource::momijiDeath},
        {VoiceSamples::mission, VoiceSamplesSource::mission},
        {VoiceSamples::start, VoiceSamplesSource::start},
        {VoiceSamples::arrested, VoiceSamplesSource::arrested},
        {VoiceSamples::suspect, VoiceSamplesSource::suspect},
        {VoiceSamples::kotoDeath, VoiceSamplesSource::kotoDeath},
        {VoiceSamples::ryotaDeath, VoiceSamplesSource::ryotaDeath},
        {VoiceSamples::lumiDeath, VoiceSamplesSource::lumiDeath},
        {VoiceSamples::botamochiDeathBota, VoiceSamplesSource::botamochiDeathBota},
        {VoiceSamples::botamochiDeathMochi, VoiceSamplesSource::botamochiDeathMochi},
        {VoiceSamples::momijiDeathRattle2, VoiceSamplesSource::momijiDeathRattle2},
        {VoiceSamples::momijiDeathRattle5, VoiceSamplesSource::momijiDeathRattle5},
    };
    static_assert(std::size(voices) == 14);
    for (const auto& [sample, original] : voices) {
        assert(sample.byteCount == original.byteCount);
        assert(sample.sampleCount == original.sampleCount && sample.sampleRate == original.sampleRate &&
            sample.initialPredictor == original.initialPredictor && sample.initialStepIndex == original.initialStepIndex);
        const auto* restored = Audio::PackedAudioSamples::Get(sample.offset, sample.byteCount);
        assert(restored && std::equal(original.data, original.data + original.byteCount, restored));
        assert(VoiceCodec::DecodeImaAdpcm(sample).size() == original.sampleCount);
    }
    // バッファ境界、空入力、加算オーバーフロー相当の範囲外を拒否する
    assert(!Audio::PackedAudioSamples::Get(0, 0));
    assert(!Audio::PackedAudioSamples::Get(Audio::PackedAudioSamples::TotalSize, 1));
    assert(!Audio::PackedAudioSamples::Get(static_cast<std::size_t>(-1), 1));
    assert(!Audio::PackedAudioSamples::Get(1, static_cast<std::size_t>(-1)));
    auto invalidVoice = VoiceSamples::mission;
    invalidVoice.offset = Audio::PackedAudioSamples::TotalSize;
    assert(VoiceCodec::DecodeImaAdpcm(invalidVoice).empty());
    invalidVoice = VoiceSamples::mission;
    invalidVoice.byteCount = 1;
    assert(VoiceCodec::DecodeImaAdpcm(invalidVoice).empty());
    assert(Audio::PackedAudioSamples::Get(0, 1) == Audio::PackedAudioSamples::Get(0, 1));
    Audio::RunSamplePlaybackChecks();
    // 圧縮・復元で全14曲の原譜が1文字も変化しないことを確認する
    const std::pair<std::string_view, std::string_view> scores[] = {
        {"stage1", MMLSource::stage1}, {"stage2", MMLSource::stage2}, {"stage3", MMLSource::stage3},
        {"stage4", MMLSource::stage4}, {"stage5", MMLSource::stage5}, {"stage6", MMLSource::stage6},
        {"boss1", MMLSource::boss1}, {"boss2", MMLSource::boss2}, {"boss3", MMLSource::boss3},
        {"boss4", MMLSource::boss4}, {"boss5", MMLSource::boss5}, {"boss6", MMLSource::boss6},
        {"title", MMLSource::title}, {"ending", MMLSource::ending}
    };
    for (const auto& [name, original] : scores) {
        const auto restored = MMLData::GetByName(name);
        if (restored != original) {
            std::cerr << name << ": restored=" << restored.size() << ", original=" << original.size() << '\n';
        }
        assert(restored == original);
    }
    assert(MMLData::GetByName("unknown").empty());
    assert(MMLData::GetTitleBgm() == MMLSource::title);
    assert(MMLData::GetEndingBgm() == MMLSource::ending);
    assert(MMLData::GetStageBgm(99) == MMLSource::stage1);
    assert(MMLData::GetBossBgm(99) == MMLSource::boss1);

    for (const auto& sample : Audio::WavSamples::SAMPLES) {
        std::vector<std::uint8_t> pcm;

        // 全サンプルを復号し、変換前PCMのサイズとチェックサムを照合する
        assert(Audio::WavSamples::Decode(sample, pcm));
        assert(pcm.size() == sample.sampleCount);
        std::uint32_t checksum = 2166136261u;
        for (const std::uint8_t value : pcm) {
            checksum = (checksum ^ value) * 16777619u;
        }
        assert(checksum == sample.checksum);
    }

    // ループの長さを維持し、継ぎ目が開始点直前の自然な傾きにつながる
    Audio::SynthWaveGenerator generator;
    Audio::MMLNoteEvent note;
    note.durationSec = 2;
    note.frequency = 110;
    note.waveType = Audio::WaveformType::Custom;
    Audio::MMLSequence sequence{{{{note}, 2}}, 2};
    const auto rendered = generator.GeneratePCM(sequence, 16000);
    const auto* piano = Audio::WavSamples::Find("AcousticGrandPiano");
    assert(piano && piano->loopStart > 0 && piano->loopEnd > piano->loopStart + 1);
    std::vector<std::uint8_t> pianoPcm;
    assert(Audio::WavSamples::Decode(*piano, pianoPcm));
    const double expectedEnd = std::tanh((pianoPcm[piano->loopStart - 1] - 128.0) / 128 * 0.7 * 0.55) * 30000;
    const double expectedStart = std::tanh((pianoPcm[piano->loopStart] - 128.0) / 128 * 0.7 * 0.55) * 30000;
    assert(std::abs(rendered[(piano->loopEnd - 1) * 2] - expectedEnd) < 2);
    assert(std::abs(rendered[piano->loopEnd * 2] - expectedStart) < 2);
    assert(std::abs(rendered[(2 * piano->loopEnd - piano->loopStart) * 2] - expectedStart) < 2);

    // 音程と長さを維持し、PCM音源・合成音・打楽器の末尾を無音にする
    assert(rendered.size() == 32000);
    assert(rendered.front() == 0 && rendered.back() == 0);
    assert(generator.GeneratePCM(sequence, 0).empty());
    assert(generator.GeneratePCM(sequence, -1).empty());
    sequence.tracks[0].events[0].waveType = Audio::WaveformType::Sine;
    auto synthesized = generator.GeneratePCM(sequence, 16000);
    assert(synthesized.front() == 0 && synthesized.back() == 0);
    sequence.maxDurationSec = 0.05;
    sequence.tracks[0].events[0].isDrum = true;
    sequence.tracks[0].events[0].drumMidiNote = 49;
    const auto drum = generator.GeneratePCM(sequence, 16000);
    assert(drum.size() == 800 && drum.back() == 0);

    // ドラムはステレオでも中央に定位し、左右交互のフレーム数を守る
    const auto stereoDrum = generator.GeneratePCM(sequence, 16000, true);
    assert(stereoDrum.size() == drum.size() * 2);
    for (std::size_t i = 0; i < drum.size(); ++i) {
        assert(stereoDrum[i * 2] == drum[i]);
        assert(stereoDrum[i * 2 + 1] == drum[i]);
    }
    // 伴奏を左右へ配置し、左右を足したときに音が打ち消されない
    sequence = {{{{}, 1}, {{note}, 1}}, 1};
    sequence.tracks[1].events[0].frequency = 220;
    const auto stereo = generator.GeneratePCM(sequence, 16000, true);
    double leftEnergy = 0;
    double rightEnergy = 0;
    for (std::size_t i = 0; i < stereo.size(); i += 2) {
        leftEnergy += static_cast<double>(stereo[i]) * stereo[i];
        rightEnergy += static_cast<double>(stereo[i + 1]) * stereo[i + 1];
        assert(static_cast<int>(stereo[i]) * stereo[i + 1] >= 0);
    }
    assert(leftEnergy > rightEnergy * 1.5 && rightEnergy > 0);

    // BGM補正は空トラック数に依存せず、元の音量差を一定倍率で補正する
    const auto normalized = generator.GeneratePCM(sequence, 16000, true, true);
    sequence.tracks.push_back({});
    assert(normalized == generator.GeneratePCM(sequence, 16000, true, true));
    sequence.tracks[1].events[0].volume *= 0.5f;
    const auto quieter = generator.GeneratePCM(sequence, 16000, true, true);
    for (size_t i = 0; i < normalized.size(); ++i) {
        assert(std::abs(static_cast<int>(normalized[i]) - quieter[i]) <= 1);
        assert(std::abs(static_cast<int>(normalized[i])) <= 26214);
    }
    sequence.tracks.clear();
    const auto silence = generator.GeneratePCM(sequence, 16000, true, true);
    for (auto sample : silence) assert(sample == 0);

    // 必要時だけ全曲の生成時間を計測する: FLOPPY_AUDIO_BENCHMARK=1
    char* benchmark = nullptr;
    std::size_t benchmarkLength = 0;
    _dupenv_s(&benchmark, &benchmarkLength, "FLOPPY_AUDIO_BENCHMARK");
    const bool runBenchmark = benchmark && benchmark[0] == '1';
    std::free(benchmark);
    if (runBenchmark) {
        Audio::MMLParser parser;
        for (const auto& [name, original] : scores) {
            const auto started = std::chrono::steady_clock::now();
            const auto music = parser.Parse(std::string(MMLData::GetByName(name)));
            const auto pcm = generator.GeneratePCM(music, 44100, true);
            assert(pcm.size() == static_cast<std::size_t>(music.maxDurationSec * 44100) * 2);
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            std::cout << name << ": " << music.maxDurationSec << "s audio, " << elapsed << "s render\n" << std::flush;
        }
    }
}
