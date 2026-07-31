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

void Time::Initialize()
{
    m_previousTime = Clock::now();
    m_accumulator = 0.0;

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

void Time::BeginFrame()
{
    const auto currentTime = Clock::now();

    double elapsedSeconds = std::chrono::duration<double>(
        currentTime - m_previousTime
    ).count();

    m_previousTime = currentTime;

    elapsedSeconds = (std::min)(
        elapsedSeconds,
        static_cast<double>(maximumDeltaTime)
    );

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

    const float effectiveTimeScale = isPaused
        ? 0.0f
        : (std::max)(timeScale, 0.0f);
    const double scaledElapsedSeconds =
        elapsedSeconds * static_cast<double>(effectiveTimeScale);

    m_accumulator += scaledElapsedSeconds;
    time += static_cast<float>(scaledElapsedSeconds);
    ++frameCount;

    deltaTime = frameDeltaTime * effectiveTimeScale;
}

bool Time::HasFixedStep()
{
    if (fixedDeltaTime <= 0.0f) {
        return false;
    }

    return m_accumulator >= static_cast<double>(fixedDeltaTime);
}

void Time::ConsumeFixedStep()
{
    if (fixedDeltaTime <= 0.0f) {
        fixedDeltaTime = 1.0f / 60.0f;
    }

    m_accumulator -= static_cast<double>(fixedDeltaTime);
    deltaTime = fixedDeltaTime;
    fixedTime += fixedDeltaTime;
    ++fixedFrameCount;
}

void Time::DiscardExcessFixedTime()
{
    m_accumulator = 0.0;
}

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

void Time::SetPaused(bool paused)
{
    isPaused = paused;
}
