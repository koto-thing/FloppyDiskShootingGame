#include "Switch2ProInput.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <hidsdi.h>
#include <setupapi.h>
#define INITGUID
#include <devpkey.h>
#undef INITGUID
#include <winusb.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "windowsapp.lib")

using namespace std::chrono_literals;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Foundation;

/**
 * @brief USBのIDを除いた0x09レポートを既存ゲームパッド形式へ変換する
 * @param data Bluetooth通知またはUSBレポートのペイロード
 * @param state 正常なレポートの出力先
 * @return 補正が完了した有効レポートの場合はtrue
 */
bool Switch2ProReport::Decode(std::span<const unsigned char> data, XINPUT_GAMEPAD& state) {
    // 公開された0x09形式のみを受理し、短い通知を軸として読み込まない
    if (data.size() != 63) return false;
    const std::array<int, 4> axes {
        data[5] | ((data[6] & 15) << 8), (data[6] >> 4) | (data[7] << 4),
        data[8] | ((data[9] & 15) << 8), (data[9] >> 4) | (data[10] << 4)
    };
    if (!calibrated) {
        // 接続時は手を離した中心付近の安定値を16報待つ
        bool stable = data[2] == 0 && data[3] == 0 && (data[4] & 0x1f) == 0;
        for (std::size_t i = 0; i < axes.size(); ++i) {
            stable = stable && std::abs(axes[i] - 2048) <= 512 &&
                (stableSamples == 0 || std::abs(axes[i] - candidate[i]) <= 32);
        }
        if (!stable) { stableSamples = 0; return false; }
        if (stableSamples++ == 0) candidate = axes;
        if (stableSamples < 16) return false;
        center = candidate;
        calibrated = true;
    }

    // Nintendoの印字どおりにA決定/B取消/X視点/Yボムを割り当てる
    XINPUT_GAMEPAD result {};
    constexpr WORD rightButtons[] = {XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_A,
        XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        0, XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_RIGHT_THUMB};
    constexpr WORD leftButtons[] = {XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_LEFT_SHOULDER,
        0, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB};
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (data[2] & (1u << bit)) result.wButtons |= rightButtons[bit];
        if (data[3] & (1u << bit)) result.wButtons |= leftButtons[bit];
    }
    result.bRightTrigger = (data[2] & 0x20) ? 255 : 0;
    result.bLeftTrigger = (data[3] & 0x20) ? 255 : 0;

    // ponytail: デジタル移動とUI用に中心だけを実測し、精密アナログ操作を追加する場合は工場校正値も読み出す
    SHORT* output[] = {&result.sThumbLX, &result.sThumbLY, &result.sThumbRX, &result.sThumbRY};
    for (std::size_t i = 0; i < axes.size(); ++i) {
        const int delta = axes[i] - center[i];
        const int range = delta < 0 ? center[i] : 4095 - center[i];
        *output[i] = static_cast<SHORT>(delta * (delta < 0 ? 32768 : 32767) / range);
    }
    state = result;
    return true;
}

/**
 * @brief 初代Proの0x30ペイロードを共通の操作割り当てへ変換する
 * @param data USBまたはBluetoothのIDを除いたペイロード
 * @param state 補正後の入力の出力先
 * @return 有効な補正済み入力の場合はtrue
 */
bool Switch2ProReport::DecodeSwitchPro(std::span<const unsigned char> data, XINPUT_GAMEPAD& state) {
    // USBは64報、Bluetoothは49報で、スティックの位置と12bit形式は共通
    if (data.size() != 63 && data.size() != 48) return false;
    std::array<unsigned char, 63> normalized {};
    std::copy_n(data.begin(), 11, normalized.begin());
    const auto right = data[2], system = data[3], left = data[4];
    normalized[2] = static_cast<unsigned char>(((right & 0x0c) >> 2) |
        ((right & 3) << 2) | ((right & 0xc0) >> 2) |
        ((system & 2) << 5) | ((system & 4) << 5));
    normalized[3] = static_cast<unsigned char>((left & 1) | ((left & 4) >> 1) |
        ((left & 8) >> 1) | ((left & 2) << 2) | ((left & 0xc0) >> 2) |
        ((system & 1) << 6) | ((system & 8) << 4));
    // 初代の予約ビット0x80は実機で立つためボタンとして扱わない
    normalized[4] = static_cast<unsigned char>((system & 0x30) >> 4);
    return Decode(normalized, state);
}

namespace {
constexpr GUID UsbInterface {0x6f13725e, 0xef0e, 0x4fd3, {0xae, 0x5f, 0xb2, 0xde, 0x98, 0x9e, 0xc8, 0x25}};
constexpr winrt::guid InputService {"ab7de9be-89fe-49ad-828f-118f09df7fd0"};
constexpr winrt::guid InputCharacteristic {"7492866c-ec3e-4619-8258-32755ffcc0f8"};

// 切断中の所有者も保持し、もう一方の受信スレッドによる再割り当てを防ぐ
// ponytail: 起動中は接続方式やパッド交換を固定し、必要時はゲーム開始前の再割り当て操作を追加する
std::mutex deviceAssignmentMutex;
std::array<std::wstring, 2> nativeDevicePaths;
std::array<GUID, 2> nativeContainers {};
std::array<uint64_t, 2> nativeBluetoothAddresses {};

/** @brief 接続単位の通知状態、古いBLEコールバックは新しい接続へ書き込めない */
struct Sample {
    std::mutex mutex;
    Switch2ProReport decoder;
    XINPUT_GAMEPAD state {};
    ULONGLONG received = 0;
    ULONGLONG lastReport = 0;

    /**
     * @brief 有効な通知だけを公開する
     * @param bytes IDを含まない0x09ペイロード
     * @return なし
     */
    void Receive(std::span<const unsigned char> bytes) {
        if (bytes.size() != 63) return;
        std::lock_guard lock(mutex);
        lastReport = GetTickCount64();
        if (decoder.Decode(bytes, state)) received = GetTickCount64();
    }

    /** @brief 初代Proの通知を公開する @param bytes 0x30ペイロード @return なし */
    void ReceiveSwitchPro(std::span<const unsigned char> bytes) {
        if (bytes.size() != 63 && bytes.size() != 48) return;
        std::lock_guard lock(mutex);
        lastReport = GetTickCount64();
        if (decoder.DecodeSwitchPro(bytes, state)) received = GetTickCount64();
    }
};

/**
 * @brief Nintendo純正Switch 2 ProのUSBインターフェイスを列挙する
 * @param guid 列挙するデバイスインターフェイス
 * @param productId 検出するNintendo製品ID
 * @param slot 直接接続スロット0または1
 * @param wake 入力開始用のWinUSBインターフェイスを検索する場合はtrue
 * @return 対象のデバイスパス、未検出時は空文字列
 */
std::wstring FindUsbPath(const GUID& guid, USHORT productId, int slot, bool wake = false) {
    std::lock_guard lock(deviceAssignmentMutex);
    if (nativeBluetoothAddresses[slot] != 0) return {};
    // Bluetoothや初代Proを誤って初期化しない
    const HDEVINFO devices = SetupDiGetClassDevsW(&guid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) return {};
    std::wstring result;
    SP_DEVICE_INTERFACE_DATA entry {sizeof(entry)};
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(devices, nullptr, &guid, index, &entry); ++index) {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(devices, &entry, nullptr, 0, &size, nullptr);
        if (size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<unsigned char> buffer(size);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
        detail->cbSize = sizeof(*detail);
        SP_DEVINFO_DATA info {sizeof(info)};
        if (!SetupDiGetDeviceInterfaceDetailW(devices, &entry, detail, size, nullptr, &info)) continue;
        std::wstring path = detail->DevicePath;
        std::transform(path.begin(), path.end(), path.begin(), towlower);
        const wchar_t* product = productId == 0x2009 ? L"vid_057e&pid_2009" : L"vid_057e&pid_2069";
        if (path.find(product) != std::wstring::npos || (productId == 0x2009 &&
            path.find(L"vid&0002057e_pid&2009") != std::wstring::npos)) {
            GUID container {};
            DEVPROPTYPE propertyType = 0;
            SetupDiGetDevicePropertyW(devices, &info, &DEVPKEY_Device_ContainerId, &propertyType,
                reinterpret_cast<BYTE*>(&container), sizeof(container), nullptr, 0);
            if (wake) {
                if (container == GUID{} || container != nativeContainers[slot]) continue;
            } else {
                if ((!nativeDevicePaths[slot].empty() && nativeDevicePaths[slot] != path) ||
                    nativeDevicePaths[1 - slot] == path || (container != GUID{} &&
                    container == nativeContainers[1 - slot])) continue;
                nativeDevicePaths[slot] = path;
                nativeContainers[slot] = container;
            }
            result = detail->DevicePath;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devices);
    return result;
}

/**
 * @brief WinUSBでUSB入力の送信を開始する
 * @param slot 直接接続スロット0または1
 * @return 初期化コマンドを書き込めた場合はtrue
 */
bool WakeUsb(int slot) {
    const auto path = FindUsbPath(UsbInterface, 0x2069, slot, true);
    if (path.empty()) return false;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    WINUSB_INTERFACE_HANDLE usb = nullptr;
    bool success = false;
    if (WinUsb_Initialize(file, &usb)) {
        // 通信待ちはワーカースレッド内でも上限を設ける
        USB_INTERFACE_DESCRIPTOR descriptor {};
        if (WinUsb_QueryInterfaceSettings(usb, 0, &descriptor)) {
            for (UCHAR index = 0; index < descriptor.bNumEndpoints; ++index) {
                WINUSB_PIPE_INFORMATION pipe {};
                if (!WinUsb_QueryPipe(usb, 0, index, &pipe) ||
                    pipe.PipeType != UsbdPipeTypeBulk || (pipe.PipeId & 0x80)) continue;
                ULONG timeout = 250;
                if (!WinUsb_SetPipePolicy(usb, pipe.PipeId, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout)) break;
                // 入力開始と0x09選択だけを送信し、振動や本体のペアリング情報は変更しない
                unsigned char start[] = {0x03,0x91,0,0x0d,0,8,0,0,1,0,0xff,0xff,0xff,0xff,0xff,0xff};
                unsigned char select[] = {0x03,0x91,0,0x0a,0,4,0,0,9,0,0,0};
                ULONG written = 0;
                success = WinUsb_WritePipe(usb, pipe.PipeId, start, sizeof(start), &written, nullptr) && written == sizeof(start);
                std::this_thread::sleep_for(20ms);
                success = success && WinUsb_WritePipe(usb, pipe.PipeId, select, sizeof(select), &written, nullptr) && written == sizeof(select);
                break;
            }
        }
        WinUsb_Free(usb);
    }
    CloseHandle(file);
    return success;
}

/** @brief USB読み出しのOVERLAPPEDとバッファを完了まで保持する */
struct UsbReader {
    HANDLE file = INVALID_HANDLE_VALUE;
    OVERLAPPED operation {};
    std::array<unsigned char, 64> bytes {};
    bool pending = false;
    bool originalPro = false;
    USHORT inputLength = 64;
    USHORT outputLength = 64;

    /** @brief 保留中I/Oを回収してハンドルを解放する @return なし */
    ~UsbReader() { Close(); }

    /** @brief 切断して保留中I/Oを回収する @return なし */
    void Close() {
        if (file != INVALID_HANDLE_VALUE) {
            if (pending) {
                CancelIoEx(file, &operation);
                DWORD count = 0;
                GetOverlappedResult(file, &operation, &count, TRUE);
            }
            CloseHandle(file);
        }
        if (operation.hEvent) CloseHandle(operation.hEvent);
        file = INVALID_HANDLE_VALUE;
        operation = {};
        pending = false;
    }

    /**
     * @brief HIDコマンドを期限付きで書き込む
     * @param command レポートIDを含むコマンド
     * @return 全バイトを送信した場合はtrue
     */
    bool Write(std::span<const unsigned char> command) {
        std::array<unsigned char, 64> output {};
        if (command.size() > outputLength || outputLength > output.size()) return false;
        std::copy(command.begin(), command.end(), output.begin());
        OVERLAPPED write {};
        write.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!write.hEvent) return false;
        DWORD count = 0;
        bool success = WriteFile(file, output.data(), outputLength, &count, &write) != FALSE;
        if (!success && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(write.hEvent, 250) != WAIT_OBJECT_0) CancelIoEx(file, &write);
            success = GetOverlappedResult(file, &write, &count, TRUE) != FALSE;
        }
        CloseHandle(write.hEvent);
        return success && count == outputLength;
    }

    /** @brief 対応するProのHIDを開いて入力を開始する @param slot 直接接続スロット0または1 @return 成功時はtrue */
    bool Open(int slot) {
        GUID hidGuid {};
        HidD_GetHidGuid(&hidGuid);
        auto path = FindUsbPath(hidGuid, 0x2069, slot);
        originalPro = path.empty();
        if (originalPro) path = FindUsbPath(hidGuid, 0x2009, slot);
        if (path.empty() || (!originalPro && !WakeUsb(slot))) return false;
        file = CreateFileW(path.c_str(), originalPro ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        // WindowsのHID長を使い、Bluetoothの49バイト報にも対応する
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps {};
        if (!HidD_GetPreparsedData(file, &preparsed)) { Close(); return false; }
        const auto status = HidP_GetCaps(preparsed, &caps);
        HidD_FreePreparsedData(preparsed);
        if (status != HIDP_STATUS_SUCCESS || caps.InputReportByteLength > bytes.size() ||
            caps.InputReportByteLength < 13) { Close(); return false; }
        inputLength = caps.InputReportByteLength;
        outputLength = caps.OutputReportByteLength;
        operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!operation.hEvent) { Close(); return false; }
        if (originalPro) {
            // 初代のUSBはHIDのハンドシェイクを使う、BluetoothにはUSBコマンドを送らない
            if (path.find(L"vid_057e&pid_2009") != std::wstring::npos) {
                for (const unsigned char command : std::array<unsigned char, 4>{2, 3, 2, 4}) {
                    const unsigned char packet[] = {0x80, command};
                    if (!Write(packet)) { Close(); return false; }
                    if (command == 4) break;
                    const auto deadline = GetTickCount64() + 250;
                    bool acknowledged = false;
                    while (GetTickCount64() < deadline) {
                        const int count = ReadReport();
                        if (count < 0) break;
                        if (count >= 2 && bytes[0] == 0x81 && bytes[1] == command) { acknowledged = true; break; }
                    }
                    if (!acknowledged) { Close(); return false; }
                }
            }
            // 振動は無音値のまま、連続入力0x30を要求する
            const unsigned char mode[] = {1,0,0,1,0x40,0x40,0,1,0x40,0x40,3,0x30};
            if (!Write(mode)) { Close(); return false; }
        }
        return true;
    }

    /**
     * @brief 最大50ms待機してHIDレポートをバッファへ受信する
     * @return 受信バイト数、待機中は0、切断時は-1
     */
    int ReadReport() {
        DWORD count = 0;
        if (!pending) {
            ResetEvent(operation.hEvent);
            if (ReadFile(file, bytes.data(), inputLength, &count, &operation)) return static_cast<int>(count);
            if (GetLastError() != ERROR_IO_PENDING) return -1;
            pending = true;
        }
        const DWORD wait = WaitForSingleObject(operation.hEvent, 50);
        if (wait == WAIT_TIMEOUT) return 0;
        if (wait != WAIT_OBJECT_0) return -1;
        const bool success = GetOverlappedResult(file, &operation, &count, FALSE) != FALSE;
        pending = false;
        return success ? static_cast<int>(count) : -1;
    }

    /** @brief 形式に応じてHID通知を復号する @param sample 入力の格納先 @return 接続継続時はtrue */
    bool Read(Sample& sample) {
        const int count = ReadReport();
        if (count > 0 && originalPro && bytes[0] == 0x30) {
            sample.ReceiveSwitchPro(std::span(bytes).first(count).subspan(1));
        } else if (count == 64 && !originalPro && bytes[0] == 9) {
            sample.Receive(std::span(bytes).subspan(1));
        }
        return count >= 0;
    }
};

/** @brief 一台分の直接接続をゲームループと独立して管理する */
class NativeController {
    std::mutex mutex;
    std::shared_ptr<Sample> current;
    unsigned generation = 0;
    int slot;
    std::jthread worker;

    /**
     * @brief 接続を切り替え、旧接続の通知を公開対象から外す
     * @param sample 新しい接続、切断時はnullptr
     * @return なし
     */
    void Publish(std::shared_ptr<Sample> sample) {
        std::lock_guard lock(mutex);
        current = std::move(sample);
        ++generation;
    }

    /**
     * @brief USBとBLEを検出し、接続中の入力を受信する
     * @param stop 終了要求
     * @return なし
     */
    void Run(std::stop_token stop) {
        // BLEの障害やアダプター不在でもUSB入力を継続する
        bool apartment = false;
        BluetoothLEAdvertisementWatcher watcher {nullptr};
        auto address = std::make_shared<std::atomic<uint64_t>>(0);
        BluetoothLEAdvertisementWatcher::Received_revoker discovery;
        try {
            winrt::init_apartment();
            apartment = true;
            watcher = BluetoothLEAdvertisementWatcher();
            discovery = watcher.Received(winrt::auto_revoke, [address, slot = slot](auto const&, auto const& args) {
                // NintendoのメーカーIDと製品IDの一致した広告だけに接続する
                for (auto const& manufacturer : args.Advertisement().ManufacturerData()) {
                    auto data = manufacturer.Data();
                    if (manufacturer.CompanyId() != 0x0553 || data.Length() < 7) continue;
                    const auto* p = data.data();
                    if (p[3] == 0x7e && p[4] == 0x05 && p[5] == 0x69 && p[6] == 0x20) {
                        std::lock_guard lock(deviceAssignmentMutex);
                        const uint64_t target = args.BluetoothAddress();
                        if (!nativeDevicePaths[slot].empty() || nativeBluetoothAddresses[1 - slot] == target ||
                            (nativeBluetoothAddresses[slot] != 0 && nativeBluetoothAddresses[slot] != target)) continue;
                        nativeBluetoothAddresses[slot] = target;
                        address->store(target);
                    }
                }
            });
            watcher.Start();
        } catch (winrt::hresult_error const&) {
            OutputDebugStringW(L"Switch 2 Pro: Bluetooth discovery unavailable\n");
        }

        UsbReader usb;
        BluetoothLEDevice device {nullptr};
        GattDeviceService service {nullptr};
        GattCharacteristic input {nullptr};
        GattCharacteristic::ValueChanged_revoker notification;
        BluetoothLEPreferredConnectionParametersRequest connectionParameters {nullptr};
        // 解放処理の失敗がゲームの終了やUSBへの切替を中断しないようにする
        const auto closeBluetooth = [&] {
            notification.revoke();
            input = nullptr;
            connectionParameters = nullptr;
            try { if (service) service.Close(); } catch (winrt::hresult_error const&) {}
            service = nullptr;
            try { if (device) device.Close(); } catch (winrt::hresult_error const&) {}
            device = nullptr;
        };
        std::shared_ptr<Sample> sample;
        ULONGLONG nextSearch = 0;
        ULONGLONG nextBluetoothSearch = 0;
        ULONGLONG started = 0;
        while (!stop.stop_requested()) {
            const auto now = GetTickCount64();
            // USB優先で1秒ごとに検出する
            if (usb.file == INVALID_HANDLE_VALUE && now >= nextSearch) {
                nextSearch = now + 1000;
                // 起動後にBluetoothを有効化した場合も探索を再開する
                if (watcher && !device) {
                    try {
                        const auto status = watcher.Status();
                        if (status == BluetoothLEAdvertisementWatcherStatus::Aborted ||
                            status == BluetoothLEAdvertisementWatcherStatus::Stopped) watcher.Start();
                    } catch (winrt::hresult_error const&) {}
                }
                if (usb.Open(slot)) {
                    closeBluetooth();
                    sample = std::make_shared<Sample>();
                    started = now;
                    Publish(sample);
                }
            }
            if (usb.file != INVALID_HANDLE_VALUE) {
                bool fresh;
                {
                    std::lock_guard lock(sample->mutex);
                    fresh = GetTickCount64() - (sample->lastReport ? sample->lastReport : started) < 5000;
                }
                if (!usb.Read(*sample) || !fresh) {
                    usb.Close();
                    Publish(nullptr);
                    sample.reset();
                }
                continue;
            }

            try {
                if (device) {
                    bool fresh;
                    {
                        std::lock_guard lock(sample->mutex);
                        fresh = GetTickCount64() - (sample->lastReport ? sample->lastReport : started) < 5000;
                    }
                    if (device.ConnectionStatus() == BluetoothConnectionStatus::Disconnected || !fresh) {
                        closeBluetooth();
                        Publish(nullptr);
                        sample.reset();
                    }
                } else if (const auto target = now >= nextBluetoothSearch ? address->exchange(0) : 0; target != 0) {
                    // OSペアリングを行わず、SYNC広告からGATTサービスへ接続する
                    auto connect = BluetoothLEDevice::FromBluetoothAddressAsync(target);
                    if (connect.wait_for(2s) != AsyncStatus::Completed) { connect.Cancel(); throw winrt::hresult_error(E_FAIL); }
                    device = connect.GetResults();
                    if (!device) throw winrt::hresult_error(E_FAIL);
                    // 対応OSでは低遅延を要求し、Windows 10などでは標準間隔を使う
                    try {
                        if (device.try_as<IBluetoothLEDevice6>()) {
                            connectionParameters = device.RequestPreferredConnectionParameters(
                                BluetoothLEPreferredConnectionParameters::ThroughputOptimized());
                        }
                    } catch (winrt::hresult_error const&) {}
                    auto services = device.GetGattServicesForUuidAsync(InputService, BluetoothCacheMode::Uncached);
                    if (services.wait_for(2s) != AsyncStatus::Completed) { services.Cancel(); throw winrt::hresult_error(E_FAIL); }
                    auto found = services.GetResults();
                    if (found.Status() != GattCommunicationStatus::Success || found.Services().Size() == 0) throw winrt::hresult_error(E_FAIL);
                    service = found.Services().GetAt(0);
                    auto characteristics = service.GetCharacteristicsForUuidAsync(InputCharacteristic, BluetoothCacheMode::Uncached);
                    if (characteristics.wait_for(2s) != AsyncStatus::Completed) { characteristics.Cancel(); throw winrt::hresult_error(E_FAIL); }
                    auto entries = characteristics.GetResults();
                    if (entries.Status() != GattCommunicationStatus::Success || entries.Characteristics().Size() == 0) throw winrt::hresult_error(E_FAIL);
                    input = entries.Characteristics().GetAt(0);
                    sample = std::make_shared<Sample>();
                    notification = input.ValueChanged(winrt::auto_revoke, [sample = sample](auto const&, auto const& args) {
                        auto data = args.CharacteristicValue();
                        sample->Receive({data.data(), data.Length()});
                    });
                    auto subscribe = input.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
                    if (subscribe.wait_for(2s) != AsyncStatus::Completed) { subscribe.Cancel(); throw winrt::hresult_error(E_FAIL); }
                    if (subscribe.GetResults() != GattCommunicationStatus::Success) throw winrt::hresult_error(E_FAIL);
                    started = GetTickCount64();
                    Publish(sample);
                }
            } catch (winrt::hresult_error const&) {
                closeBluetooth();
                Publish(nullptr);
                sample.reset();
                nextBluetoothSearch = GetTickCount64() + 2000;
                OutputDebugStringW(L"Switch 2 Pro: Bluetooth connection failed; retry SYNC\n");
            }
            std::this_thread::sleep_for(50ms);
        }
        // 通知解除後も実行中コールバックのSampleは共有所有で生存する
        closeBluetooth();
        discovery.revoke();
        if (watcher) { try { watcher.Stop(); } catch (winrt::hresult_error const&) {} }
        watcher = nullptr;
        if (apartment) winrt::uninit_apartment();
    }

public:
    /** @brief 入力受信スレッドを開始する @param index 直接接続スロット0または1 @return なし */
    explicit NativeController(int index) : slot(index), worker([this](std::stop_token stop) { Run(stop); }) {}

    /**
     * @brief 接続単位の入力をゲームスレッドへコピーする
     * @param state 出力する入力状態
     * @param connection 接続番号の出力先
     * @return 受信から500ms以内の入力がある場合はtrue
     */
    bool Poll(XINPUT_GAMEPAD& state, unsigned& connection) {
        std::lock_guard lock(mutex);
        connection = generation;
        if (!current) return false;
        std::lock_guard sampleLock(current->mutex);
        if (!current->received || GetTickCount64() - current->received > 500) return false;
        state = current->state;
        return true;
    }
};
}

/**
 * @brief 非同期で取得したSwitch 2 Proの入力を読み出す
 * @param state 最新入力の出力先
 * @param connection 接続を識別する通し番号の出力先
 * @param slot 直接接続スロット0または1
 * @return 有効な接続と新鮮な入力がある場合はtrue
 */
bool Switch2ProInput::Poll(XINPUT_GAMEPAD& state, unsigned& connection, int slot) {
    if (slot < 0 || slot >= 2) return false;
    static NativeController controllers[] {NativeController(0), NativeController(1)};
    return controllers[slot].Poll(state, connection);
}
