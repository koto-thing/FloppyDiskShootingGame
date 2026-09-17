#pragma once

#include <array>
#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include "DifficultyType.h"
#include "PlayerType.h"

/** @brief チャプター開始地点と持ち越すプレイヤー状態 */
struct ResumeCode {
    int stage = 1;
    int part = 1;
    char chapter = '1';
    DifficultyType difficulty = Easy;
    PlayerType player = Homing;
    int power = 0;
    int score = 0;
    int bombs = 3;
    int kills = 0;
    std::array<int, 3> retries {};
    static constexpr size_t MaxLength = 80;

    /** @brief コードを検証して復元情報へ変換する @param text 入力文字列 @param result 成功時の格納先 @return 有効ならtrue */
    static bool Parse(std::string_view text, ResumeCode& result) {
        if (text.size() < 8 || text.size() > MaxLength) return false;
        ResumeCode value;
        size_t pos = 0;
        value.stage = text[pos++] - '0';
        if (value.stage < 1 || value.stage > 5) return false;
        if (value.stage == 5) value.part = text[pos++] - '0';
        if (value.part < 1 || value.part > 2) return false;
        value.chapter = text[pos++];
        if (value.chapter != 'B' && !(value.chapter == 'D' && value.stage == 5 && value.part == 2) &&
            (value.chapter < '1' || value.chapter > '3')) return false;
        const auto difficultyIndex = std::string_view("ENH").find(text[pos++]);
        const auto playerIndex = std::string_view("HPS").find(text[pos++]);
        if (difficultyIndex == std::string_view::npos || playerIndex == std::string_view::npos) return false;
        value.difficulty = static_cast<DifficultyType>(difficultyIndex);
        value.player = static_cast<PlayerType>(playerIndex);

        // 数値全体の消費と上限を確認し、桁あふれや末尾のごみを拒否する
        const auto number = [](std::string_view field, int maximum, int& target) {
            if (field.empty() || field.front() < '0' || field.front() > '9') return false;
            const auto parsed = std::from_chars(field.data(), field.data() + field.size(), target);
            return parsed.ec == std::errc {} && parsed.ptr == field.data() + field.size() && target <= maximum;
        };
        if (pos + 3 >= text.size() || !number(text.substr(pos, 3), 400, value.power)) return false;
        pos += 3;
        const size_t separator = text.find('-', pos);
        if (!number(text.substr(pos, separator == std::string_view::npos ? text.size() - pos : separator - pos),
            999999999, value.score)) return false;
        if (separator != std::string_view::npos) {
            // 拡張部はボム、累計撃破数、各チャプターのリトライ回数の順
            pos = separator + 1;
            int* fields[] = {&value.bombs, &value.kills, &value.retries[0], &value.retries[1], &value.retries[2]};
            for (int i = 0; i < 5; ++i) {
                const size_t end = text.find('-', pos);
                if ((i < 4) == (end == std::string_view::npos)) return false;
                if (!number(text.substr(pos, end == std::string_view::npos ? text.size() - pos : end - pos),
                    i == 0 ? 3 : 1000000, *fields[i])) return false;
                if (end != std::string_view::npos) pos = end + 1;
            }
        }
        result = value;
        return true;
    }

    /** @brief 復元情報を表示用コードへ変換する @return 状態拡張部付きコード */
    std::string Encode() const {
        char text[MaxLength + 1] {};
        const std::string progress = std::to_string(stage) + (stage == 5 ? std::to_string(part) : "");
        std::snprintf(text, sizeof(text), "%s%c%c%c%03d%d-%d-%d-%d-%d-%d", progress.c_str(), chapter,
            "ENH"[difficulty], "HPS"[player], power, score, bombs, kills, retries[0], retries[1], retries[2]);
        return text;
    }
};
