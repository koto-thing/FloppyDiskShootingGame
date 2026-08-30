#pragma once

#include <array>

#include "../../Domain/ValueObjects/PlayerType.h"

class AudioService;
class Renderer;

/**
 * @brief 固定長プールで動作する横スクロールシューティングのゲーム本体
 */
class SideScrollingShooter {
public:
    /** @brief 自機弾の挙動を調整するパラメータ */
    struct PlayerShotParameters {
        int fireIntervalFrames;
        int projectileCount;
        float speed;
        float spreadAngleDegrees;
        float spawnOffsetX;
        float spawnOffsetY;
        float hitRadius;
        int damage;
        float homingStrength;
        bool piercing;
    };

    /** @brief 機体タイプ別の自機弾パラメータ */
    inline static constexpr std::array<PlayerShotParameters, 3> PlayerShotConfigs {{
        // HOMING
        { 10, 2, 0.038f, 5.0f, 0.08f, 0.20f, 0.025f, 1, 0.075f, false },
        // PIERCING
        { 14, 2, 0.052f, 0.0f, 0.10f, 0.20f, 0.032f, 2, 0.000f, true },
        // SPREAD
        { 12, 5, 0.043f, 24.0f, 0.17f, 0.00f, 0.022f, 1, 0.000f, false },
    }};

    /** @brief 全機体共通の通常弾パラメータ */
    inline static constexpr PlayerShotParameters NormalShotConfig {
        6, 1, 0.055f, 0.0f, 0.19f, 0.0f, 0.020f, 1, 0.0f, false
    };

    void Initialize(AudioService* audio, PlayerType playerType);
    void ProcessInput();
    void Tick();
    void Render(Renderer& renderer) const;

private:
    struct Shot {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float hitRadius = 0.022f;
        int damage = 1;
        unsigned int hitEnemyMask = 0;
        PlayerType playerType = Homing;
        bool special = false;
        bool piercing = false;
        bool enemy = false;
        bool active = false;
    };

    struct Enemy {
        float x = 0.0f;
        float y = 0.0f;
        float baseY = 0.0f;
        float phase = 0.0f;
        int hp = 0;
        int maxHp = 0;
        int type = 0;
        int age = 0;
        bool active = false;
    };

    static constexpr int ShotCapacity = 64;
    static constexpr int EnemyCapacity = 12;
    static constexpr float BossStartDistance = 12.0f;
    static constexpr int BossMaxHp = 48;

    void Reset();
    void SpawnEnemy();
    void StartBossBattle();
    void SpawnShot(float x, float y, float vx, float vy, bool enemy);
    void FireNormalShot();
    void FireSpecialShots();
    void UpdateHomingShot(Shot& shot);
    void DamagePlayer();
    void PlayShotSound();
    void PlayHitSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    static void DrawShape(Renderer& renderer,
        float x, float y, float w, float h, const float color[4]);

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    AudioService* m_audio = nullptr;
    PlayerType m_playerType = Homing;
    float m_playerX = -0.72f;
    float m_playerY = 0.0f;
    float m_scroll = 0.0f;
    int m_frame = 0;
    int m_spawnCooldown = 0;
    int m_shotCooldown = 0;
    int m_specialShotCooldown = 0;
    int m_invincible = 0;
    int m_lives = 3;
    int m_score = 0;
    int m_kills = 0;
    int m_bossHp = 0;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_fire = false;
    bool m_gameOver = false;
    bool m_clear = false;
    bool m_bossBattle = false;
};
