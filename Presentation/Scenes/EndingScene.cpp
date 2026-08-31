#include "EndingScene.h"

/**
 * @brief エンディング終了後に遷移するシーンを取得する
 * @return ランキングシーン
 */
SceneType EndingScene::NextScene() const {
    return SceneType::Ranking;
}

/**
 * @brief エンディングで表示する文章を取得する
 * @return 表示順に並んだエンディング文章
 */
std::span<const char* const> EndingScene::CrawlLines() const {
    return EndingLines;
}
