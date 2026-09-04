#pragma once

#include "StoryScene.h"

/** @brief 全ステージクリア後のエンディングを遠近スクロールで表示するシーン */
class EndingScene final : public StoryScene {
public:
    /** @brief エンディングシーンを初期化する */
    void Initialize() override;

protected:
    /**
     * @brief エンディング終了後に遷移するシーンを取得する
     * @return クレジットシーン
     */
    SceneType NextScene() const override;
    /**
     * @brief エンディングで表示する文章を取得する
     * @return 表示順に並んだエンディング文章
     */
    std::span<const char* const> CrawlLines() const override;

private:
    inline static constexpr std::array<const char*, 15> EndingLines {{
        "SPACE YAKUZA",
        "",
        "THE YAKUZA ARMADA HAS FALLEN.",
        "",
        "MOMIJI'S STARFIGHTER RETURNS",
        "FROM THE EDGE OF THE GALAXY.",
        "",
        "THE STARS ARE SAFE FOR NOW,",
        "AND HOPE SHINES ONCE MORE",
        "ACROSS THE COSMIC FRONTIER.",
        "",
        "BUT SOMEWHERE IN THE DARKNESS,",
        "A NEW SHADOW IS WAITING....",
        "",
        "THE END"
    }};
};
