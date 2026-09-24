#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi1_4.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>

#include "../Engine/Graphics/Utf8Text.h"
#include "../Engine/Graphics/Renderer.h"
#include "../Engine/UI/Button.h"
#include "../Infrastructure/ExternalServices/TextRenderingService.h"

namespace {
/** @brief 変換回数を確認する翻訳関数 @param text 元の文字列 @return 翻訳済み文字列 */
std::string Translate(std::string_view text) {
    return text == "PLAY" ? "日本語" : text == "日本語" ? "DOUBLE TRANSLATION" : std::string(text);
}

/** @brief UTF-8のデコード、不正入力、文字境界、折り返しを検査する @return なし */
void CheckUtf8() {
    std::size_t offset = 0;
    const std::string text = "Aé日한😀";
    for (const auto expected : {0x41u, 0xe9u, 0x65e5u, 0xd55cu, 0x1f600u}) {
        assert(Utf8Text::Next(text, offset) == expected);
    }
    assert(offset == text.size());
    assert(Utf8Text::Next(text, offset) == 0);
    for (const std::string invalid : {"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe6\x97"}) {
        offset = 0;
        assert(Utf8Text::Next(invalid, offset) == 0xfffd);
        assert(offset > 0 && offset <= invalid.size());
    }
    assert(Utf8Text::PrefixLength(text, 2) == 1);
    assert(Utf8Text::PrefixLength(text, 5) == 3);
    assert(Utf8Text::PrefixLength(text, 6) == 6);
    assert(Utf8Text::WrapLineEnd("HELLO WORLD", 1.0f, 0.1f, 0.0f) == 5);
    assert(Utf8Text::WrapLineEnd("日本語", 0.42f, 0.1f, 0.0f) == 6);
    assert(Utf8Text::WrapLineEnd("日本語", 0.01f, 0.1f, 0.0f) == 3);
    assert(Utf8Text::WrapLineEnd("", 0.01f, 0.1f, 0.0f) == 0);
    assert(std::abs(Utf8Text::Measure("日本", 0.1f, 0.0f).width - 0.41f) < 0.0001f);
    assert(std::abs(Utf8Text::Measure("éé", 0.1f, 0.0f).width - 0.35f) < 0.0001f);
}

/** @brief 翻訳後の中央配置、ボタン内縮小、UTF-8保持を検査する @return なし */
void CheckCommands() {
    Renderer renderer;
    renderer.SetTextTranslator(&Translate);
    renderer.BeginFrame();
    renderer.DrawText("PLAY", TextAlign::Center, 0.1f, ColorF::White());
    const auto& centered = renderer.Command(0);
    assert(std::string_view(centered.text.data()) == "日本語");
    assert(std::abs(centered.position.x + 0.21f) < 0.0001f);

    // 511バイト上限で3バイト文字を分断しない
    renderer.DrawText(std::string(509, 'A') + "日", {}, 0.1f, ColorF::White());
    assert(renderer.Command(1).textLength == 509);
    renderer.DrawText(std::string(200, 'A'), TextAlign::TopCenter, 0.1f,
                      ColorF::White(), {0.5f, 0.0f});
    const auto& fitted = renderer.Command(2);
    const auto fittedWidth = Utf8Text::Measure(fitted.text.data(), fitted.size, fitted.characterSpacing).width;
    assert(fittedWidth <= 0.961f);
    assert(fitted.position.x - fitted.size >= 0.0f);
    assert(fitted.position.x - fitted.size + fittedWidth <= 1.0f);

    renderer.BeginFrame();
    Button button(Rect{{-0.1f, -0.05f}, {0.2f, 0.1f}}, "PLAY");
    button.textSize = 0.1f;
    button.Render(renderer);
    const auto& label = renderer.Command(1);
    const auto width = Utf8Text::Measure(label.text.data(), label.size, label.characterSpacing).width;
    assert(std::string_view(label.text.data()) == "日本語");
    assert(width <= 0.181f);
    assert(std::abs(label.position.x - label.size + width * 0.5f) < 0.0001f);
}

/**
 * @brief 7言語をGPU描画して各行を検査し、目視確認用BMPを保存する
 * @param device WARPのD3D12デバイス
 * @param renderer 初期化済みの文字描画サービス
 * @return なし
 */
void CheckRendering(ID3D12Device* device, TextRenderingService& renderer) {
    constexpr UINT width = 1280, height = 720;
    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    assert(SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12CommandAllocator> allocator;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))));
    ComPtr<ID3D12GraphicsCommandList> commands;
    assert(SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                              IID_PPV_ARGS(&commands))));

    // オフスクリーン描画先と定数バッファを実ゲームと同じ形式で用意する
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC texture{};
    texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width = width;
    texture.Height = height;
    texture.DepthOrArraySize = texture.MipLevels = 1;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.SampleDesc.Count = 1;
    texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    ComPtr<ID3D12Resource> target;
    assert(SUCCEEDED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&target))));
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = 1;
    ComPtr<ID3D12DescriptorHeap> rtv;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtv))));
    const auto handle = rtv->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(target.Get(), nullptr, handle);
    commands->OMSetRenderTargets(1, &handle, FALSE, nullptr);
    const float clear[] = {0.0f, 0.0f, 0.0f, 1.0f};
    commands->ClearRenderTargetView(handle, clear, 0, nullptr);
    const D3D12_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    const D3D12_RECT scissor{0, 0, width, height};
    commands->RSSetViewports(1, &viewport);
    commands->RSSetScissorRects(1, &scissor);
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = 1024 * 256;
    buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    ComPtr<ID3D12Resource> constants;
    assert(SUCCEEDED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constants))));
    void* mapped = nullptr;
    assert(SUCCEEDED(constants->Map(0, nullptr, &mapped)));
    const char* samples[][3] = {
        {"English  Start game  Settings", "", ""},
        {"日本語  ゲーム開始  設定  宇宙を守れ", "", ""},
        {"IDIOMA", "EFECTO RETRO: NO", "Español"},
        {"한국어  게임 시작  설정  우주를 지켜라", "", ""},
        {"简体中文  开始游戏  设置  保卫宇宙", "", ""},
        {"Français  Nouvelle partie  Paramètres", "", ""},
        {"IDIOMA", "EFEITO RETRO: NÃO", "Português"}
    };
    std::size_t cursor = 0;
    for (int line = 0; line < 7; ++line) {
        for (int column = 0; column < 3; ++column) {
            const std::string_view text = samples[line][column];
            renderer.RenderText(commands.Get(), constants->GetGPUVirtualAddress() + cursor * 256,
                static_cast<char*>(mapped) + cursor * 256, text.data(), -0.9f + column * 0.6f,
                0.8f - line * 0.25f, 0.018f, {1, 1, 1, 1}, 0.002f, width, height);
            for (std::size_t offset = 0; offset < text.size(); ++cursor) Utf8Text::Next(text, offset);
        }
        // 1文字ずつ描画する演出でも同じASCIIラベルの書体を維持する
        if (line == 2 || line == 6) {
            float x = -0.9f;
            for (char c : std::string_view("IDIOMA")) {
                const char glyph[] = {c, '\0'};
                renderer.RenderText(commands.Get(), constants->GetGPUVirtualAddress() + cursor * 256,
                    static_cast<char*>(mapped) + cursor * 256, glyph, x, 0.7f - line * 0.25f,
                    0.018f, {1, 1, 1, 1}, 0.002f, width, height);
                x += Utf8Text::Advance(c, 0.018f, 0.002f);
                ++cursor;
            }
        }
    }
    // シェーダーのu_uvOffsetScale.yを調べ、アクセントの有無や分割でアトラスが変わらないことを検査する
    for (std::size_t glyph = 0; glyph < cursor; ++glyph) {
        const auto* data = reinterpret_cast<const DirectX::XMFLOAT4*>(static_cast<char*>(mapped) + glyph * 256);
        assert(data[2].y == 1.0f);
    }
    constants->Unmap(0, nullptr);

    // GPUの完了を待って画素を読み戻し、空描画を検出する
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 byteCount = 0;
    device->GetCopyableFootprints(&texture, 0, 1, 0, &footprint, nullptr, nullptr, &byteCount);
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    buffer.Width = byteCount;
    ComPtr<ID3D12Resource> readback;
    assert(SUCCEEDED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback))));
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {target.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE};
    commands->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION destination{}, source{};
    destination.pResource = readback.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint = footprint;
    source.pResource = target.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    assert(SUCCEEDED(commands->Close()));
    ID3D12CommandList* lists[] = {commands.Get()};
    queue->ExecuteCommandLists(1, lists);
    ComPtr<ID3D12Fence> fence;
    assert(SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))));
    const HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    assert(event);
    assert(SUCCEEDED(queue->Signal(fence.Get(), 1)));
    assert(SUCCEEDED(fence->SetEventOnCompletion(1, event)));
    assert(WaitForSingleObject(event, 10000) == WAIT_OBJECT_0);
    CloseHandle(event);
    unsigned char* pixels = nullptr;
    assert(SUCCEEDED(readback->Map(0, nullptr, reinterpret_cast<void**>(&pixels))));
    for (int line = 0; line < 7; ++line) {
        const int centerY = 72 + line * 90;
        bool ink = false;
        for (int y = centerY - 22; y < centerY + 22; ++y) {
            for (UINT x = 0; x < width; ++x) ink |= pixels[y * footprint.Footprint.RowPitch + x * 4] != 0;
        }
        assert(ink);
    }

    // 白黒の32ビットBMPなのでRGB/BGRのチャンネル変換は不要
    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + width * height * 4;
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = -static_cast<LONG>(height);
    info.biPlanes = 1;
    info.biBitCount = 32;
    std::ofstream image("obj/UnicodeTextTests/UnicodeTextPreview.bmp", std::ios::binary);
    image.write(reinterpret_cast<const char*>(&file), sizeof(file));
    image.write(reinterpret_cast<const char*>(&info), sizeof(info));
    for (UINT y = 0; y < height; ++y) {
        image.write(reinterpret_cast<const char*>(pixels + y * footprint.Footprint.RowPitch), width * 4);
    }
    assert(image.good());
    readback->Unmap(0, nullptr);
}

/** @brief 同梱フォントの6言語グリフとD3D12シェーダー生成を検査する @return なし */
void CheckFontAndPipeline() {
    const auto module = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(module, L"UNICODE_FONT", RT_RCDATA);
    assert(resource && SizeofResource(module, resource) == 65536 * 32);
    const auto data = static_cast<const unsigned char*>(LockResource(LoadResource(module, resource)));
    const std::string text = "日本語設定開始Español한국어中文简体FrançaisPortuguês";
    for (std::size_t offset = 0; offset < text.size();) {
        const auto codepoint = Utf8Text::Next(text, offset);
        bool ink = false;
        for (int byte = 0; byte < 32; ++byte) ink |= data[codepoint * 32 + byte] != 0;
        assert(ink);
    }

    // ソフトウェアアダプターで実際のパイプラインと埋め込みシェーダーを検証する
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))));
    TextRenderingService renderer;
    assert(renderer.Initialize(device.Get(), DXGI_FORMAT_R8G8B8A8_UNORM));
    CheckRendering(device.Get(), renderer);
}
}

/** @brief Unicode描画の単独回帰チェックを実行する @return 成功時0 */
int main() {
    CheckUtf8();
    CheckCommands();
    CheckFontAndPipeline();
    std::puts("Unicode text tests passed");
    return 0;
}
