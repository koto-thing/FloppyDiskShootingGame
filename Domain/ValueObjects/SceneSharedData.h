#pragma once
#include "DifficultyType.h"
#include "PlayerType.h"

/**
 * @brief シーン間で共有されるデータを格納する構造体
 */
class AudioService;

struct SceneSharedData {
    AudioService* audio = nullptr;
    DifficultyType difficulty = Easy;
    PlayerType playerType = Homing;
};
