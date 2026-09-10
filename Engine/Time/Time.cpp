#include "Time.h"

#include <algorithm>
#include <cmath>

float Time::deltaTime = 0.0f;
float Time::fixedDeltaTime = 1.0f / 60.0f;
float Time::fixedTime = 0.0f;
float Time::frameDeltaTime = 0.0f;
float Time::time = 0.0f;
float Time::smoothDeltaTime = 0.0f;
unsigned long long Time::frameCount = 0;
unsigned long long Time::fixedFrameCount = 0;
float Time::timeScale = 1.0f;
bool Time::isPaused = false;
float Time::unscaledDeltaTime = 0.0f;
float Time::unscaledTime = 0.0f;

Time::Clock::time_point Time::m_previousTime;
double Time::m_accumulator = 0.0;

/** @brief 時間管理の状態を初期化する */
void Time::Initialize()
{
    // 実時間計測の基準と固定更新の蓄積時間を初期化する
    m_previousTime = Clock::now();
    m_accumulator = 0.0;

    // ゲーム時間とフレームカウンターを初期化する
    deltaTime = 0.0f;
    fixedTime = 0.0f;
    frameDeltaTime = 0.0f;
    time = 0.0f;
    smoothDeltaTime = 0.0f;
    frameCount = 0;
    fixedFrameCount = 0;
    timeScale = 1.0f;
    isPaused = false;
    unscaledDeltaTime = 0.0f;
    unscaledTime = 0.0f;
}

/** @brief 実時間を計測して可変更新と固定更新へ反映する */
void Time::BeginFrame()
{
    // 前回フレームからの経過時間を計測して上限を適用する
    const auto currentTime = Clock::now();

    double elapsedSeconds = std::chrono::duration<double>(
        currentTime - m_previousTime
    ).count();

    m_previousTime = currentTime;

    elapsedSeconds = (std::min)(
        elapsedSeconds,
        static_cast<double>(maximumDeltaTime)
    );

    // 実時間を更新し、フレーム時間を平滑化する
    frameDeltaTime = static_cast<float>(elapsedSeconds);
    unscaledDeltaTime = frameDeltaTime;
    unscaledTime += unscaledDeltaTime;

    const float smoothingFactor = 1.0f - std::exp(
        -frameDeltaTime / smoothingTime
    );

    if (smoothDeltaTime <= 0.0f) {
        smoothDeltaTime = frameDeltaTime;
    } else {
        smoothDeltaTime +=
            (frameDeltaTime - smoothDeltaTime) * smoothingFactor;
    }

    // ポーズと時間倍率を適用してゲーム時間を蓄積する
    const float effectiveTimeScale = isPaused
        ? 0.0f
        : (std::max)(timeScale, 0.0f);
    const double scaledElapsedSeconds =
        elapsedSeconds * static_cast<double>(effectiveTimeScale);

    m_accumulator += scaledElapsedSeconds;
    time += static_cast<float>(scaledElapsedSeconds);
    ++frameCount;

    // 可変更新で参照する時間を確定する
    deltaTime = frameDeltaTime * effectiveTimeScale;
}

/** @brief 固定更新を実行できる時間が蓄積されているか取得する */
bool Time::HasFixedStep()
{
    if (fixedDeltaTime <= 0.0f) {
        return false;
    }

    return m_accumulator >= static_cast<double>(fixedDeltaTime);
}

/** @brief 固定更新1回分の時間を消費する */
void Time::ConsumeFixedStep()
{
    // 不正な固定更新幅を既定値へ戻す
    if (fixedDeltaTime <= 0.0f) {
        fixedDeltaTime = 1.0f / 60.0f;
    }

    // 固定更新用の時間とカウンターを進める
    m_accumulator -= static_cast<double>(fixedDeltaTime);
    deltaTime = fixedDeltaTime;
    fixedTime += fixedDeltaTime;
    ++fixedFrameCount;
}

/** @brief 固定更新の余剰時間を破棄する */
void Time::DiscardExcessFixedTime()
{
    m_accumulator = 0.0;
}

/** @brief 固定更新と描画の間隔を補間する係数を取得する */
float Time::GetInterpolationAlpha()
{
    if (fixedDeltaTime <= 0.0f) {
        return 0.0f;
    }

    const float alpha = static_cast<float>(
        m_accumulator / static_cast<double>(fixedDeltaTime)
    );

    return (std::clamp)(alpha, 0.0f, 1.0f);
}

/**
 * @brief ポーズ状態を設定する
 * @param paused trueの場合はゲーム時間を停止する
 */
void Time::SetPaused(bool paused)
{
    isPaused = paused;
}
