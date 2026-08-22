#include "../Engine/Graphics/Camera2D.h"
#include "../Engine/Graphics/Camera3D.h"
#include <cmath>
#include <stdexcept>
namespace { void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }
void RunCameraTests() {
    Camera2D camera; camera.SetViewport({0, 0, 800, 600}); camera.SetPosition({10, 20}); camera.SetZoom(2.0f);
    Vector2 screen; Require(camera.TryWorldToScreen({10, 20}, screen) && screen == Vector2{400, 300}, "Camera2D center conversion failed");
    Vector2 world; Require(camera.TryScreenToWorld(screen, world) && (world - Vector2{10, 20}).Length() < 0.001f, "Camera2D round trip failed");
    Camera3D camera3d; camera3d.SetViewport({0, 0, 800, 600}); Require(camera3d.Forward() == Vector3::Forward, "Camera3D default forward failed"); Require(camera3d.LookAt({0, 0, 10}), "LookAt failed");
    Require(std::abs(camera3d.Forward().z - 1.0f) < 0.001f, "LookAt direction failed"); Vector2 center; Require(camera3d.TryWorldToScreen({0, 0, 10}, center), "Camera3D projection failed"); Require(std::abs(center.x - 400) < 0.01f && std::abs(center.y - 300) < 0.01f, "Camera3D center failed");
    Matrix4x4 inverse; Require(camera3d.Matrices().TryInverseViewProjection(inverse), "Camera matrix inverse failed"); Require(!camera3d.LookAt({0, 0, 0}), "LookAt same point must fail");
}
