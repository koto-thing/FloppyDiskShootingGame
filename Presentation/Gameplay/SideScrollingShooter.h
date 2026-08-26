#pragma once

#include <array>

class AudioService;
class D3D12RenderingService;

/**
 * @brief 固定長プールで動作する横スクロールシューティングのゲーム本体
 */
class SideScrollingShooter {
public:
    void Initialize(AudioService* audio);
    void ProcessInput();
    void Tick();
    void Render(D3D12RenderingService& renderer) const;

private:
    struct Shot {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
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
    void DamagePlayer();
    void PlayShotSound();
    void PlayHitSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    static void DrawShape(D3D12RenderingService& renderer, int& index,
        float x, float y, float w, float h, const float color[4]);

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    AudioService* m_audio = nullptr;
    float m_playerX = -0.72f;
    float m_playerY = 0.0f;
    float m_scroll = 0.0f;
    int m_frame = 0;
    int m_spawnCooldown = 0;
    int m_shotCooldown = 0;
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
