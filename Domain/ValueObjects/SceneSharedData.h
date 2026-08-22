#pragma once

/**
 * @brief シーン間で共有されるデータを格納する構造体
 */
class AudioService;

struct SceneSharedData {
    AudioService* audio = nullptr;
};
