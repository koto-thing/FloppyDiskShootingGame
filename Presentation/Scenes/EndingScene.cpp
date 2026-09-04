#include "EndingScene.h"

/** @brief エンディングシーンを初期化する */
void EndingScene::Initialize() {
    // クレジット終了後にランキングへ進むことを共有する
    getData().showRankingAfterCredits = true;
    StoryScene::Initialize();
}

/**
 * @brief エンディング終了後に遷移するシーンを取得する
 * @return クレジットシーン
 */
SceneType EndingScene::NextScene() const {
    return SceneType::Credit;
}

/**
 * @brief エンディングで表示する文章を取得する
 * @return 表示順に並んだエンディング文章
 */
std::span<const char* const> EndingScene::CrawlLines() const {
    return EndingLines;
}
