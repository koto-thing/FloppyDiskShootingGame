#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

using Microsoft::WRL::ComPtr;
struct Pixel { float r, g, b, a; };
constexpr UINT LowWidth = 640, LowHeight = 360;

/** @brief 画面中央の縦方向20画素で走査線の相対振幅を測る @param pixels 出力色 @param width 幅 @param height 高さ @return 最大値に対する明暗差の比 */
float ScanlineContrast(const std::vector<Pixel>& pixels, UINT width, UINT height) {
    float low = 1, high = 0;
    for (UINT y = height / 2; y < height / 2 + 20; ++y) {
        low = std::min(low, pixels[y * width + width / 2].r);
        high = std::max(high, pixels[y * width + width / 2].r);
    }
    return (high - low) / high;
}

/** @brief 実ゲームのシェーダーをコンパイルする @param entry エントリー名 @param profile 対象ステージ @return バイトコード */
ComPtr<ID3DBlob> Compile(const char* entry, const char* profile) {
    ComPtr<ID3DBlob> code, error;
    const HRESULT result = D3DCompileFromFile(L"Shaders/PixelUpscaleShader.hlsl", nullptr, nullptr,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &code, &error);
    if (error) std::fputs(static_cast<const char*>(error->GetBufferPointer()), stderr);
    assert(SUCCEEDED(result));
    return code;
}

/** @brief 実シェーダーをオフスクリーン描画して色を読み戻す @param width 出力幅 @param height 出力高さ @param sample 入力色 @param preview 比較用階調画像の生成有無 @return 出力画素 */
std::vector<Pixel> Render(UINT width, UINT height, Pixel sample, bool preview = false) {
    // 同じShader Model 5を扱えるWindows標準WARPで、ゲームやGPUに依存せず検査する
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &device, nullptr, &context)));
    const auto vs = Compile("VSMain", "vs_5_0"), ps = Compile("PSMain", "ps_5_0");
    ComPtr<ID3D11VertexShader> vertex;
    ComPtr<ID3D11PixelShader> pixel;
    assert(SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertex)));
    assert(SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixel)));

    // グラデーションと原色を含む低解像度の入力を用意する
    std::vector<Pixel> source(LowWidth * LowHeight, sample);
    if (preview) {
        for (UINT y = 0; y < LowHeight; ++y) for (UINT x = 0; x < LowWidth; ++x) {
            const float value = static_cast<float>(x) / (LowWidth - 1);
            source[y * LowWidth + x] = y < 180 ? Pixel{value, value, value, 1} :
                y < 240 ? Pixel{value, 0, 0, 1} : y < 300 ? Pixel{0, value, 0, 1} : Pixel{0, 0, value, 1};
        }
    }
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = LowWidth; desc.Height = LowHeight;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{source.data(), LowWidth * sizeof(Pixel), 0};
    ComPtr<ID3D11Texture2D> input, target, readback;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, &data, &input)));
    ComPtr<ID3D11ShaderResourceView> srv;
    assert(SUCCEEDED(device->CreateShaderResourceView(input.Get(), nullptr, &srv)));
    desc.Width = width; desc.Height = height;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)));
    ComPtr<ID3D11RenderTargetView> rtv;
    assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
    desc.BindFlags = 0; desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &readback)));
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> sampler;
    assert(SUCCEEDED(device->CreateSamplerState(&samplerDesc, &sampler)));

    // 実ゲームと同じ全画面三角形を描画する
    const D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    context->RSSetViewports(1, &viewport);
    context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex.Get(), nullptr, 0);
    context->PSSetShader(pixel.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());
    context->PSSetSamplers(0, 1, sampler.GetAddressOf());
    context->Draw(3, 0);
    context->CopyResource(readback.Get(), target.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    assert(SUCCEEDED(context->Map(readback.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
    std::vector<Pixel> result(width * height);
    for (UINT y = 0; y < height; ++y) {
        const auto* row = reinterpret_cast<const Pixel*>(static_cast<const char*>(mapped.pData) + y * mapped.RowPitch);
        std::copy_n(row, width, result.data() + y * width);
    }
    context->Unmap(readback.Get(), 0);
    return result;
}

/** @brief 確認用画像をBMPへ保存する @param pixels 出力色 @param width 幅 @param height 高さ @return なし */
void SavePreview(const std::vector<Pixel>& pixels, UINT width, UINT height) {
    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + width * height * 4;
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -static_cast<LONG>(height);
    info.biPlanes = 1; info.biBitCount = 32;
    char path[128];
    std::snprintf(path, sizeof(path), "obj/RetroEffectTests/Preview-%u.bmp", height);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file), sizeof(file));
    output.write(reinterpret_cast<const char*>(&info), sizeof(info));
    for (const auto& p : pixels) {
        const unsigned char bgra[] = {static_cast<unsigned char>(p.b * 255 + 0.5f),
            static_cast<unsigned char>(p.g * 255 + 0.5f), static_cast<unsigned char>(p.r * 255 + 0.5f), 255};
        output.write(reinterpret_cast<const char*>(bgra), sizeof(bgra));
    }
    assert(output.good());
}

/** @brief 黒・白・原色・透明度・ディザ・走査線を実シェーダーで検査する @return 成功時0 */
int main() {
    // 等倍以下と整数倍率と非整数倍率の表示解像度で色と走査線の振幅を検査する
    for (const UINT height : {144u, 360u, 720u, 900u, 1080u, 1440u}) {
        const UINT width = height * 16 / 9;
        for (const Pixel sample : {Pixel{0, 0, 0, 0.37f}, Pixel{1, 1, 1, 0.37f}, Pixel{1, 0, 0, 0.37f}}) {
            const auto pixels = Render(width, height, sample);
            for (const auto& p : pixels) {
                assert(std::isfinite(p.r) && std::isfinite(p.g) && std::isfinite(p.b));
                assert(std::abs(p.a - sample.a) < 0.00001f);
                assert(p.r >= sample.r * 0.75f && p.r <= sample.r);
                assert(p.g >= sample.g * 0.75f && p.g <= sample.g);
                assert(p.b >= sample.b * 0.75f && p.b <= sample.b);
            }
            if (sample.g == 1) {
                const float center = pixels[(height / 2) * width + width / 2].r;
                assert(center > pixels[(height / 2) * width].r + 0.02f);
                // 白は走査線でほとんど暗くならない
                assert(ScanlineContrast(pixels, width, height) < 0.05f);
            }
        }
        // 暗い色には拡大時だけ走査線が現れ、2倍表示でも消えない
        const float dark = ScanlineContrast(Render(width, height, {0.2f, 0.2f, 0.2f, 1}), width, height);
        assert(height <= LowHeight ? dark < 0.01f : dark > 0.08f);
        SavePreview(Render(width, height, {}, true), width, height);
        std::printf("Retro effect: %ux%u passed\n", width, height);
    }
    // 一様な中間色から隣接する2階調が現れ、4x4の網目が繰り返されることを検査する
    const auto pixels = Render(LowWidth, LowHeight, {0.5f, 0.5f, 0.5f, 1});
    int dark = 0, light = 0;
    for (UINT y = 176; y < 180; ++y) for (UINT x = 316; x < 320; ++x) {
        const float value = pixels[y * LowWidth + x].r;
        dark += value < 0.5f; light += value > 0.5f;
        assert(std::abs(value - pixels[y * LowWidth + x + 4].r) < 0.001f);
    }
    assert(dark == 8 && light == 8);

    // 減色とディザの後も平均の明るさが入力の表示値から外れないことを検査する
    for (const float input : {0.1f, 0.25f, 0.5f, 0.75f, 0.9f}) {
        const auto uniform = Render(LowWidth, LowHeight, {input, input, input, 1});
        float sum = 0;
        for (UINT y = 176; y < 180; ++y) for (UINT x = 316; x < 320; ++x) sum += uniform[y * LowWidth + x].r;
        assert(std::abs(sum / 16 - input) < 0.04f);
    }
    std::puts("Retro effect tests passed");
}
