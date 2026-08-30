#pragma once

#include <array>
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"

/** @brief ゲーム開始前のストーリーを遠近スクロールで表示するシーン */
class StoryScene : public IScene<SceneType, SceneSharedData> {
public:
    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;

private:
    enum class Phase {
        Scrolling,
        Fading,
        Waiting
    };

    /** @brief 星空の背景を描画する */
    void RenderStars(Renderer& renderer) const;
    /** @brief 遠近感を付けたストーリー本文を描画する */
    void RenderCrawl(Renderer& renderer) const;

    // 短縮版
    inline static constexpr std::array<const char*, 21> StoryLines {{
        "SPACE YAKUZA",
        "",

        /** @brief 宇宙秩序は崩壊し、銀河は力と暴力が支配する無法の荒野と化した */
        "THE COSMIC ORDER HAS COLLAPSED.",
        "THE GALAXY IS NOW A LAWLESS WASTELAND",
        "RULED BY POWER AND VIOLENCE.",
        "",

        /**
         * @brief TAYAMA会長率いる血も涙もない巨大極道組織が無法の宙域を支配している
         * @details LUMI、RYOTA、BOTAMOCHI、KOTOが率いるヤクザ艦隊は星々を粉砕し、宇宙を恐怖に陥れている
         */
        "CHAIRMAN TAYAMA AND HIS BLOODTHIRSTY",
        "YAKUZA SYNDICATE RULE THE FRONTIER.",
        "BOSSES LUMI, RYOTA, BOTAMOCHI, AND KOTO",
        "LEAD A FLEET THAT CRUSHES EVERY PLANET",
        "AND PLUNGES THE STARS INTO TERROR.",
        "",

        /**
         * @brief 銀河警察が恐怖と腐敗に屈する中、はぐれ者の宇宙警察MOMIJIだけが抵抗を続けていた
         */
        "THE GALACTIC POLICE HAVE SURRENDERED",
        "TO FEAR AND CORRUPTION.",
        "ONLY ONE ROGUE SPACE COP STILL RESISTS:",
        "MOMIJI.",
        "",

        /** @brief ヤクザ艦隊に裁きを下すため、MOMIJIは愛機でたった一人死の星海へ飛び立つ */
        "TO BRING JUSTICE TO THE YAKUZA ARMADA,",
        "MOMIJI BOARDS A TRUSTY STARFIGHTER",
        "AND FLIES ALONE INTO A DEADLY SEA",
        "OF STARS...."
    }};

    std::unique_ptr<Button> m_skipButton;
    Phase m_phase = Phase::Scrolling;
    float m_scrollTime = 0.0f;
    float m_backgroundTime = 0.0f;
    float m_phaseTime = 0.0f;
};

// 悔しいので短縮前のストーリを以下に残しておきます
// 
// かつての平和な宇宙秩序は完全に崩壊し、
// 銀河は力と暴力が支配する無秩序な荒野と化していた。
//
// 無法の宙域を牛耳るのは、血も涙もない巨大極道組織である。
// 絶対的な権力を持つ元締め、TAYAMA会長の支配の下、
// 鉄の掟を敷くLUMI組長、恐れ知らずの若頭RYOTA、
// 冷酷なる本部長BOTAMOCHI、そして歴戦の舎弟頭KOTOという無慈悲な指揮系統が、
// 逆らう者すべてを粉砕していた。
// 彼らの強大な「ヤクザ艦隊」は罪なき星々を次々と制圧し、宇宙全体を恐怖のどん底に陥れている。
//
// 警察組織すらも極道の力に屈し、腐敗していく中、決して暗黒に屈しない一人の警官がいた。
// はぐれ者の宇宙警察、MOMIJIである。
//
// 宇宙を蹂躙するヤクザ軍団に裁きを下すため、MOMIJIは愛機に乗り込み、
// たった一人で巨大な極道艦隊が待ち受ける死の星海へと飛び立ったのである……。
