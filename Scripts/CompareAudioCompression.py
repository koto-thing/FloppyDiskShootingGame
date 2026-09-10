"""全音源の既存圧縮バイト列を可逆圧縮し、復元一致と容量を比較する"""
import bz2
import ctypes as ct
import hashlib
import json
import lzma
from pathlib import Path
import re
import statistics
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Temp/AudioCompressionComparison"

# Windows標準APIのサイズ型とハンドル型を明示する
cab = ct.WinDLL("cabinet", use_last_error=True)
for operation in ("Compressor", "Decompressor"):
    create = getattr(cab, "Create" + operation)
    create.argtypes = [ct.c_uint32, ct.c_void_p, ct.POINTER(ct.c_void_p)]
    create.restype = ct.c_int
    close = getattr(cab, "Close" + operation)
    close.argtypes = [ct.c_void_p]
    close.restype = ct.c_int
for operation in ("Compress", "Decompress"):
    fn = getattr(cab, operation)
    fn.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_size_t, ct.c_void_p,
                   ct.c_size_t, ct.POINTER(ct.c_size_t)]
    fn.restype = ct.c_int


def windows_codec(data, algorithm, decode=False):
    """@brief Windows標準圧縮APIを呼ぶ
    @param data 入力バイト列
    @param algorithm Windows圧縮アルゴリズム番号
    @param decode 展開する場合True
    @return 変換後のバイト列
    """
    kind = "Decompressor" if decode else "Compressor"
    fn = cab.Decompress if decode else cab.Compress
    handle = ct.c_void_p()
    if not getattr(cab, "Create" + kind)(algorithm, None, ct.byref(handle)):
        raise ct.WinError(ct.get_last_error())
    try:
        # 必要容量を問い合わせ、その容量で変換する
        size = ct.c_size_t()
        ok = fn(handle, data, len(data), None, 0, ct.byref(size))
        if not ok and ct.get_last_error() != 122:
            raise ct.WinError(ct.get_last_error())
        output = ct.create_string_buffer(size.value)
        if not fn(handle, data, len(data), output, len(output), ct.byref(size)):
            raise ct.WinError(ct.get_last_error())
        return output.raw[:size.value]
    finally:
        getattr(cab, "Close" + kind)(handle)


def main():
    """@brief 全方式を比較して復元一致を検証する
    @return なし
    """
    OUT.mkdir(parents=True, exist_ok=True)
    # ヘッダーの文字数ではなく全uint8_t配列の実バイト数を計測する
    groups = {}
    manifest = []
    offset = 0
    for group, relative in (
        ("instruments", "Infrastructure/ExternalServices/WavSamplesSource.h"),
        ("voices", "Presentation/Gameplay/Voices/VoiceSamplesSource.h"),
    ):
        source = (ROOT / relative).read_text(encoding="utf-8-sig")
        arrays = re.findall(r"inline constexpr std::uint8_t\s+(\w+)\[\]\s*=\s*\{(.*?)\};", source, re.S)
        assert arrays and len(arrays) == len(re.findall(r"inline constexpr std::uint8_t", source))
        groups[group] = []
        for name, body in arrays:
            assert not re.sub(r"0x[0-9A-Fa-f]{2}|[\s,]", "", body), name
            data = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", body))
            assert data and f"sizeof({name})" in source, name
            groups[group].append(data)
            manifest.append(dict(group=group, source=relative, name=name, offset=offset,
                                 size=len(data), sha256=hashlib.sha256(data).hexdigest()))
            offset += len(data)

    codecs = {
        "zlib-9": (lambda data: zlib.compress(data, 9), zlib.decompress),
        "bzip2-9": (lambda data: bz2.compress(data, 9), bz2.decompress),
        "LZMA2-XZ-9": (lambda data: lzma.compress(data, preset=9), lzma.decompress),
    }
    for name, algorithm in (("MSZIP", 2), ("XPRESS", 3), ("XPRESS-HUFF", 4), ("LZMS", 5)):
        codecs[name] = (lambda data, a=algorithm: windows_codec(data, a),
                        lambda data, a=algorithm: windows_codec(data, a, True))

    all_samples = sum(groups.values(), [])
    joined = b"".join(all_samples)
    rows = []
    for name, (compress, decompress) in codecs.items():
        # 各配列、カテゴリ別、一括の全ケースで復元一致を必須にする
        individual = 0
        for data in all_samples:
            packed = compress(data)
            assert decompress(packed) == data, name
            individual += len(packed)
        category_sizes = {}
        for group, samples in groups.items():
            data = b"".join(samples)
            packed = compress(data)
            assert decompress(packed) == data, name
            category_sizes[group] = len(packed)
        packed = compress(joined)
        timings = []
        for _ in range(7):
            start = time.perf_counter()
            restored = decompress(packed)
            timings.append((time.perf_counter() - start) * 1000)
            assert restored == joined, name
        for entry in manifest:
            restored_sample = restored[entry["offset"]:entry["offset"] + entry["size"]]
            assert hashlib.sha256(restored_sample).hexdigest() == entry["sha256"], entry["name"]
        (OUT / (name + ".bin")).write_bytes(packed)
        rows.append(dict(codec=name, individual_bytes=individual, **category_sizes,
                         separate_bytes=sum(category_sizes.values()), combined_bytes=len(packed),
                         saved_bytes=len(joined) - len(packed),
                         saved_percent=round(100 * (1 - len(packed) / len(joined)), 2),
                         decode_ms_median=round(statistics.median(timings), 3)))

    # 再現用の全配列台帳と比較結果を残す
    result = dict(original_bytes=len(joined), groups={
        group: dict(count=len(samples), bytes=sum(map(len, samples)))
        for group, samples in groups.items()}, rows=rows, manifest=manifest)
    (OUT / "results.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({key: value for key, value in result.items() if key != "manifest"}, indent=2))


if __name__ == "__main__":
    main()
