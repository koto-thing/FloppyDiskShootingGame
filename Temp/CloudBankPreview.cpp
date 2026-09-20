#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <string>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
#include "../Infrastructure/ExternalServices/TextRenderingService.h"
#include "../Engine/Graphics/IRenderBackend.h"
#define private public
#include "../Infrastructure/ExternalServices/D3D12RenderingService.h"
#undef private
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"

/** @brief 実描画のRenderTargetをBMPへ保存する @param gpu 描画先 @param renderer コマンド記録 @param path 保存先 @return なし */
void SaveFrame(D3D12RenderingService& gpu, Renderer& renderer, const char* path) {
    renderer.Flush();
    auto* target = gpu.m_renderTargets[gpu.GetFrameIndex()].Get();
    const auto description = target->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint {};
    UINT64 bytes = 0;
    gpu.GetDevice()->GetCopyableFootprints(&description, 0, 1, 0, &footprint, nullptr, nullptr, &bytes);
    D3D12_HEAP_PROPERTIES heap {};
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer {};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = bytes;
    buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> readback;
    assert(SUCCEEDED(gpu.GetDevice()->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
        &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback))));

    // 実ゲームの描画結果をGPUから読み戻す
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = target;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    gpu.GetCommandList()->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION source {};
    source.pResource = target;
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION destination {};
    destination.pResource = readback.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint = footprint;
    gpu.GetCommandList()->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    gpu.GetCommandList()->ResourceBarrier(1, &barrier);
    renderer.EndFrame();

    // RGBAをBMPのBGRAへ並べ替える
    unsigned char* mapped = nullptr;
    D3D12_RANGE range {0, static_cast<SIZE_T>(bytes)};
    assert(SUCCEEDED(readback->Map(0, &range, reinterpret_cast<void**>(&mapped))));
    const int width = static_cast<int>(description.Width);
    const int height = static_cast<int>(description.Height);
    std::vector<unsigned char> pixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto* src = mapped + footprint.Offset + y * footprint.Footprint.RowPitch + x * 4;
            auto* dst = pixels.data() + (y * width + x) * 4;
            dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = 255;
        }
    }
    readback->Unmap(0, nullptr);
    BITMAPFILEHEADER file {};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
    BITMAPINFOHEADER info {};
    info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -height;
    info.biPlanes = 1; info.biBitCount = 32;
    FILE* output = nullptr;
    fopen_s(&output, path, "wb");
    assert(output);
    fwrite(&file, sizeof(file), 1, output);
    fwrite(&info, sizeof(info), 1, output);
    fwrite(pixels.data(), pixels.size(), 1, output);
    fclose(output);
}

struct CoopStageTests {
    /** @brief 実際の2D・遷移中・3D背景を夜明けの各段階で保存する @return なし */
    static void Run() {
        WNDCLASSW windowClass {};
        windowClass.lpfnWndProc = DefWindowProcW;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"CloudBankPreview";
        RegisterClassW(&windowClass);
        HWND window = CreateWindowW(windowClass.lpszClassName, L"Cloud Bank Preview",
            WS_OVERLAPPEDWINDOW, 0, 0, 1024, 576, nullptr, nullptr, windowClass.hInstance, nullptr);
        D3D12RenderingService gpu;
        assert(gpu.Initialize(window, 1024, 576));
        gpu.SetRetroEffectEnabled(false);
        auto renderer = std::make_unique<Renderer>(gpu);
        auto game = std::make_unique<SideScrollingShooter>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage = &SideScrollingShooter::Stage5Module::Definition(Easy);
        g.m_stage5.phase = ShooterStages::Stage5::Phase::TayamaDragonBattle;
        g.m_stage5.tayamaMaxHp = ShooterStages::Stage5::TayamaDragonMaxHp;
        g.m_frame = 600;
        for (int view = 0; view < 3; ++view) {
            g.m_viewMode = view == 2 ? SideScrollingShooter::ViewMode::Rail3D : SideScrollingShooter::ViewMode::Side2D;
            g.m_nextViewMode = view == 0 ? SideScrollingShooter::ViewMode::Side2D : SideScrollingShooter::ViewMode::Rail3D;
            g.m_viewTransitionTimer = view == 1 ? 30 : 0;
            g.m_viewTransitionProgress = view == 1 ? 0.5f : 0.0f;
            for (int hp : {3000, 1500, 0}) {
                g.m_stage5.tayamaHp = hp;
                Camera3D camera;
                if (view == 0) g.ConfigureSideCamera(camera, *renderer);
                else g.ConfigureRailCamera(camera, *renderer);
                renderer->BeginFrame();
                SideScrollingShooter::Stage5Module::DrawSky(g, *renderer);
                renderer->SetPipeline(PipelineId::Model3D);
                renderer->SetCamera(camera);
                SideScrollingShooter::Stage5Module::DrawCloudSeaWorld(g, *renderer, camera, g.RailBlend());
                char path[128];
                sprintf_s(path, "Temp/CloudBankPreview/view%d-hp%d.bmp", view, hp);
                SaveFrame(gpu, *renderer, path);
            }
        }
        gpu.Cleanup();
        DestroyWindow(window);
    }
};

/** @brief 背景の実描画を保存する @return 成功時0 */
int main() { CoopStageTests::Run(); return 0; }
