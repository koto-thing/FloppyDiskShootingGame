#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Audio {

// 音源波形種別
enum class WaveformType : uint8_t {
    Square = 0,
    Pulse25 = 1,
    Pulse12_5 = 2,
    Triangle = 3,
    Sawtooth = 4,
    Noise = 5,
    Sine = 6,
    Custom = 7
};

// ノートイベント
struct MMLNoteEvent {
    double startTimeSec = 0.0;
    double durationSec = 0.0;
    float frequency = 0.0f;
    float volume = 1.0f;
    WaveformType waveType = WaveformType::Square;
    uint8_t customWaveId = 0;
    bool isDrum = false;
    uint8_t drumMidiNote = 0;
};

// トラック情報
struct MMLTrack {
    std::vector<MMLNoteEvent> events;
    double totalDurationSec = 0.0;
};

// シーケンス全体
struct MMLSequence {
    std::vector<MMLTrack> tracks;
    double maxDurationSec = 0.0;
};

// MMLパーサー
class MMLParser {
public:
    MMLParser();
    ~MMLParser() = default;

    void SetReverseOctave(bool reverse) { reverseOctave_ = reverse; }
    bool GetReverseOctave() const { return reverseOctave_; }

    MMLSequence Parse(const std::string& mml);

private:
    MMLTrack ParseTrack(const std::string& trackMml, const std::map<double, double>& tempoMap, size_t trackIndex);
    static float NoteToFrequency(char noteChar, int accidentals, int octave);

    bool reverseOctave_ = false;
};

} // namespace Audio
