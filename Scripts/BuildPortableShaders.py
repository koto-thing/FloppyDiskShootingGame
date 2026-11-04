"""既存HLSLをVulkan SPIR-VとWebGL 2 GLSL ESへ変換し、描画契約を検証する"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(command):
    """@brief 外部コンパイラーを実行し、失敗時には診断を保持する
    @param command シェルを介さない引数の配列
    @return 標準出力
    """
    result = subprocess.run([str(arg) for arg in command], capture_output=True, text=True,
                            encoding="utf-8", errors="replace")
    if result.returncode:
        raise RuntimeError(f"{' '.join(map(str, command))}\n{result.stdout}{result.stderr}")
    return result.stdout


def sources(root):
    """@brief 既存ソースから全描画プログラムのHLSLと入口名を取得する
    @param root リポジトリのルート
    @return プログラム名から頂点・ピクセルシェーダーへの辞書
    """
    def read(relative):
        """@brief UTF-8ソースを読む
        @param relative ルートからの相対パス
        @return ソース文字列
        """
        return (root / relative).read_text(encoding="utf-8-sig")

    # 背景と魔法陣はDirectX版と同じObjectの頂点シェーダーを共有する
    object_source = read("Shaders/ObjectShader.hlsl")
    result = {"object": ((object_source, "VSMain"), (object_source, "PSObject"))}
    for name, file, entry in (("background", "BackgroundShader", "PSBackground"),
                              ("spell", "SpellCircleShader", "PSSpellCircle")):
        result[name] = ((object_source, "VSMain"), (read(f"Shaders/{file}.hlsl"), entry))

    # 埋め込みHLSLをそのまま抽出し、Windows側の効果の更新を同時に取り込む
    renderer = read("Infrastructure/ExternalServices/D3D12RenderingService.cpp")
    for name, symbol, entry in (("shot", "PlayerShot", "PlayerShot"),
                                ("explosion", "Explosion", "Explosion"),
                                ("railgun", "Railgun", "Railgun")):
        match = re.search(rf"constexpr char {symbol}ShaderCode\[\]\s*=\s*EMBEDDED_HLSL\((.*?)^\);",
                          renderer, re.S | re.M)
        if not match:
            raise ValueError(f"Embedded shader not found: {symbol}ShaderCode")
        result[name] = ((match[1], f"VS{entry}"), (match[1], f"PS{entry}"))
    for name, file in (("font", "PortableFontShader"), ("upscale", "PixelUpscaleShader")):
        source = read(f"Shaders/{file}.hlsl")
        result[name] = ((source, "VSMain"), (source, "PSMain"))
    return result


def check_reflection(reflection, name, stage):
    """@brief UBO配置とテクスチャのbindingを実行時の契約と照合する
    @param reflection SPIRV-CrossのJSON反射情報
    @param name プログラム名
    @param stage vertまたはfrag
    @return なし
    """
    entry = "VSMain" if stage == "vert" else "PSMain"
    if reflection["entryPoints"] != [{"name": entry, "mode": stage}]:
        raise ValueError(f"{name}.{stage}: unexpected entrypoint")
    for block in reflection.get("ubos", []):
        expected_size = 64 if name == "font" else 96
        if (block["set"], block["binding"], block["block_size"]) != (0, 0, expected_size):
            raise ValueError(f"{name}.{stage}: incompatible uniform block {block}")
        members = reflection["types"][block["type"]]["members"]
        expected_offsets = [0, 16, 32, 48] if name == "font" else [0, 64, 80, 84, 88, 92]
        if [member["offset"] for member in members] != expected_offsets:
            raise ValueError(f"{name}.{stage}: incompatible member offsets {members}")
        if name != "font" and (members[0].get("matrix_stride"), members[0].get("row_major")) != (16, True):
            raise ValueError(f"{name}.{stage}: incompatible DirectX matrix layout")
    for resource, binding in (("separate_images", 1), ("separate_samplers", 2)):
        for item in reflection.get(resource, []):
            if name != "upscale" or (item["set"], item["binding"]) != (0, binding):
                raise ValueError(f"{name}.{stage}: incompatible texture resource {item}")


def build(root, output, mode, glslang, spirv_cross):
    """@brief シェーダーを変換し、全ステージの配置とGLSLリンクを検証する
    @param root リポジトリのルート
    @param output 実行時シェーダーの出力ディレクトリ
    @param mode vulkan、webまたはall
    @param glslang glslang実行ファイル
    @param spirv_cross SPIRV-Cross実行ファイル
    @return なし
    """
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="spaceyakuza-shaders-") as temporary:
        work = Path(temporary)
        for name, stages in sources(root).items():
            interfaces = {}
            for stage, (source, source_entry) in zip(("vert", "frag"), stages):
                # 既存の逆向きsmoothstepはGLSL/SPIR-Vでは未定義なので正向きの補数へ変換する
                if name in ("background", "spell") and stage == "frag":
                    source = re.sub(r"smoothstep\((\d+\.\d+f?),\s*0\.0f?,",
                                    r"PortableInverseSmoothstep(\1, 0.0,", source)
                    source = """/** @brief 逆向きのHermite補間を定義済みの演算で計算する
 * @param high 上端 @param low 下端 @param value 入力値
 * @return 上端で0、下端で1となる補間値 */
float PortableInverseSmoothstep(float high, float low, float value)
{
    // GLSLのsmoothstepには小さい端点を先に渡す
    return 1.0 - smoothstep(low, high, value);
}
""" + source
                for backend in (("vulkan", "web") if mode == "all" else (mode,)):
                    # OpenGLのRenderTargetは上下が逆になるためサンプル座標だけを補正する
                    text = source
                    if backend == "web" and name == "upscale" and stage == "frag":
                        text, count = re.subn(r"sceneTexture\.Sample\(pointSampler,\s*input\.uv\)",
                            "sceneTexture.Sample(pointSampler, float2(input.uv.x, 1.0 - input.uv.y))", text)
                        if count != 1:
                            raise ValueError("Upscale sample changed; update WebGL texture orientation fix")
                    hlsl = work / f"{name}.{stage}.hlsl"
                    hlsl.write_text(text, encoding="utf-8")
                    spv = work / f"{name}.{stage}.spv"
                    entry = "VSMain" if stage == "vert" else "PSMain"
                    command = [glslang, "-D", "-V", "--target-env", "vulkan1.0", "-S", stage,
                               "-e", entry, "--source-entrypoint", source_entry, "--auto-map-locations",
                               "--hlsl-iomap", "--shift-texture-binding", "1", "--shift-sampler-binding", "2"]
                    if backend == "vulkan" and stage == "vert":
                        command.append("--invert-y")
                    run(command + ["-o", spv, hlsl])
                    reflection = json.loads(run([spirv_cross, spv, "--reflect"]))
                    check_reflection(reflection, name, stage)
                    interfaces[stage] = reflection
                    if backend == "vulkan":
                        shutil.copyfile(spv, output / spv.name)
                        continue

                    # ES300はvaryingのlocationによるリンクを持たないため同じ名前に揃える
                    glsl = work / f"{name}.{stage}"
                    command = [spirv_cross, spv, "--es", "--version", "300", "--output", glsl]
                    if stage == "vert":
                        command.append("--fixup-clipspace")
                    direction = "out" if stage == "vert" else "in"
                    for varying in reflection.get("outputs" if stage == "vert" else "inputs", []):
                        location = varying["location"]
                        command += ["--rename-interface-variable", direction, str(location), f"varying_{location}"]
                    run(command)
                    shutil.copyfile(glsl, output / f"{name}.{stage}.glsl")

            # 最適化後も全ピクセル入力が対応する頂点出力を持つことを確認する
            outputs = {v["location"]: v["type"] for v in interfaces["vert"].get("outputs", [])}
            for item in interfaces["frag"].get("inputs", []):
                if outputs.get(item["location"]) != item["type"]:
                    raise ValueError(f"{name}: vertex/fragment interface mismatch: {item}")
            if mode in ("web", "all"):
                run([glslang, "-l", work / f"{name}.vert", work / f"{name}.frag"])
            print(f"Verified {name} ({mode})")


def main():
    """@brief CLIの引数を解析してビルドまたは独立検証を実行する
    @return なし
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--mode", choices=("vulkan", "web", "all"), default="all")
    parser.add_argument("--glslang", default=shutil.which("glslangValidator") or shutil.which("glslang"))
    parser.add_argument("--spirv-cross", default=shutil.which("spirv-cross"))
    parser.add_argument("--check", action="store_true", help="Compile and validate in a temporary directory")
    args = parser.parse_args()
    if not args.glslang or not args.spirv_cross:
        parser.error("glslang and spirv-cross are required; install them or pass explicit paths")
    if not args.output and not args.check:
        parser.error("--output or --check is required")
    if args.check:
        with tempfile.TemporaryDirectory(prefix="spaceyakuza-shader-check-") as temporary:
            build(args.source_root, Path(temporary), args.mode, args.glslang, args.spirv_cross)
    else:
        build(args.source_root, args.output, args.mode, args.glslang, args.spirv_cross)


if __name__ == "__main__":
    main()
