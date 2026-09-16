#pragma once
#include "DifficultyType.h"
#include "PlayerType.h"

/**
 * @brief シーン間で共有されるデータを格納する構造体
 */
class AudioService;
class SteamCoopSession;
class OnlineCoopSession;

struct SceneSharedData {
    AudioService* audio = nullptr;
    DifficultyType difficulty = Easy;
    PlayerType playerType = Homing;
    int playerCount = 1;
    PlayerType secondPlayerType = Homing;
    bool showRankingAfterCredits = false;
#if defined(SPACEYAKUZA_EDITION_Steam)
    SteamCoopSession* coop = nullptr;
#elif defined(SPACEYAKUZA_EDITION_Online)
    OnlineCoopSession* coop = nullptr;
#endif
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    bool onlineGame = false;
#endif
};
