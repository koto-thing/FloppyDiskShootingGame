#include "Stage4Module.h"

#include "../../SideScrollingShooterEnemies.h"
#include "../Common/StageDefinition.h"

/** @brief Stage 4の敵出現とボス弾幕を定義する */
class SideScrollingShooter::Stage4Module::StageDefinitionImpl final : public SideScrollingShooter::Stage {
public:
    /**
     * @brief ステージ番号を取得する
     * @return Stage 4を表す番号
     */
    int StageIndex() const override {
        return 4;
    }

    /**
     * @brief Stage 4の経過フレームから通常敵の出現を選択する
     * @param frame Stage 4開始からの経過フレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {4, 20, 48, 1.10f, -0.82f, -0.68f, 60.0f},
            {5, 90, 210, 1.14f, -0.28f, 0.86f, 50.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {4, 10, 46, 1.12f, 0.28f, 0.18f, 60.0f},
            {3, 60, 125, 1.10f, 0.82f, -0.88f, 34.0f},
            {1, 130, 190, 1.16f, -0.40f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {5, 10, 110, 1.14f, -0.82f, 0.86f, 50.0f},
            {4, 50, 105, 1.12f, 0.82f, -0.18f, 60.0f},
            {3, 120, 135, 1.10f, 0.28f, -0.88f, 40.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }

    /**
     * @brief Stage 4ボスの一斉射撃数を取得する
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 7;
    }

    /**
     * @brief Stage 4ボスの指定番号の弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.022f, -0.027f},
            {-0.12f, 0.0f, -0.025f, -0.018f},
            {-0.12f, 0.0f, -0.027f, -0.009f},
            {-0.12f, 0.0f, -0.029f, 0.0f},
            {-0.12f, 0.0f, -0.027f, 0.009f},
            {-0.12f, 0.0f, -0.025f, 0.018f},
            {-0.12f, 0.0f, -0.022f, 0.027f}
        };
        constexpr BossBullet RailPattern[] = {
            {0.0f, 0.0f, -0.018f, -0.028f},
            {0.0f, 0.0f, -0.012f, -0.018f},
            {0.0f, 0.0f, -0.006f, -0.009f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.006f, 0.009f},
            {0.0f, 0.0f, 0.012f, 0.018f},
            {0.0f, 0.0f, 0.018f, 0.028f}
        };
        return (railMode ? RailPattern : SidePattern)[index % 7];
    }
};

const SideScrollingShooter::Stage& SideScrollingShooter::Stage4Module::Definition() {
    static const StageDefinitionImpl definition;
    return definition;
}

bool SideScrollingShooter::Stage4Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float yaw) {
    (void)shooter;
    (void)renderer;
    (void)camera;
    (void)enemy;
    (void)yaw;
    // 現行ゲームはStage 4でも共通大型戦闘機モデルを使用する
    return false;
}
