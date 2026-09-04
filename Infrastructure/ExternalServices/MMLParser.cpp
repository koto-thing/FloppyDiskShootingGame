#include "MMLParser.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>

namespace Audio {

MMLParser::MMLParser() = default;

// ループ展開
static std::string ExpandMMLLoops(const std::string& input) {
    std::string result = input;
    bool changed = true;
    int maxDepth = 500;
    while (changed && maxDepth-- > 0) {
        changed = false;
        size_t closePos = result.find(']');
        if (closePos != std::string::npos) {
            size_t openPos = result.rfind('[', closePos);
            if (openPos != std::string::npos) {
                std::string body = result.substr(openPos + 1, closePos - openPos - 1);
                size_t numIdx = closePos + 1;
                int count = 1;
                if (numIdx < result.length() && std::isdigit(static_cast<unsigned char>(result[numIdx]))) {
                    count = 0;
                    while (numIdx < result.length() && std::isdigit(static_cast<unsigned char>(result[numIdx]))) {
                        count = count * 10 + (result[numIdx] - '0');
                        numIdx++;
                    }
                }
                std::string expanded = "";
                expanded.reserve((body.length() + 1) * count);
                for (int k = 0; k < count; ++k) {
                    expanded += body + " ";
                }
                result.replace(openPos, numIdx - openPos, expanded);
                changed = true;
            }
        }
    }
    return result;
}

// 拍数を秒数に変換
static double ConvertBeatsToSeconds(double bStart, double numBeats, const std::map<double, double>& tempoMap) {
    if (numBeats <= 0.0 || tempoMap.empty()) return 0.0;
    double bEnd = bStart + numBeats;
    double totalSec = 0.0;

    auto it = tempoMap.upper_bound(bStart);
    if (it != tempoMap.begin()) {
        --it;
    }

    double curB = bStart;
    while (curB < bEnd) {
        double curBPM = it->second;
        if (curBPM <= 0.0) curBPM = 120.0;

        auto nextIt = std::next(it);
        double nextChangeB = (nextIt != tempoMap.end()) ? nextIt->first : bEnd;

        double segmentEndB = std::min(bEnd, nextChangeB);
        double segmentBeats = segmentEndB - curB;

        if (segmentBeats > 0.0) {
            totalSec += segmentBeats * (60.0 / curBPM);
            curB = segmentEndB;
        }

        if (nextIt != tempoMap.end() && curB >= nextChangeB) {
            it = nextIt;
        } else {
            break;
        }
    }

    return totalSec;
}

MMLSequence MMLParser::Parse(const std::string& mml) {
    std::stringstream rawStream(mml);
    std::string line;
    std::string cleanedMml;

    // コメント除去
    while (std::getline(rawStream, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        cleanedMml += line + "\n";
    }

    // ヘッダー・フッター除去
    size_t mmlPrefixPos = cleanedMml.find("MML@");
    if (mmlPrefixPos != std::string::npos) {
        cleanedMml = cleanedMml.substr(mmlPrefixPos + 4);
    }

    size_t endSemicolonPos = cleanedMml.find(';');
    if (endSemicolonPos != std::string::npos) {
        cleanedMml = cleanedMml.substr(0, endSemicolonPos);
    }

    // トラック分割
    std::vector<std::string> trackSegments;
    std::stringstream ss(cleanedMml);
    std::string trackSegment;
    while (std::getline(ss, trackSegment, ',')) {
        bool hasContent = std::any_of(trackSegment.begin(), trackSegment.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        });
        if (hasContent) {
            trackSegments.push_back(ExpandMMLLoops(trackSegment));
        }
    }

    if (trackSegments.empty()) {
        return {};
    }

    // テンポマップ構築
    std::map<double, double> tempoMap;
    tempoMap[0.0] = 120.0;

    auto parseNumberInString = [](const std::string& str, size_t& idx) -> int {
        size_t len = str.length();
        while (idx < len && std::isspace(static_cast<unsigned char>(str[idx]))) {
            idx++;
        }
        int val = 0;
        bool hasDigits = false;
        while (idx < len && std::isdigit(static_cast<unsigned char>(str[idx]))) {
            val = val * 10 + (str[idx] - '0');
            hasDigits = true;
            idx++;
        }
        return hasDigits ? val : -1;
    };

    auto parseLengthInBeats = [&](const std::string& str, size_t& idx, double defaultBeats) -> double {
        int val = parseNumberInString(str, idx);
        if (val <= 0) return defaultBeats;

        double beats = 4.0 / static_cast<double>(val);
        double dotMultiplier = 1.0;
        double currentDotAdd = 0.5;
        size_t len = str.length();
        while (idx < len) {
            if (std::isspace(static_cast<unsigned char>(str[idx]))) {
                idx++;
            } else if (str[idx] == '.') {
                dotMultiplier += currentDotAdd;
                currentDotAdd *= 0.5;
                idx++;
            } else {
                break;
            }
        }
        return beats * dotMultiplier;
    };

    for (const auto& segment : trackSegments) {
        double currentBeat = 0.0;
        double defaultLengthBeats = 0.25;
        size_t i = 0;
        size_t len = segment.length();

        while (i < len) {
            char ch = segment[i];
            if (std::isspace(static_cast<unsigned char>(ch))) {
                i++;
                continue;
            }
            else if (ch == '&' || ch == '^') {
                i++;
                double tieBeats = parseLengthInBeats(segment, i, defaultLengthBeats);
                currentBeat += tieBeats;
                continue;
            }
            char lowerCh = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

            if ((lowerCh >= 'a' && lowerCh <= 'g') || lowerCh == 'n' || lowerCh == 'r') {
                i++;
                if (lowerCh == 'n') {
                    parseNumberInString(segment, i);
                } else if (lowerCh >= 'a' && lowerCh <= 'g') {
                    while (i < len) {
                        if (std::isspace(static_cast<unsigned char>(segment[i]))) {
                            i++;
                        } else if (segment[i] == '+' || segment[i] == '#' || segment[i] == '-') {
                            i++;
                        } else {
                            break;
                        }
                    }
                }

                double durationInBeats = parseLengthInBeats(segment, i, defaultLengthBeats);
                currentBeat += durationInBeats;
            }
            else if (lowerCh == 't') {
                i++;
                int val = parseNumberInString(segment, i);
                if (val > 0) {
                    tempoMap[currentBeat] = static_cast<double>(val);
                }
            }
            else if (lowerCh == 'l') {
                i++;
                defaultLengthBeats = parseLengthInBeats(segment, i, defaultLengthBeats);
                while (i < len && segment[i] == '&') {
                    i++;
                    defaultLengthBeats += parseLengthInBeats(segment, i, 0.0);
                }
            }
            else if (ch == '@') {
                i++;
                if (i < len && (segment[i] == 'd' || segment[i] == 'D')) {
                    i++;
                } else if (i < len && (segment[i] == 'v' || segment[i] == 'V')) {
                    i++;
                    parseNumberInString(segment, i);
                } else if (i < len && (segment[i] == 'w' || segment[i] == 'W')) {
                    i++;
                    while (i < len && !std::isspace(static_cast<unsigned char>(segment[i])) &&
                           segment[i] != ',' && segment[i] != ';' && !std::isdigit(static_cast<unsigned char>(segment[i]))) {
                        i++;
                    }
                    parseNumberInString(segment, i);
                } else {
                    parseNumberInString(segment, i);
                }
            }
            else if (lowerCh == 'u' || lowerCh == '~') {
                i++;
                parseNumberInString(segment, i);
            }
            else if (ch == '_') {
                i++;
                while (i < len && (segment[i] == '-' || segment[i] == '+' || std::isdigit(static_cast<unsigned char>(segment[i])) || segment[i] == '.')) {
                    i++;
                }
                continue;
            }

            else if (lowerCh == 'o' || lowerCh == 'v') {
                i++;
                parseNumberInString(segment, i);
            }
            else {
                i++;
            }
        }
    }

    // ノートパース
    MMLSequence sequence;
    size_t trackIdx = 0;

    for (const auto& segment : trackSegments) {
        MMLTrack track = ParseTrack(segment, tempoMap, trackIdx++);
        if (!track.events.empty()) {
            sequence.tracks.push_back(track);
            if (track.totalDurationSec > sequence.maxDurationSec) {
                sequence.maxDurationSec = track.totalDurationSec;
            }
        }
    }

    return sequence;
}

MMLTrack MMLParser::ParseTrack(const std::string& rawTrackMml, const std::map<double, double>& tempoMap, size_t trackIndex) {
    std::string trackMml = ExpandMMLLoops(rawTrackMml);
    MMLTrack track;
    
    int octave = 4;
    double defaultLengthBeats = 0.25;
    int volumeInt = 15;
    int fineVol = 127;
    int velocity = 127;
    bool isDrumKit = false;

    // トラック文字列全体に @d が含まれているか（大小問わず）初期判定
    for (size_t k = 0; k + 1 < trackMml.length(); ++k) {
        if (trackMml[k] == '@' && (trackMml[k + 1] == 'd' || trackMml[k + 1] == 'D')) {
            isDrumKit = true;
            break;
        }
    }

    WaveformType waveType = isDrumKit ? WaveformType::Custom : WaveformType::Square;
    if (!isDrumKit) {
        if (trackIndex == 1) {
            waveType = WaveformType::Pulse25;
        } else if (trackIndex == 2) {
            waveType = WaveformType::Triangle;
        } else if (trackIndex >= 3) {
            waveType = WaveformType::Pulse12_5;
        }
    }

    uint8_t customWaveId = 0;
    double currentBeat = 0.0;
    bool isTie = false;

    size_t i = 0;
    size_t len = trackMml.length();

    auto parseNumber = [&](size_t& idx) -> int {
        while (idx < len && std::isspace(static_cast<unsigned char>(trackMml[idx]))) {
            idx++;
        }
        int val = 0;
        bool hasDigits = false;
        while (idx < len && std::isdigit(static_cast<unsigned char>(trackMml[idx]))) {
            val = val * 10 + (trackMml[idx] - '0');
            hasDigits = true;
            idx++;
        }
        return hasDigits ? val : -1;
    };

    auto parseLengthInBeats = [&](size_t& idx, double defaultBeats) -> double {
        int val = parseNumber(idx);
        if (val <= 0) return defaultBeats;

        double beats = 4.0 / static_cast<double>(val);
        double dotMultiplier = 1.0;
        double currentDotAdd = 0.5;
        while (idx < len) {
            if (std::isspace(static_cast<unsigned char>(trackMml[idx]))) {
                idx++;
            } else if (trackMml[idx] == '.') {
                dotMultiplier += currentDotAdd;
                currentDotAdd *= 0.5;
                idx++;
            } else {
                break;
            }
        }
        return beats * dotMultiplier;
    };

    while (i < len) {
        char ch = trackMml[i];

        if (std::isspace(static_cast<unsigned char>(ch))) {
            i++;
            continue;
        }

        char lowerCh = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        if (ch == '\'') {
            i++;
            continue;
        }

        if (lowerCh == '&' || lowerCh == '^') {
            i++;
            bool hasExplicitVal = false;
            size_t saveI = i;
            while (saveI < len && std::isspace(static_cast<unsigned char>(trackMml[saveI]))) saveI++;
            if (saveI < len && std::isdigit(static_cast<unsigned char>(trackMml[saveI]))) {
                hasExplicitVal = true;
            }

            double tieBeats = hasExplicitVal ? parseLengthInBeats(i, 0.0) : defaultLengthBeats;
            double durationSec = ConvertBeatsToSeconds(currentBeat, tieBeats, tempoMap);

            bool extendedNote = false;
            if (!track.events.empty()) {
                auto& lastEv = track.events.back();
                double lastEndSec = lastEv.startTimeSec + lastEv.durationSec;
                double curStartSec = ConvertBeatsToSeconds(0.0, currentBeat, tempoMap);
                if (std::abs(lastEndSec - curStartSec) < 0.05) {
                    lastEv.durationSec += durationSec;
                    extendedNote = true;
                }
            }

            if (!extendedNote && !hasExplicitVal) {
                isTie = true;
            }

            currentBeat += tieBeats;
            continue;
        }

        if ((lowerCh >= 'a' && lowerCh <= 'g') || lowerCh == 'n') {
            i++;
            float freq = 0.0f;
            int midiNote = 60;

            if (lowerCh == 'n') {
                int parsed = parseNumber(i);
                if (parsed >= 0) midiNote = parsed;
                freq = static_cast<float>(440.0 * std::pow(2.0, (midiNote - 69) / 12.0));
            } else {
                int accidentals = 0;
                while (i < len) {
                    if (std::isspace(static_cast<unsigned char>(trackMml[i]))) {
                        i++;
                    } else if (trackMml[i] == '+' || trackMml[i] == '#') {
                        accidentals++;
                        i++;
                    } else if (trackMml[i] == '-') {
                        accidentals--;
                        i++;
                    } else {
                        break;
                    }
                }
                int baseOffset = 0;
                switch (lowerCh) {
                    case 'c': baseOffset = 0; break;
                    case 'd': baseOffset = 2; break;
                    case 'e': baseOffset = 4; break;
                    case 'f': baseOffset = 5; break;
                    case 'g': baseOffset = 7; break;
                    case 'a': baseOffset = 9; break;
                    case 'b': baseOffset = 11; break;
                    default: baseOffset = 0; break;
                }
                int semitonesFromA4 = (baseOffset + accidentals) - 9 + (octave - 4) * 12;
                midiNote = 69 + semitonesFromA4;
                freq = static_cast<float>(440.0 * std::pow(2.0, static_cast<double>(semitonesFromA4) / 12.0));
            }

            double durationInBeats = parseLengthInBeats(i, defaultLengthBeats);
            double startTimeSec = ConvertBeatsToSeconds(0.0, currentBeat, tempoMap);
            double durationSec = ConvertBeatsToSeconds(currentBeat, durationInBeats, tempoMap);

            if (isTie && !track.events.empty() && std::abs(track.events.back().frequency - freq) < 0.1f) {
                track.events.back().durationSec += durationSec;
            } else {
                float vBase = (volumeInt > 15) ? (static_cast<float>(volumeInt) / 127.0f) : (static_cast<float>(volumeInt) / 15.0f);
                float vol = vBase * (static_cast<float>(fineVol) / 127.0f) * (static_cast<float>(velocity) / 127.0f);
                vol = std::clamp(vol, 0.0f, 1.0f);

                MMLNoteEvent note;
                note.startTimeSec = startTimeSec;
                note.durationSec = durationSec;
                note.frequency = freq;
                note.volume = vol;
                note.waveType = waveType;
                note.customWaveId = customWaveId;
                note.isDrum = isDrumKit;
                note.drumMidiNote = static_cast<uint8_t>(std::clamp(midiNote, 0, 127));

                track.events.push_back(note);
            }

            currentBeat += durationInBeats;
            isTie = false;
        }
        else if (lowerCh == 'r') {
            i++;
            double durationInBeats = parseLengthInBeats(i, defaultLengthBeats);
            currentBeat += durationInBeats;
            isTie = false;
        }
        else if (lowerCh == 't') {
            i++;
            parseNumber(i);
        }
        else if (lowerCh == 'o') {
            i++;
            int val = parseNumber(i);
            if (val >= 0) {
                octave = std::clamp(val, 1, 8);
            }
        }
        else if (ch == '<') {
            i++;
            int delta = reverseOctave_ ? 1 : -1;
            octave = std::clamp(octave + delta, 1, 8);
        }
        else if (ch == '>') {
            i++;
            int delta = reverseOctave_ ? -1 : 1;
            octave = std::clamp(octave + delta, 1, 8);
        }
        else if (lowerCh == 'l') {
            i++;
            defaultLengthBeats = parseLengthInBeats(i, defaultLengthBeats);
            while (i < len && trackMml[i] == '&') {
                i++;
                defaultLengthBeats += parseLengthInBeats(i, 0.0);
            }
        }
        else if (lowerCh == 'v') {
            i++;
            int val = parseNumber(i);
            if (val >= 0) {
                volumeInt = std::clamp(val, 0, 15);
            }
        }
        else if (ch == '@') {
            i++;
            if (i < len && (trackMml[i] == 'd' || trackMml[i] == 'D')) {
                i++;
                isDrumKit = true;
                waveType = WaveformType::Custom;
                continue;
            } else if (i < len && (trackMml[i] == 'v' || trackMml[i] == 'V')) {
                i++;
                int v = parseNumber(i);
                if (v >= 0) fineVol = std::clamp(v, 0, 127);
                continue;
            } else if (i < len && (trackMml[i] == 'w' || trackMml[i] == 'W')) {
                i++;
                isDrumKit = false;
                std::string wName;
                while (i < len && (std::isalnum(static_cast<unsigned char>(trackMml[i])) || trackMml[i] == '.')) {
                    wName += static_cast<char>(std::tolower(static_cast<unsigned char>(trackMml[i])));
                    i++;
                }
                if (wName.find("12.5") != std::string::npos || wName.find("12") != std::string::npos) {
                    waveType = WaveformType::Pulse12_5;
                } else if (wName.find("25") != std::string::npos) {
                    waveType = WaveformType::Pulse25;
                } else if (wName.find("tri") != std::string::npos) {
                    waveType = WaveformType::Triangle;
                } else {
                    waveType = WaveformType::Square;
                }
                continue;
            } else {
                int val = parseNumber(i);
                if (val >= 0) {
                    customWaveId = static_cast<uint8_t>(val);
                    waveType = WaveformType::Custom;
                    isDrumKit = false;
                }
                continue;
            }
        }
        else if (lowerCh == 'u') {
            i++;
            int vel = parseNumber(i);
            if (vel >= 0) velocity = std::clamp(vel, 0, 127);
            continue;
        }
        else if (lowerCh == '~') {
            i++;
            parseNumber(i);
            continue;
        }
        else if (ch == '_') {
            i++;
            while (i < len && (trackMml[i] == '-' || trackMml[i] == '+' || std::isdigit(static_cast<unsigned char>(trackMml[i])) || trackMml[i] == '.')) {
                i++;
            }
            continue;
        }

        else {
            i++;
        }
    }

    track.totalDurationSec = ConvertBeatsToSeconds(0.0, currentBeat, tempoMap);
    return track;
}

float MMLParser::NoteToFrequency(char noteChar, int accidentals, int octave) {
    int baseOffset = 0;
    switch (noteChar) {
        case 'c': baseOffset = 0; break;
        case 'd': baseOffset = 2; break;
        case 'e': baseOffset = 4; break;
        case 'f': baseOffset = 5; break;
        case 'g': baseOffset = 7; break;
        case 'a': baseOffset = 9; break;
        case 'b': baseOffset = 11; break;
        default: baseOffset = 0; break;
    }

    int semitonesFromA4 = (baseOffset + accidentals) - 9 + (octave - 4) * 12;
    return static_cast<float>(440.0 * std::pow(2.0, static_cast<double>(semitonesFromA4) / 12.0));
}

} // namespace Audio
