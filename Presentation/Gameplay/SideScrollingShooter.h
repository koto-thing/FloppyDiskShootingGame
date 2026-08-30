#pragma once

#include <array>

#include "../../Engine/Graphics/Camera3D.h"

class AudioService;
class Renderer;

/**
 * @brief 固定長プールで動作する横スクロールシューティングのゲーム本体
 */
class SideScrollingShooter {
public:
    void Initialize(AudioService* audio);
    void ProcessInput();
    void Tick();
    void Render(Renderer& renderer) const;

private:
    struct Shot {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        bool enemy = false;
        bool active = false;
    };

    struct Enemy {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float baseX = 0.0f;
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
    static constexpr int ViewTransitionFrames = 90;
    static constexpr float WorldXScale = 7.0f;
    static constexpr float WorldYScale = 4.4f;
    static constexpr float PlayerRailZ = 8.0f;
    static constexpr float SidePlaneZ = 10.0f;
    static constexpr float EnemyRailFarZ = 60.0f;

    enum class ViewMode {
        Side2D,
        Rail3D
    };

    void Reset();
    void TickViewTransition();
    void InitializeRailObjects();
    void TickPlayer();
    void TickEnemies();
    void TickShots();
    void SpawnEnemy();
    void StartBossBattle();
    void SpawnShot(float x, float y, float vx, float vy, bool enemy, float z = -1.0f);
    void DamagePlayer();
    void PlayShotSound();
    void PlayHitSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    static bool Hit3D(float ax, float ay, float az, float ar, float bx, float by, float bz, float br);
    static float SmoothStep(float value);
    static float ToWorldX(float x);
    static float ToWorldY(float y);
    static float FromWorldX(float x);
    static float FromWorldY(float y);
    static float ToRailZFromSideX(float x);
    static float ToSideXFromRailZ(float z);
    float RailBlend() const;
    bool IsRailGameplayActive() const;
    bool IsRailRenderActive() const;
    void ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const;
    void ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const;
    void Render2D(Renderer& renderer) const;
    void Render3D(Renderer& renderer) const;
    void DrawBossHud(Renderer& renderer) const;
    static void DrawShape(Renderer& renderer,
        float x, float y, float w, float h, const float color[4]);
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        float x, float y, float z, float w, float h, float d, const float color[4], float yaw = 0.0f);
    static void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible, float yaw = 0.0f);
    static void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f);
    static void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f);

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
    bool m_viewToggleRequested = false;
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    float m_viewTransitionProgress = 0.0f;
};
