#include <SDL.h>
#include <cstdio>
#include <stdexcept>
#include "../Infrastructure/ExternalServices/PortableRenderingService.h"

/** @brief 全描画経路とリサイズを実GPUで検査する @return 成功時0 */
int main() {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("Portable renderer smoke", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 640, 360, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!window) { std::fprintf(stderr, "%s\n", SDL_GetError()); SDL_Quit(); return 1; }
    int result = 0;
    try {
        PortableRenderingService renderer;
        if (renderer.Initialize(nullptr, 640, 360)) throw std::runtime_error("Null window accepted");
        if (!renderer.Initialize(window, 640, 360)) throw std::runtime_error("Renderer initialization failed");
        // 全プリミティブ、専用PSO、通常解像度、レトロ解像度、HUDを同じフレームで通す
        for (int frame = 0; frame < 6; ++frame) {
            if (frame == 3) {
                SDL_SetWindowSize(window, 800, 600);
                SDL_PumpEvents();
                renderer.Resize(800, 600);
            }
            renderer.SetRetroEffectEnabled(frame % 2 == 0);
            renderer.BeginFrame();
            renderer.SetCamera({}, {0, 0, renderer.Width(), renderer.Height()});
            for (auto pipeline : {PipelineId::Background, PipelineId::Object, PipelineId::SpellCircle, PipelineId::Model3D}) {
                renderer.SetPipeline(pipeline);
                for (auto shape : {PrimitiveShape::Plate, PrimitiveShape::Box, PrimitiveShape::Sphere,
                    PrimitiveShape::Cylinder, PrimitiveShape::Cone, PrimitiveShape::Prism, PrimitiveShape::Sprite2D}) {
                    Primitive3D primitive;
                    primitive.shape = shape;
                    primitive.wvpMatrix = Matrix4x4::Translation({0.25f, -0.2f, 0.5f}) *
                        Matrix4x4::RotationY(0.4f) * Matrix4x4::Scale({0.3f, 0.3f, 0.3f});
                    renderer.DrawPrimitive3D(primitive);
                }
            }
            for (int type = 0; type <= 6; ++type) {
                PlayerShotVisual shot;
                shot.position = {-0.6f + type * 0.2f, 0.4f};
                shot.size = {0.08f, 0.05f};
                shot.type = type;
                renderer.DrawPlayerShot(shot);
                shot.depthTest = true;
                renderer.DrawPlayerShot(shot);
            }
            for (int type = 0; type <= 4; ++type) {
                ExplosionVisual explosion;
                explosion.wvpMatrix = Matrix4x4::Scale({0.1f, 0.1f, 0.1f});
                explosion.progress = 0.4f;
                explosion.effectType = type;
                renderer.DrawExplosion(explosion);
                RailgunVisual railgun;
                railgun.wvpMatrix = explosion.wvpMatrix;
                railgun.progress = 0.2f;
                railgun.effectType = type;
                renderer.DrawRailgun(railgun);
            }
            renderer.ResetCamera();
            renderer.DrawRect({{-0.5f, -0.5f}, {0.2f, 0.1f}}, ColorF::Red());
            renderer.DrawCircle({{0.5f, -0.5f}, 0.1f}, ColorF::White());
            renderer.DrawTextCommand("ASCII 0123456789\n!@#?", {-0.7f, 0.8f}, 0.025f, ColorF::White(), 0);
            renderer.EndFrame();
        }
        // 最小化中は描画せず、復帰と再初期化でも解放順を検査する
        renderer.Resize(0, 0);
        renderer.BeginFrame();
        renderer.EndFrame();
        renderer.Resize(800, 600);
        renderer.BeginFrame();
        renderer.EndFrame();
        renderer.Cleanup();
        renderer.Cleanup();
        if (!renderer.Initialize(window, 800, 600)) throw std::runtime_error("Renderer reinitialization failed");
        renderer.BeginFrame();
        renderer.EndFrame();
        std::puts("Portable renderer smoke passed");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Portable renderer smoke failed: %s\n", error.what());
        result = 1;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
