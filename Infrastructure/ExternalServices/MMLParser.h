#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Audio {

/** @brief 音源波形種別 */
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

/** @brief MMLから生成したノートイベント */
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

/** @brief MMLトラック情報 */
struct MMLTrack {
    std::vector<MMLNoteEvent> events;
    double totalDurationSec = 0.0;
};

/** @brief MMLシーケンス全体 */
struct MMLSequence {
    std::vector<MMLTrack> tracks;
    double maxDurationSec = 0.0;
};

/** @brief MML文字列をノートシーケンスへ変換するパーサー */
class MMLParser {
public:
    /**
     * @brief MMLパーサーを生成する
     */
    MMLParser();

    /**
     * @brief MMLパーサーを破棄する
     */
    ~MMLParser() = default;

    /**
     * @brief オクターブ解釈の反転を設定する
     * @param reverse 反転を有効にする場合true
     */
    void SetReverseOctave(bool reverse) { reverseOctave_ = reverse; }

    /**
     * @brief オクターブ解釈の反転状態を取得する
     * @return 反転が有効な場合true
     */
    bool GetReverseOctave() const { return reverseOctave_; }

    /**
     * @brief MML文字列を解析する
     * @param mml 解析するMML文字列
     * @return 解析結果のMMLシーケンス
     */
    MMLSequence Parse(const std::string& mml);

private:
    /**
     * @brief 1トラック分のMMLを解析する
     * @param trackMml 解析するトラック文字列
     * @param tempoMap 時刻とテンポの対応表
     * @param trackIndex 解析対象トラックの番号
     * @return 解析結果のMMLトラック
     */
    MMLTrack ParseTrack(const std::string& trackMml, const std::map<double, double>& tempoMap, size_t trackIndex);

    /**
     * @brief ノート表記を周波数へ変換する
     * @param noteChar ノート文字
     * @param accidentals 半音変化量
     * @param octave オクターブ番号
     * @return ノートの周波数
     */
    static float NoteToFrequency(char noteChar, int accidentals, int octave);

    bool reverseOctave_ = false;
};

} // namespace Audio
