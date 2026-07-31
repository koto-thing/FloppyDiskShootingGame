#include "../Engine/Time/Time.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void Require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void RequireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::fabs(actual - expected) > tolerance) {
        throw std::runtime_error(message);
    }
}

void WaitForElapsedTime()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
}

void InitializeResetsState()
{
    Time::time = 42.0f;
    Time::frameCount = 12;
    Time::fixedFrameCount = 7;
    Time::isPaused = true;

    Time::Initialize();

    RequireNear(Time::time, 0.0f, 0.0001f, "Initialize must reset time");
    RequireNear(Time::fixedTime, 0.0f, 0.0001f, "Initialize must reset fixedTime");
    RequireNear(Time::frameDeltaTime, 0.0f, 0.0001f, "Initialize must reset frameDeltaTime");
    Require(Time::frameCount == 0, "Initialize must reset frameCount");
    Require(Time::fixedFrameCount == 0, "Initialize must reset fixedFrameCount");
    Require(!Time::isPaused, "Initialize must clear the paused state");
}

void BeginFrameUpdatesScaledAndUnscaledTime()
{
    Time::Initialize();
    Time::timeScale = 0.5f;

    WaitForElapsedTime();
    Time::BeginFrame();

    Require(Time::frameCount == 1, "BeginFrame must increment frameCount");
    Require(Time::frameDeltaTime > 0.0f, "BeginFrame must measure elapsed time");
    Require(Time::frameDeltaTime <= Time::maximumDeltaTime, "frameDeltaTime must be clamped");
    RequireNear(Time::unscaledDeltaTime, Time::frameDeltaTime, 0.0001f,
                "unscaledDeltaTime must match frameDeltaTime");
    RequireNear(Time::unscaledTime, Time::frameDeltaTime, 0.001f,
                "unscaledTime must advance by frameDeltaTime");
    RequireNear(Time::time, Time::frameDeltaTime * 0.5f, 0.002f,
                "time must use timeScale");
    RequireNear(Time::deltaTime, Time::frameDeltaTime * 0.5f, 0.002f,
                "deltaTime must use timeScale");
}

void PauseStopsScaledTimeButNotUnscaledTime()
{
    Time::Initialize();
    Time::SetPaused(true);

    WaitForElapsedTime();
    Time::BeginFrame();

    RequireNear(Time::deltaTime, 0.0f, 0.0001f, "Paused time must have zero deltaTime");
    RequireNear(Time::time, 0.0f, 0.0001f, "Paused time must not advance");
    Require(Time::unscaledDeltaTime > 0.0f, "Unscaled time must continue while paused");
    Require(Time::unscaledTime > 0.0f, "unscaledTime must continue while paused");
}

void FixedStepConsumesAccumulatedTime()
{
    Time::Initialize();
    Time::fixedDeltaTime = 0.01f;

    WaitForElapsedTime();
    Time::BeginFrame();

    int consumedSteps = 0;
    while (Time::HasFixedStep()) {
        Time::ConsumeFixedStep();
        ++consumedSteps;
    }

    Require(consumedSteps >= 1, "Elapsed time must produce at least one fixed step");
    Require(Time::fixedFrameCount == static_cast<unsigned long long>(consumedSteps),
            "fixedFrameCount must match consumed fixed steps");
    RequireNear(Time::fixedTime,
                static_cast<float>(consumedSteps) * Time::fixedDeltaTime,
                0.0001f,
                "fixedTime must advance by fixedDeltaTime");
    Require(Time::GetInterpolationAlpha() >= 0.0f &&
                Time::GetInterpolationAlpha() <= 1.0f,
            "Interpolation alpha must be in the range [0, 1]");
}

} // namespace

int main()
{
    try {
        InitializeResetsState();
        BeginFrameUpdatesScaledAndUnscaledTime();
        PauseStopsScaledTimeButNotUnscaledTime();
        FixedStepConsumesAccumulatedTime();
    } catch (const std::exception& exception) {
        std::cerr << "TimeTests failed: " << exception.what() << '\n';
        return 1;
    }

    std::cout << "TimeTests passed\n";
    return 0;
}
