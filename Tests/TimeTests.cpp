#include "../Engine/Time/Time.h"
#include "../Engine/Math/Quaternion.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

void RunTransformTests();
void RunGeometryTests();
void RunRendererTests();
void RunObjectPoolTests();
void RunCollisionServiceTests();
void RunSceneManagerTests();
void RunCameraTests();
void RunGeometry3DTests();
void RunUITests();
void RunScoreRepositoryTests();
void RunStage4BossModelViewTests();
void RunStage5ModelViewTests();
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

/**
 * @brief ベクトルの基本演算とゼロベクトルの正規化を検証する
 */
void VectorOperationsProduceExpectedResults()
{
    const Vector3 a{1.0f, 2.0f, 3.0f};
    const Vector3 b{4.0f, 5.0f, 6.0f};

    Require(Vector3::Dot(a, b) == 32.0f, "Vector3 dot product must match");
    Require(Vector3::Cross(Vector3::Right, Vector3::Up) == Vector3::Forward,
            "Vector3 cross product must follow the right-handed rule");
    Require(Vector3::Zero.Normalized() == Vector3::Zero,
            "Normalizing zero must remain zero");
}

/**
 * @brief 行列の座標変換と逆行列を検証する
 */
void MatrixOperationsProduceExpectedResults()
{
    const Matrix4x4 transform =
        Matrix4x4::Translation({2.0f, 3.0f, 4.0f}) *
        Matrix4x4::Scale({2.0f, 2.0f, 2.0f});
    const Vector3 transformed = transform.TransformPoint({1.0f, 1.0f, 1.0f});

    RequireNear(transformed.x, 4.0f, 0.0001f, "Matrix transform x must match");
    RequireNear(transformed.y, 5.0f, 0.0001f, "Matrix transform y must match");
    RequireNear(transformed.z, 6.0f, 0.0001f, "Matrix transform z must match");

    Matrix4x4 inverse;
    Require(transform.TryInverse(inverse), "Invertible matrix must produce an inverse");
    const Vector3 restored = inverse.TransformPoint(transformed);
    RequireNear(restored.x, 1.0f, 0.0001f, "Inverse transform x must match");
    RequireNear(restored.y, 1.0f, 0.0001f, "Inverse transform y must match");
    RequireNear(restored.z, 1.0f, 0.0001f, "Inverse transform z must match");
}

/**
 * @brief クォータニオンの回転と球面線形補間を検証する
 */
void QuaternionOperationsProduceExpectedResults()
{
    const Quaternion rotation = Quaternion::FromAxisAngle(Vector3::Up, Math::HalfPi);
    const Vector3 rotated = rotation.Rotate(Vector3::Forward);

    RequireNear(rotated.x, 1.0f, 0.0001f, "Quaternion rotation x must match");
    RequireNear(rotated.z, 0.0f, 0.0001f, "Quaternion rotation z must match");

    const Quaternion halfway = Quaternion::Slerp(Quaternion::Identity, rotation, 0.5f);
    RequireNear(halfway.Length(), 1.0f, 0.0001f, "Slerp result must remain normalized");
}

} // namespace

int main()
{
    try {
        InitializeResetsState();
        BeginFrameUpdatesScaledAndUnscaledTime();
        PauseStopsScaledTimeButNotUnscaledTime();
        FixedStepConsumesAccumulatedTime();
        VectorOperationsProduceExpectedResults();
        MatrixOperationsProduceExpectedResults();
        QuaternionOperationsProduceExpectedResults();
        RunTransformTests();
        RunGeometryTests();
        RunRendererTests();
        RunObjectPoolTests();
        RunCollisionServiceTests();
        RunSceneManagerTests();
        RunCameraTests();
        RunGeometry3DTests();
        RunUITests();
        RunScoreRepositoryTests();
        RunStage4BossModelViewTests();
        RunStage5ModelViewTests();
    } catch (const std::exception& exception) {
        std::cerr << "TimeTests failed: " << exception.what() << '\n';
        return 1;
    }

    std::cout << "TimeTests passed\n";
    return 0;
}
