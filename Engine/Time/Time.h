#pragma once

#include <chrono>

class Time {
public:
    /**
     * @brief 時間管理を初期化する
     */
    static void Initialize();

    /**
     * @brief 描画フレームの経過時間を更新する
     */
    static void BeginFrame();

    /**
     * @brief 固定更新を実行できる時間が蓄積されているか取得する
     * @return 固定更新を実行できる場合はtrue
     */
    static bool HasFixedStep();

    /**
     * @brief 固定更新1回分の時間を消費する
     */
    static void ConsumeFixedStep();

    /**
     * @brief 固定更新の余剰時間を破棄する
     */
    static void DiscardExcessFixedTime();

    /**
     * @brief 固定更新と描画の間隔を補間する係数を取得する
     * @return 0.0fから1.0fの補間係数
     */
    static float GetInterpolationAlpha();

    /**
     * @brief ポーズ状態を設定する
     * @param paused trueの場合はゲーム時間を停止する
     */
    static void SetPaused(bool paused);

    // ゲーム更新中に参照する時間
    static float deltaTime;

    // 固定のタイムステップ
    static float fixedDeltaTime;
    static float fixedTime;

    // 実際の描画フレーム間隔
    static float frameDeltaTime;

    // ゲーム開始からの経過時間
    static float time;

    // フレーム間隔を平滑化した時間
    static float smoothDeltaTime;

    // 描画フレーム数
    static unsigned long long frameCount;

    // 固定更新の実行回数
    static unsigned long long fixedFrameCount;

    // 時間の進行倍率
    static float timeScale;

    // ポーズ状態
    static bool isPaused;

    // 1フレームに加算する最大時間
    static constexpr float maximumDeltaTime = 0.25f;

    // 実時間を参照する値
    static float unscaledDeltaTime;
    static float unscaledTime;

private:
    using Clock = std::chrono::steady_clock;

    static Clock::time_point m_previousTime;
    static double m_accumulator;

    static constexpr float smoothingTime = 0.1f;
};
