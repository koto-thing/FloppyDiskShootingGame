#pragma once

#include <algorithm>

#include "../../SideScrollingShooterEnemies.h"

/**
 * @brief ステージごとの敵出現と弾幕を定義する基底クラス
 */
class SideScrollingShooter::Stage {
public:
    /** @brief 敵の挙動・モデル・弾幕を組み合わせた構成種別 */
    enum EnemyType {
        BasicEnemy = 0,
        HeavyEnemy = 1,
        BossEnemy = 2,
        StraightShooterEnemy = 3,
        ArmoredEnemy = 4,
        CircleShooterEnemy = 5,
        SquareShooterEnemy = 6,
        DiveRusherEnemy = 7,
        MissileShooterEnemy = 8,
        LinkedLaserEnemy = 9,
        WallSecurityDroneEnemy = 10
    };

    struct BossBullet {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
    };

    struct EnemySpawnRule {
        int enemyType = BasicEnemy;
        int firstFrame = 0;
        int interval = 0;
        float sideX = 1.10f;
        float railX = 0.0f;
        float y = 0.0f;
        float railZ = SideScrollingShooter::EnemyRailFarZ;
    };

    struct Chapter {
        const EnemySpawnRule* spawnRules = nullptr;
        int spawnRuleCount = 0;
    };

    virtual ~Stage() = default;
    virtual int StageIndex() const = 0;
    virtual float BossStartDistance() const {
        return SideScrollingShooter::BossStartDistance;
    }
    virtual int BossMaxHp() const {
        return BossEnemyBehaviorInstance().MaxHpForStage(StageIndex());
    }
    /**
     * @brief 撃破時の飛散部品に重力を適用するか取得する
     * @return 重力を適用するステージの場合true、適用しない場合false
     */
    virtual bool HasDebrisGravity() const {
        return false;
    }
    /**
     * @brief 1チャプターの長さを取得する
     * @return 1チャプターのフレーム数
     */
    virtual int ChapterFrameLength() const {
        return SideScrollingShooter::ChapterLengthFrames;
    }
    /**
     * @brief 指定チャプターの終了フレームを取得する
     * @param chapterNumber チャプター番号
     * @return 指定チャプターの終了フレーム
     */
    virtual int ChapterEndFrame(int chapterNumber) const {
        return chapterNumber * ChapterFrameLength();
    }
    /**
     * @brief 指定フレームで出現させる敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    virtual bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const = 0;
    virtual void ConfigureEnemy(SideScrollingShooter& shooter, Enemy& enemy,
        int enemyType, int frame, int kills, bool railMode) const {
        EnemyBehaviorForType(enemyType).ConfigureSpawn(
            shooter, enemy, frame, kills, railMode, StageIndex());
    }
    virtual void ConfigureBoss(Enemy& boss, bool railMode) const {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
    }
    /**
     * @brief ボスの3Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    virtual void ConfigureBossRailAnchor(Enemy& boss) const {
        boss.x = 0.0f;
        boss.y = 0.0f;
        boss.z = 48.0f;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
    }
    /**
     * @brief ボスの2Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    virtual void ConfigureBossSideAnchor(Enemy& boss) const {
        boss.x = 1.80f;
        boss.y = 0.0f;
        boss.z = ToRailZFromSideX(boss.x);
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
    }
    /**
     * @brief ボス部位HPを設定する
     * @param boss 部位HPを設定するボス
     * @return なし
     */
    virtual void ConfigureBossPartHp(Enemy& boss) const {
        boss.bossPartHp = { 120, 180, 180, 150, 150 };
    }
    /**
     * @brief 部位破壊時にボス本体へ与えるダメージを取得する
     * @param part 破壊された部位
     * @return 本体へ与えるダメージ
     */
    virtual int BossPartBreakDamage(BossPart part) const {
        (void)part;
        return 120;
    }
    /**
     * @brief ボスの移動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    virtual void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const {
        BossEnemyBehaviorInstance().Tick(shooter, boss);
    }
    /**
     * @brief ボスが弾幕を止める特殊攻撃中か取得する
     * @param shooter ゲーム本体
     * @param boss 判定するボス
     * @return 特殊攻撃中の場合true、通常攻撃中の場合false
     */
    virtual bool IsBossSpecialAttackActive(const SideScrollingShooter& shooter, const Enemy& boss) const {
        (void)shooter;
        (void)boss;
        return false;
    }
    /**
     * @brief ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    virtual int BossAttackInterval(BossPhase phase) const {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 84;
    }
    /**
     * @brief 本体HPからボス攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ボス攻撃フェーズ
     */
    virtual int BossPhaseForHp(int hp, int maxHp) const {
        return SideScrollingShooter::BossPhaseForHp(hp, maxHp);
    }
    virtual int BossBulletCount(bool railMode) const = 0;
    virtual BossBullet GetBossBullet(int index, bool railMode) const = 0;
    /**
     * @brief 指定部位・フェーズの弾数を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param railMode レール表示中か
    * @return 発射する弾数
    */
    virtual int BossPartBulletCount(BossPart part, BossPhase phase, bool railMode) const {
        if (part == BossNose) return BossBulletCount(railMode);
        if (part == BossLeftWing || part == BossRightWing) {
            return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 3 : 2;
        }
        return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
    }
    /**
     * @brief 指定部位・フェーズの弾幕内の弾を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    virtual BossBullet GetBossPartBullet(BossPart part, BossPhase phase, int index, bool railMode) const {
        const int baseCount = BossBulletCount(railMode);
        const int patternIndex = part == BossNose ? index :
            (part == BossLeftWing ? 0 : (part == BossRightWing ? baseCount - 1 : baseCount / 2));
        BossBullet bullet = GetBossBullet(patternIndex, railMode);
        if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
            bullet.vx *= 1.35f;
            bullet.vy += (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }

protected:
    template<int RuleCount>
    static constexpr Chapter MakeChapter(const EnemySpawnRule (&rules)[RuleCount]) {
        return { rules, RuleCount };
    }

    bool TrySelectByChapters(const Chapter* chapters, int chapterCount, int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const {
        const int chapterFrameLength = ChapterFrameLength();
        for (int chapterIndex = 0; chapterIndex < chapterCount; ++chapterIndex) {
            const int chapterFirstFrame = chapterIndex * chapterFrameLength;
            if (frame < chapterFirstFrame || frame >= chapterFirstFrame + chapterFrameLength) continue;
            chapterNumber = chapterIndex + 1;
            const Chapter& chapter = chapters[chapterIndex];
            return TrySelectByRules(chapter.spawnRules, chapter.spawnRuleCount, spawnIndex,
                frame - chapterFirstFrame, spawn);
        }
        return false;
    }

    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int spawnIndex,
        int frame, EnemySpawnRule& spawn) const {
        int matchedIndex = 0;
        for (int i = 0; i < ruleCount; ++i) {
            const EnemySpawnRule& rule = rules[i];
            if (frame < rule.firstFrame) {
                continue;
            }
            if (frame == rule.firstFrame ||
                (rule.interval > 0 && (frame - rule.firstFrame) % rule.interval == 0)) {
                if (matchedIndex++ != spawnIndex) {
                    continue;
                }
                spawn = rule;
                return true;
            }
        }
        return false;
    }

    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int spawnIndex,
        int frame, int kills, EnemySpawnRule& spawn) const {
        (void)kills;
        return TrySelectByRules(rules, ruleCount, spawnIndex, frame, spawn);
    }
};
