/**
 * @file GamepadSystem.cpp
 * @brief ゲームパッド入力管理システムの実装
 * @author 山内陽
 * @date2025
 * @version6.0
 */

#include "input/GamepadSystem.h"
#include "app/DebugLog.h"
#include "app/ServiceLocator.h"
#include <wbemidl.h>
#include <oleauto.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <sstream>
#include <chrono>
#include <comdef.h>
#include <algorithm>
#include <iomanip>
#include <vector>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

#if !defined(GAMEPAD_TRACE_LOG)
#if defined(ENABLE_VERBOSE_INPUT_LOG) && ENABLE_VERBOSE_INPUT_LOG
#define GAMEPAD_TRACE_LOG(message) DEBUGLOG(message)
#define GAMEPAD_TRACE_CATEGORY(category, message) DEBUGLOG_CATEGORY(category, message)
#else
#define GAMEPAD_TRACE_LOG(message) ((void)0)
#define GAMEPAD_TRACE_CATEGORY(category, message) ((void)0)
#endif
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)     \
    {                       \
        if (p) {            \
            (p)->Release(); \
            (p) = nullptr;  \
        }                   \
    }
#endif

namespace {
struct ForceFeedbackAxisContext {
    DWORD axes[2]{};
    int count = 0;
};

std::string GuidToHexString(const GUID &guid) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0')
        << '{'
        << std::setw(8) << static_cast<unsigned long>(guid.Data1) << '-'
        << std::setw(4) << static_cast<unsigned int>(guid.Data2) << '-'
        << std::setw(4) << static_cast<unsigned int>(guid.Data3) << '-'
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[0])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[1]) << '-'
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[2])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[3])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[4])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[5])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[6])
        << std::setw(2) << static_cast<unsigned int>(guid.Data4[7])
        << '}';
    return oss.str();
}

unsigned short ProductVid(const GUID &guid) {
    return LOWORD(guid.Data1);
}

unsigned short ProductPid(const GUID &guid) {
    return HIWORD(guid.Data1);
}

bool IsDualSensePid(unsigned short pid) {
    return (pid == 0x0CE6) || (pid == 0x0DF2);
}

bool IsGamepadUsage(USHORT usagePage, USHORT usage) {
    return usagePage == 0x01 && (usage == 0x04 || usage == 0x05);
}

uint32_t ComputeCrc32(const unsigned char *data, size_t size) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= static_cast<uint32_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = static_cast<uint32_t>(-(static_cast<int>(crc & 1u)));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

HANDLE OpenDualSenseHidHandle(unsigned short vid, unsigned short pid) {
    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO deviceInfo = SetupDiGetClassDevs(
        &hidGuid,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE foundHandle = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA interfaceData{};
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &hidGuid, index, &interfaceData); ++index) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0) {
            continue;
        }

        std::vector<unsigned char> detailBuffer(requiredSize);
        auto *detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(detailBuffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, requiredSize, nullptr, nullptr)) {
            continue;
        }

        HANDLE handle = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            continue;
        }

        HIDD_ATTRIBUTES attributes{};
        attributes.Size = sizeof(HIDD_ATTRIBUTES);
        if (!HidD_GetAttributes(handle, &attributes)) {
            CloseHandle(handle);
            continue;
        }
        if (attributes.VendorID != vid || attributes.ProductID != pid) {
            CloseHandle(handle);
            continue;
        }

        PHIDP_PREPARSED_DATA preparsedData = nullptr;
        if (!HidD_GetPreparsedData(handle, &preparsedData) || preparsedData == nullptr) {
            CloseHandle(handle);
            continue;
        }
        HIDP_CAPS caps{};
        const NTSTATUS capsStatus = HidP_GetCaps(preparsedData, &caps);
        HidD_FreePreparsedData(preparsedData);
        if (capsStatus != HIDP_STATUS_SUCCESS || !IsGamepadUsage(caps.UsagePage, caps.Usage) || caps.OutputReportByteLength < 5) {
            CloseHandle(handle);
            continue;
        }

        foundHandle = handle;
        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);
    return foundHandle;
}

bool SendDualSenseHidVibration(HANDLE handle, float leftMotor, float rightMotor, DWORD *lastErrorOut) {
    if (lastErrorOut) {
        *lastErrorOut = ERROR_SUCCESS;
    }
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        if (lastErrorOut) {
            *lastErrorOut = ERROR_INVALID_HANDLE;
        }
        return false;
    }

    PHIDP_PREPARSED_DATA preparsedData = nullptr;
    if (!HidD_GetPreparsedData(handle, &preparsedData) || preparsedData == nullptr) {
        if (lastErrorOut) {
            *lastErrorOut = GetLastError();
        }
        return false;
    }

    HIDP_CAPS caps{};
    const NTSTATUS capsStatus = HidP_GetCaps(preparsedData, &caps);
    HidD_FreePreparsedData(preparsedData);
    if (capsStatus != HIDP_STATUS_SUCCESS || caps.OutputReportByteLength < 5) {
        if (lastErrorOut) {
            *lastErrorOut = ERROR_NOT_SUPPORTED;
        }
        return false;
    }

    const unsigned char weakMotor = static_cast<unsigned char>(std::clamp(rightMotor, 0.0f, 1.0f) * 255.0f);
    const unsigned char strongMotor = static_cast<unsigned char>(std::clamp(leftMotor, 0.0f, 1.0f) * 255.0f);

    std::vector<unsigned char> report(static_cast<size_t>(caps.OutputReportByteLength), 0);
    if (report.size() >= 78) {
        report[0] = 0x31;
        report[1] = 0x02;
        report[2] = 0x03;
        report[3] = 0x00;
        report[4] = 0x00;
        report[5] = weakMotor;
        report[6] = strongMotor;

        std::vector<unsigned char> crcInput((report.size() - 4) + 1, 0);
        crcInput[0] = 0xA2;
        memcpy(crcInput.data() + 1, report.data(), report.size() - 4);
        const uint32_t crc = ComputeCrc32(crcInput.data(), crcInput.size());
        report[report.size() - 4] = static_cast<unsigned char>(crc & 0xFFu);
        report[report.size() - 3] = static_cast<unsigned char>((crc >> 8) & 0xFFu);
        report[report.size() - 2] = static_cast<unsigned char>((crc >> 16) & 0xFFu);
        report[report.size() - 1] = static_cast<unsigned char>((crc >> 24) & 0xFFu);
    } else {
        report[0] = 0x02;
        report[1] = 0x03;
        report[2] = 0x00;
        report[3] = weakMotor;
        report[4] = strongMotor;
    }

    DWORD written = 0;
    if (WriteFile(handle, report.data(), static_cast<DWORD>(report.size()), &written, nullptr) == TRUE) {
        return true;
    }
    const DWORD writeError = GetLastError();

    if (HidD_SetOutputReport(handle, report.data(), static_cast<ULONG>(report.size())) == TRUE) {
        return true;
    }

    if (lastErrorOut) {
        const DWORD hidError = GetLastError();
        *lastErrorOut = (hidError != ERROR_SUCCESS) ? hidError : writeError;
    }
    return false;
}

BOOL CALLBACK EnumForceFeedbackAxesCallback(const DIDEVICEOBJECTINSTANCE *lpddoi, LPVOID pvRef) {
    if (!lpddoi || !pvRef) {
        return DIENUM_STOP;
    }
    auto *context = static_cast<ForceFeedbackAxisContext *>(pvRef);
    if (context->count >= 2) {
        return DIENUM_STOP;
    }
    context->axes[context->count++] = lpddoi->dwType;
    return (context->count < 2) ? DIENUM_CONTINUE : DIENUM_STOP;
}
} // namespace

// ========================================================
// コンストラクタ / デストラクタ
// ========================================================

GamepadSystem::GamepadSystem()
    : dinput_(nullptr), windowHandle_(nullptr), nextDInputSlot_(0), deltaTime_(0.0f) {
}

GamepadSystem::~GamepadSystem() {
    Shutdown();
}

// ========================================================
// 初期化 / シャットダウン
// ========================================================

bool GamepadSystem::Init() {
#ifdef _DEBUG
    GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, "GamepadSystem::Init() - 初期化開始");
#endif

    //既存状態をリセット
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        gamepads_[i] = GamepadState();
    }

    // DirectInput8の作成
    HRESULT hr = DirectInput8Create(
        GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (LPVOID *) &dinput_,
        nullptr);

    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::Init() - DirectInput8の作成に失敗: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        return false;
    }

    auto enumerateDInput = [&](DWORD deviceClass) {
        return dinput_->EnumDevices(
            deviceClass,
            EnumDevicesCallback,
            this,
            DIEDFL_ATTACHEDONLY);
    };

    // DirectInputデバイスを列挙（通常クラス）
    hr = enumerateDInput(DI8DEVCLASS_GAMECTRL);

    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::Init() - デバイス列挙に失敗: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        return false;
    }

    int dinputCount = 0;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice != nullptr) {
            ++dinputCount;
        }
    }

    // 通常クラスで見つからない場合は全クラス列挙を試す
    if (dinputCount == 0) {
        HRESULT allEnumHr = enumerateDInput(DI8DEVCLASS_ALL);
        if (FAILED(allEnumHr)) {
            std::ostringstream oss;
            oss << "GamepadSystem::Init() - 全クラス列挙に失敗: HRESULT=0x" << std::hex << allEnumHr;
            DEBUGLOG_WARNING(oss.str());
        }

        dinputCount = 0;
        for (int i = 0; i < MAX_GAMEPADS; ++i) {
            if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice != nullptr) {
                ++dinputCount;
            }
        }
    }

#ifdef _DEBUG
    {
        std::ostringstream oss;
        oss << "GamepadSystem::Init() - DInput検出数: " << dinputCount;
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, oss.str());
    }
    if (dinputCount == 0) {
        DEBUGLOG_WARNING("GamepadSystem::Init() - DInput検出数が0です。EnumDeviceログのdwDevType/GUIDを確認してください。");
    }
#endif

#ifdef _DEBUG
    GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, "GamepadSystem::Init() - 初期化完了");
#endif

    return true;
}

void GamepadSystem::Shutdown() {
#ifdef _DEBUG
    GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, "GamepadSystem::Shutdown() - シャットダウン開始");
#endif

    //すべてのDirectInputデバイスを解放
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].dinputEffect) {
            gamepads_[i].dinputEffect->Stop();
            SAFE_RELEASE(gamepads_[i].dinputEffect);
        }
        if (gamepads_[i].dinputDevice) {
            gamepads_[i].dinputDevice->Unacquire();
            SAFE_RELEASE(gamepads_[i].dinputDevice);
        }
        if (gamepads_[i].dualSenseHidHandle != INVALID_HANDLE_VALUE && gamepads_[i].dualSenseHidHandle != nullptr) {
            CloseHandle(gamepads_[i].dualSenseHidHandle);
            gamepads_[i].dualSenseHidHandle = INVALID_HANDLE_VALUE;
        }
        gamepads_[i] = GamepadState();
    }

    // DirectInputを解放
    SAFE_RELEASE(dinput_);
    nextDInputSlot_ = 0;

#ifdef _DEBUG
    GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, "GamepadSystem::Shutdown() - シャットダウン完了");
#endif
}

// ========================================================
// 更新処理
// ========================================================

void GamepadSystem::Update() {
#ifdef _DEBUG
    static int frameCounter = 0;
    static int logInterval = 300; //5秒ごと(60FPS想定)
    bool shouldLog = (frameCounter % logInterval == 0);

    if (shouldLog) {
        GAMEPAD_TRACE_LOG("GamepadSystem::Update - Detailed Status Check");
    }
    frameCounter++;
#endif

    // デルタタイムを計算(簡易版:60FPS想定)
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - lastTime;
    deltaTime_ = elapsed.count();
    lastTime = currentTime;

    // DirectInput優先: DirectInputが占有済みのスロットはXInputで上書きしない
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (i >= MAX_GAMEPADS) {
            break;
        }
        if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice != nullptr) {
#ifdef _DEBUG
            if (shouldLog) {
                std::ostringstream oss;
                oss << " XInput Slot " << i << ": SKIPPED (DirectInput prioritized)";
                GAMEPAD_TRACE_LOG(oss.str());
            }
#endif
            continue;
        }

        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        DWORD result = XInputGetState(i, &state);

#ifdef _DEBUG
        if (shouldLog) {
            std::ostringstream oss;
            oss << " XInput Slot " << i << ": ";
            if (result == ERROR_SUCCESS) {
                oss << "CONNECTED (PacketNumber=" << state.dwPacketNumber << ")";
            } else if (result == ERROR_DEVICE_NOT_CONNECTED) {
                oss << "NOT_CONNECTED";
            } else {
                oss << "ERROR (DWORD=" << result << ")";
            }
            GAMEPAD_TRACE_LOG(oss.str());
        }
#endif

        if (result == ERROR_SUCCESS) {
            if (gamepads_[i].type != Type_XInput) {
                gamepads_[i] = GamepadState();
                gamepads_[i].type = Type_XInput;
                gamepads_[i].connected = true;
                gamepads_[i].xinputIndex = i;
#ifdef _DEBUG
                std::ostringstream oss;
                oss << "GamepadSystem::Update - XInput device connected: Index=" << i;
                GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, oss.str());
#endif
            }
            UpdateXInput(static_cast<int>(i));
            UpdateChargeSystem(static_cast<int>(i), deltaTime_);
        } else {
            if (gamepads_[i].type == Type_XInput) {
#ifdef _DEBUG
                std::ostringstream oss;
                oss << "GamepadSystem::Update - XInput device disconnected: Index=" << i;
                GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, oss.str());
#endif
                gamepads_[i] = GamepadState();
            }
        }
    }

    bool hasDInputDevice = false;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice) {
            hasDInputDevice = true;
            break;
        }
    }

    if (!hasDInputDevice && dinput_) {
        static float dinputReenumCooldown = 0.0f;
        dinputReenumCooldown -= deltaTime_;
        if (dinputReenumCooldown <= 0.0f) {
            dinputReenumCooldown = 1.0f;
            auto reenumDInput = [&](DWORD deviceClass) {
                return dinput_->EnumDevices(
                    deviceClass,
                    EnumDevicesCallback,
                    this,
                    DIEDFL_ATTACHEDONLY);
            };

            HRESULT enumHr = reenumDInput(DI8DEVCLASS_GAMECTRL);
            bool foundAfterGameCtrlEnum = false;
            for (int i = 0; i < MAX_GAMEPADS; ++i) {
                if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice != nullptr) {
                    foundAfterGameCtrlEnum = true;
                    break;
                }
            }
            if (!foundAfterGameCtrlEnum) {
                HRESULT allEnumHr = reenumDInput(DI8DEVCLASS_ALL);
                if (FAILED(allEnumHr)) {
                    enumHr = allEnumHr;
                }
            }
#ifdef _DEBUG
            if (FAILED(enumHr)) {
                std::ostringstream oss;
                oss << "GamepadSystem::Update - DirectInput再列挙に失敗: HRESULT=0x" << std::hex << enumHr;
                DEBUGLOG_WARNING(oss.str());
            }
#endif
        }
    }

    // DirectInputデバイスを更新（全スロットをチェック）
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        // Type_DInputのものだけを更新（XInputとの競合を避ける）
        if (gamepads_[i].type == Type_DInput && gamepads_[i].dinputDevice) {
#ifdef _DEBUG
            if (shouldLog) {
                std::ostringstream oss;
                oss << " DInput Slot " << i << ": CONNECTED";
                oss << " (LX=" << gamepads_[i].leftStickX
                    << " LY=" << gamepads_[i].leftStickY
                    << " RX=" << gamepads_[i].rightStickX
                    << " RY=" << gamepads_[i].rightStickY << ")";
                GAMEPAD_TRACE_LOG(oss.str());
            }
#endif
            UpdateDInput(i);
            UpdateChargeSystem(i, deltaTime_);
        }
    }
}

void GamepadSystem::UpdateXInput(int index) {
    if (index < 0 || index >= MAX_GAMEPADS)
        return;

    GamepadState &pad = gamepads_[index];
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    DWORD result = XInputGetState(pad.xinputIndex, &state);
    if (result != ERROR_SUCCESS) {
        pad.connected = false;
        return;
    }

    pad.connected = true;

    // 前フレームの状態を保存
    memcpy(pad.prevButtons, pad.buttons, sizeof(pad.buttons));

    // ボタン状態を更新
    const WORD buttons = state.Gamepad.wButtons;

    auto updateButton = [&](GamepadButton btn, WORD xinputBtn) {
        bool isDown = (buttons & xinputBtn) != 0;
        ButtonState prevState = static_cast<ButtonState>(pad.prevButtons[btn]);
        bool wasDown = (prevState == Down || prevState == Pressed);

        if (isDown) {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Pressed : Down);
        } else {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Up : None);
        }
    };

    updateButton(Button_A, XINPUT_GAMEPAD_A);
    updateButton(Button_B, XINPUT_GAMEPAD_B);
    updateButton(Button_X, XINPUT_GAMEPAD_X);
    updateButton(Button_Y, XINPUT_GAMEPAD_Y);
    updateButton(Button_LB, XINPUT_GAMEPAD_LEFT_SHOULDER);
    updateButton(Button_RB, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    updateButton(Button_Back, XINPUT_GAMEPAD_BACK);
    updateButton(Button_Start, XINPUT_GAMEPAD_START);
    updateButton(Button_LS, XINPUT_GAMEPAD_LEFT_THUMB);
    updateButton(Button_RS, XINPUT_GAMEPAD_RIGHT_THUMB);
    updateButton(Button_DPad_Up, XINPUT_GAMEPAD_DPAD_UP);
    updateButton(Button_DPad_Down, XINPUT_GAMEPAD_DPAD_DOWN);
    updateButton(Button_DPad_Left, XINPUT_GAMEPAD_DPAD_LEFT);
    updateButton(Button_DPad_Right, XINPUT_GAMEPAD_DPAD_RIGHT);
    {
        bool backDown = (buttons & XINPUT_GAMEPAD_BACK) != 0;
        bool startDown = (buttons & XINPUT_GAMEPAD_START) != 0;
        bool optionDown = startDown || backDown;
        ButtonState prevState = static_cast<ButtonState>(pad.prevButtons[Button_Option]);
        bool wasDown = (prevState == Down || prevState == Pressed);
        if (optionDown) {
            pad.buttons[Button_Option] = static_cast<uint8_t>(wasDown ? Pressed : Down);
        } else {
            pad.buttons[Button_Option] = static_cast<uint8_t>(wasDown ? Up : None);
        }
    }

    // スティック値を正規化してデッドゾーン適用
    float rawLeftX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
    float rawLeftY = static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;
    float rawRightX = static_cast<float>(state.Gamepad.sThumbRX) / 32767.0f;
    float rawRightY = static_cast<float>(state.Gamepad.sThumbRY) / 32767.0f;

    ApplyDeadzone(rawLeftX, rawLeftY, pad.leftStickX, pad.leftStickY, XINPUT_LEFT_DEADZONE);
    ApplyDeadzone(rawRightX, rawRightY, pad.rightStickX, pad.rightStickY, XINPUT_RIGHT_DEADZONE);

    // トリガー値を正規化
    pad.leftTrigger = static_cast<float>(state.Gamepad.bLeftTrigger) / 255.0f;
    pad.rightTrigger = static_cast<float>(state.Gamepad.bRightTrigger) / 255.0f;

    // トリガー閾値適用
    if (pad.leftTrigger < XINPUT_TRIGGER_THRESHOLD)
        pad.leftTrigger = 0.0f;
    if (pad.rightTrigger < XINPUT_TRIGGER_THRESHOLD)
        pad.rightTrigger = 0.0f;
}

void GamepadSystem::UpdateDInput(int index) {
    if (index < 0 || index >= MAX_GAMEPADS)
        return;

    GamepadState &pad = gamepads_[index];
    if (!pad.dinputDevice)
        return;
    auto releaseDInputSlot = [&pad]() {
        if (pad.dinputEffect) {
            pad.dinputEffect->Stop();
            SAFE_RELEASE(pad.dinputEffect);
        }
        if (pad.dinputDevice) {
            pad.dinputDevice->Unacquire();
            SAFE_RELEASE(pad.dinputDevice);
        }
        if (pad.dualSenseHidHandle != INVALID_HANDLE_VALUE && pad.dualSenseHidHandle != nullptr) {
            CloseHandle(pad.dualSenseHidHandle);
            pad.dualSenseHidHandle = INVALID_HANDLE_VALUE;
        }
        pad = GamepadState();
    };
#ifdef _DEBUG
    static int acquireFailureLogCount = 0;
    static int stateFailureLogCount = 0;
#endif

    // デバイスを取得
    HRESULT hr = pad.dinputDevice->Poll();
    if (FAILED(hr)) {
        HRESULT acquireHr = pad.dinputDevice->Acquire();
        if (FAILED(acquireHr)) {
#ifdef _DEBUG
            if (acquireFailureLogCount < 20) {
                std::ostringstream oss;
                oss << "DInput: Failed to Acquire device, HRESULT=0x" << std::hex << acquireHr;
                DEBUGLOG_WARNING(oss.str());
                acquireFailureLogCount++;
            }
#endif
            if (acquireHr == DIERR_DEVICENOTREG || acquireHr == DIERR_NOTINITIALIZED || acquireHr == HRESULT_FROM_WIN32(ERROR_READ_FAULT)) {
                releaseDInputSlot();
                return;
            }
            pad.connected = false;
            return;
        }
        hr = pad.dinputDevice->Poll();
        if (FAILED(hr)) {
#ifdef _DEBUG
            if (stateFailureLogCount < 20) {
                std::ostringstream oss;
                oss << "DInput: Failed to Poll after Acquire, HRESULT=0x" << std::hex << hr;
                DEBUGLOG_WARNING(oss.str());
                stateFailureLogCount++;
            }
#endif
            pad.connected = false;
            return;
        }
    }

    DIJOYSTATE2 js{};
    hr = pad.dinputDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);
    if (FAILED(hr) && (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)) {
        HRESULT reacquireHr = pad.dinputDevice->Acquire();
        if (SUCCEEDED(reacquireHr)) {
            hr = pad.dinputDevice->Poll();
            if (SUCCEEDED(hr)) {
                hr = pad.dinputDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);
            }
        } else {
#ifdef _DEBUG
            if (acquireFailureLogCount < 20) {
                std::ostringstream oss;
                oss << "DInput: Re-acquire failed after lost state, HRESULT=0x" << std::hex << reacquireHr;
                DEBUGLOG_WARNING(oss.str());
                acquireFailureLogCount++;
            }
#endif
        }
    }
    if (FAILED(hr)) {
#ifdef _DEBUG
        if (stateFailureLogCount < 20) {
            std::ostringstream oss;
            oss << "DInput: Failed to GetDeviceState, HRESULT=0x" << std::hex << hr;
            DEBUGLOG_WARNING(oss.str());
            stateFailureLogCount++;
        }
#endif
        if (hr == DIERR_DEVICENOTREG || hr == DIERR_NOTINITIALIZED || hr == HRESULT_FROM_WIN32(ERROR_READ_FAULT)) {
            releaseDInputSlot();
            return;
        }
        pad.connected = false;
        return;
    }

    pad.connected = true;

#ifdef _DEBUG
    static int debugFrameCounter = 0;
    static int debugInterval = 60; //1秒ごと
    bool shouldDebugLog = (debugFrameCounter % debugInterval == 0);
    debugFrameCounter++;

    if (shouldDebugLog) {
        // DirectInputの生入力値をログ出力
        std::ostringstream oss;
        oss << "DInput[" << index << "] RAW INPUT: ";
        oss << "LX=" << js.lX << ", LY=" << js.lY;
        oss << ", RX=" << js.lRx << ", RY=" << js.lRy;
        oss << ", Z=" << js.lZ << ", Rz=" << js.lRz;
        oss << ", Buttons[0]=" << (int) (js.rgbButtons[0] & 0x80);
        oss << ", POV[0]=" << js.rgdwPOV[0];
        GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, oss.str());
    }
#endif

    // 前フレームの状態を保存
    memcpy(pad.prevButtons, pad.buttons, sizeof(pad.buttons));

#if defined(ENABLE_VERBOSE_INPUT_LOG) && ENABLE_VERBOSE_INPUT_LOG
    // 入力値を詳細にログ出力
    {
        std::ostringstream oss;
        oss << "DInput[" << index << "] RAW INPUT: ";
        oss << "Buttons=0x" << std::hex;
        for (int i = 0; i < 32; ++i) {
            oss << ((js.rgbButtons[i] & 0x80) ? '1' : '0');
        }
        oss << ", LX=" << js.lX;
        oss << ", LY=" << js.lY;
        oss << ", RX=" << js.lRx;
        oss << ", RY=" << js.lRy;
        oss << ", LT=" << (int) js.lZ;
        oss << ", RT=" << (int) js.lRz;
        GAMEPAD_TRACE_CATEGORY(DebugLog::Category::Input, oss.str());
    }
#endif

    // ボタン状態を更新(最大14ボタンをサポート)
    auto updateButton = [&](GamepadButton btn, int dinputBtn) {
        if (dinputBtn >= 128)
            return; // 範囲外

        bool isDown = (js.rgbButtons[dinputBtn] & 0x80) != 0;
        ButtonState prevState = static_cast<ButtonState>(pad.prevButtons[btn]);
        bool wasDown = (prevState == Down || prevState == Pressed);

        if (isDown) {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Pressed : Down);
        } else {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Up : None);
        }
    };

    // 一般的なボタンマッピング(Xbox配置を想定)
    updateButton(Button_X, 0);  // Square → X
    updateButton(Button_B, 1);  // Cross  → B
    updateButton(Button_A, 2);  // Circle → A
    updateButton(Button_Y, 3);  // Triangle → Y
    updateButton(Button_LB, 4);
    updateButton(Button_RB, 5);
    
    // DirectInput Standard Mapping (PS4/PS5/Logitech)
    // 8: Share/Back, 9: Options/Start, 10: L3, 11: R3
    updateButton(Button_Back, 8);
    updateButton(Button_Start, 9);
    updateButton(Button_LS, 10);
    updateButton(Button_RS, 11);

    // POV(十字キー)の処理
    bool dpadUp = false, dpadDown = false, dpadLeft = false, dpadRight = false;
    if (js.rgdwPOV[0] != 0xFFFFFFFF) {
        DWORD pov = js.rgdwPOV[0];
        dpadUp = (pov >= 31500 || pov <= 4500);
        dpadRight = (pov >= 4500 && pov <= 13500);
        dpadDown = (pov >= 13500 && pov <= 22500);
        dpadLeft = (pov >= 22500 && pov <= 31500);
    }

    auto updateDPad = [&](GamepadButton btn, bool isDown) {
        ButtonState prevState = static_cast<ButtonState>(pad.prevButtons[btn]);
        bool wasDown = (prevState == Down || prevState == Pressed);

        if (isDown) {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Pressed : Down);
        } else {
            pad.buttons[btn] = static_cast<uint8_t>(wasDown ? Up : None);
        }
    };

    updateDPad(Button_DPad_Up, dpadUp);
    updateDPad(Button_DPad_Down, dpadDown);
    updateDPad(Button_DPad_Left, dpadLeft);
    updateDPad(Button_DPad_Right, dpadRight);

    {
        // 従来の6/7に加えて、標準的な8/9(Share/Option)、さらに12(PSボタン)もチェック
        bool btn6 = (js.rgbButtons[6] & 0x80) != 0;
        bool btn7 = (js.rgbButtons[7] & 0x80) != 0;
        bool btn8 = (js.rgbButtons[8] & 0x80) != 0; // Share
        bool btn9 = (js.rgbButtons[9] & 0x80) != 0; // Options
        bool btn12 = (js.rgbButtons[12] & 0x80) != 0; // PS Button

        bool backDown = btn6 || btn8;
        bool startDown = btn7 || btn9;
        
        bool optionDown = startDown || backDown || btn12;

        ButtonState prevState = static_cast<ButtonState>(pad.prevButtons[Button_Option]);
        bool wasDown = (prevState == Down || prevState == Pressed);
        if (optionDown) {
            pad.buttons[Button_Option] = static_cast<uint8_t>(wasDown ? Pressed : Down);
        } else {
            pad.buttons[Button_Option] = static_cast<uint8_t>(wasDown ? Up : None);
        }
    }

    // スティック値を正規化(-1.0 ～ +1.0)
    // DirectInputの軸範囲:0～65535、中央値:32767
    static constexpr float DINPUT_RANGE = 32767.5f;

    float rawLeftX = (static_cast<float>(js.lX) - DINPUT_RANGE) / DINPUT_RANGE;
    float rawLeftY = -(static_cast<float>(js.lY) - DINPUT_RANGE) / DINPUT_RANGE; // Y軸反転
    float rawRightX = (static_cast<float>(js.lRx) - DINPUT_RANGE) / DINPUT_RANGE;
    float rawRightY = -(static_cast<float>(js.lRy) - DINPUT_RANGE) / DINPUT_RANGE; // Y軸反転

    // DirectInput用のデッドゾーン（XInputと同じ物理的範囲を使用）
    static constexpr float DINPUT_LEFT_DEADZONE = XINPUT_LEFT_DEADZONE;
    static constexpr float DINPUT_RIGHT_DEADZONE = XINPUT_RIGHT_DEADZONE;

    ApplyDeadzone(rawLeftX, rawLeftY, pad.leftStickX, pad.leftStickY, DINPUT_LEFT_DEADZONE);
    ApplyDeadzone(rawRightX, rawRightY, pad.rightStickX, pad.rightStickY, DINPUT_RIGHT_DEADZONE);

    // トリガー値(Z軸とZ回転軸を使用、範囲:0～65535)
    pad.leftTrigger = static_cast<float>(js.lZ) / 65535.0f;
    pad.rightTrigger = static_cast<float>(js.lRz) / 65535.0f;

    if (pad.leftTrigger < XINPUT_TRIGGER_THRESHOLD)
        pad.leftTrigger = 0.0f;
    if (pad.rightTrigger < XINPUT_TRIGGER_THRESHOLD)
        pad.rightTrigger = 0.0f;
}

void GamepadSystem::UpdateChargeSystem(int index, float dt) {
    if (index < 0 || index >= MAX_GAMEPADS)
        return;
    if (!gamepads_[index].connected)
        return;

    GamepadState &pad = gamepads_[index];

    // 左スティックのチャージ判定
    float leftMagnitude = sqrtf(pad.leftStickX * pad.leftStickX + pad.leftStickY * pad.leftStickY);
    bool leftCharging = leftMagnitude > CHARGE_DETECTION_THRESHOLD;
    bool wasLeftCharging = pad.leftStickWasCharging;
    pad.leftStickJustReleased = false;

    if (leftCharging) {
        // チャージ中
        pad.leftStickChargeTime += dt;
        pad.leftStickIntensitySum += leftMagnitude;
        pad.leftStickChargeSamples++;
        pad.leftStickWasCharging = true;
    } else {
        // ニュートラル
        if (wasLeftCharging) {
            pad.leftStickJustReleased = true;
            pad.leftStickChargeTime = 0.0f;
            pad.leftStickIntensitySum = 0.0f;
            pad.leftStickChargeSamples = 0;
        } else {
            pad.leftStickChargeTime = 0.0f;
            pad.leftStickIntensitySum = 0.0f;
            pad.leftStickChargeSamples = 0;
        }
        pad.leftStickWasCharging = false;
    }

    //右スティックのチャージ判定
    float rightMagnitude = sqrtf(pad.rightStickX * pad.rightStickX + pad.rightStickY * pad.rightStickY);
    bool rightCharging = rightMagnitude > CHARGE_DETECTION_THRESHOLD;
    bool wasRightCharging = pad.rightStickWasCharging;
    pad.rightStickJustReleased = false;

    if (rightCharging) {
        // チャージ中
        pad.rightStickChargeTime += dt;
        pad.rightStickIntensitySum += rightMagnitude;
        pad.rightStickChargeSamples++;
        pad.rightStickWasCharging = true;
    } else {
        // ニュートラル
        if (wasRightCharging) {
            pad.rightStickJustReleased = true;
            pad.rightStickChargeTime = 0.0f;
            pad.rightStickIntensitySum = 0.0f;
            pad.rightStickChargeSamples = 0;
        } else {
            pad.rightStickChargeTime = 0.0f;
            pad.rightStickIntensitySum = 0.0f;
            pad.rightStickChargeSamples = 0;
        }
        pad.rightStickWasCharging = false;
    }
}

// ========================================================
// 統合入力取得（全コントローラー対応）
// ========================================================

float GamepadSystem::GetLeftStickX() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].leftStickX;
        }
    }
    // クランプ（-1.0 ～ +1.0）
    if (combined < -1.0f)
        combined = -1.0f;
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

float GamepadSystem::GetLeftStickY() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].leftStickY;
        }
    }
    // クランプ（-1.0 ～ +1.0）
    if (combined < -1.0f)
        combined = -1.0f;
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

float GamepadSystem::GetRightStickX() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].rightStickX;
        }
    }
    // クランプ（-1.0 ～ +1.0）
    if (combined < -1.0f)
        combined = -1.0f;
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

float GamepadSystem::GetRightStickY() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].rightStickY;
        }
    }
    // クランプ（-1.0 ～ +1.0）
    if (combined < -1.0f)
        combined = -1.0f;
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

float GamepadSystem::GetLeftTrigger() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].leftTrigger;
        }
    }
    // クランプ（0.0 ～1.0）
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

float GamepadSystem::GetRightTrigger() const {
    float combined = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            combined += gamepads_[i].rightTrigger;
        }
    }
    // クランプ（0.0 ～1.0）
    if (combined > 1.0f)
        combined = 1.0f;
    return combined;
}

bool GamepadSystem::GetButton(GamepadButton button) const {
    if (button < 0 || button >= Button_Count)
        return false;

    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            ButtonState state = static_cast<ButtonState>(gamepads_[i].buttons[button]);
            if (state == Pressed || state == Down) {
                return true;
            }
        }
    }
    return false;
}

bool GamepadSystem::GetButtonDown(GamepadButton button) const {
    if (button < 0 || button >= Button_Count)
        return false;

    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            if (static_cast<ButtonState>(gamepads_[i].buttons[button]) == Down) {
                return true;
            }
        }
    }
    return false;
}

bool GamepadSystem::GetButtonUp(GamepadButton button) const {
    if (button < 0 || button >= Button_Count)
        return false;

    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            if (static_cast<ButtonState>(gamepads_[i].buttons[button]) == Up) {
                return true;
            }
        }
    }
    return false;
}

bool GamepadSystem::GetAnyButton(std::initializer_list<GamepadButton> buttons) const {
    for (auto b : buttons) {
        if (GetButton(b)) return true;
    }
    return false;
}

bool GamepadSystem::GetAnyButtonDown(std::initializer_list<GamepadButton> buttons) const {
    for (auto b : buttons) {
        if (GetButtonDown(b)) return true;
    }
    return false;
}

bool GamepadSystem::GetAnyButtonUp(std::initializer_list<GamepadButton> buttons) const {
    for (auto b : buttons) {
        if (GetButtonUp(b)) return true;
    }
    return false;
}

// ========================================================
// チャージ&リリースシステム（統合版）
// ========================================================

bool GamepadSystem::IsLeftStickCharging() const {
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            const GamepadState &pad = gamepads_[i];
            float magnitude = sqrtf(pad.leftStickX * pad.leftStickX + pad.leftStickY * pad.leftStickY);
            if (magnitude > CHARGE_DETECTION_THRESHOLD) {
                return true;
            }
        }
    }
    return false;
}

bool GamepadSystem::IsRightStickCharging() const {
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            const GamepadState &pad = gamepads_[i];
            float magnitude = sqrtf(pad.rightStickX * pad.rightStickX + pad.rightStickY * pad.rightStickY);
            if (magnitude > CHARGE_DETECTION_THRESHOLD) {
                return true;
            }
        }
    }
    return false;
}

float GamepadSystem::GetLeftStickChargeTime() const {
    float maxChargeTime = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            if (gamepads_[i].leftStickChargeTime > maxChargeTime) {
                maxChargeTime = gamepads_[i].leftStickChargeTime;
            }
        }
    }
    return maxChargeTime;
}

float GamepadSystem::GetRightStickChargeTime() const {
    float maxChargeTime = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            if (gamepads_[i].rightStickChargeTime > maxChargeTime) {
                maxChargeTime = gamepads_[i].rightStickChargeTime;
            }
        }
    }
    return maxChargeTime;
}

bool GamepadSystem::IsLeftStickReleased() const {
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            GamepadState &pad = const_cast<GamepadState &>(gamepads_[i]);
            static constexpr float BOUNCE_IGNORE_TIME = 0.05f;
            if (pad.leftStickJustReleased && pad.leftStickReleaseTimer <= 0.0f) {
                pad.leftStickReleaseTimer = BOUNCE_IGNORE_TIME;
                pad.leftStickJustReleased = false;
                return true;
            }

            if (pad.leftStickReleaseTimer > 0.0f) {
                pad.leftStickReleaseTimer -= deltaTime_;
            }
            pad.leftStickJustReleased = false;
        }
    }
    return false;
}

bool GamepadSystem::IsRightStickReleased() const {
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            GamepadState &pad = const_cast<GamepadState &>(gamepads_[i]);
            static constexpr float BOUNCE_IGNORE_TIME = 0.05f;
            if (pad.rightStickJustReleased && pad.rightStickReleaseTimer <= 0.0f) {
                pad.rightStickReleaseTimer = BOUNCE_IGNORE_TIME;
                pad.rightStickJustReleased = false;
                return true;
            }

            if (pad.rightStickReleaseTimer > 0.0f) {
                pad.rightStickReleaseTimer -= deltaTime_;
            }
            pad.rightStickJustReleased = false;
        }
    }
    return false;
}

float GamepadSystem::GetLeftStickChargeAmount(float maxChargeTime) const {
    if (maxChargeTime <= 0.0f)
        return 0.0f;

    float chargeTime = GetLeftStickChargeTime();
    float amount = chargeTime / maxChargeTime;

    //0.0 ～1.0 にクランプ
    if (amount < 0.0f)
        amount = 0.0f;
    if (amount > 1.0f)
        amount = 1.0f;

    return amount;
}

float GamepadSystem::GetRightStickChargeAmount(float maxChargeTime) const {
    if (maxChargeTime <= 0.0f)
        return 0.0f;

    float chargeTime = GetRightStickChargeTime();
    float amount = chargeTime / maxChargeTime;

    //0.0 ～1.0 にクランプ
    if (amount < 0.0f)
        amount = 0.0f;
    if (amount > 1.0f)
        amount = 1.0f;

    return amount;
}

float GamepadSystem::GetLeftStickAverageIntensity() const {
    float maxIntensity = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            const GamepadState &pad = gamepads_[i];
            if (pad.leftStickChargeSamples > 0) {
                float intensity = pad.leftStickIntensitySum / static_cast<float>(pad.leftStickChargeSamples);
                if (intensity > maxIntensity) {
                    maxIntensity = intensity;
                }
            }
        }
    }
    return maxIntensity;
}

float GamepadSystem::GetRightStickAverageIntensity() const {
    float maxIntensity = 0.0f;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (gamepads_[i].connected) {
            const GamepadState &pad = gamepads_[i];
            if (pad.rightStickChargeSamples > 0) {
                float intensity = pad.rightStickIntensitySum / static_cast<float>(pad.rightStickChargeSamples);
                if (intensity > maxIntensity) {
                    maxIntensity = intensity;
                }
            }
        }
    }
    return maxIntensity;
}

void GamepadSystem::SetVibration(float leftMotor, float rightMotor) {
    // 値を0.0-1.0にクランプ
    leftMotor = (leftMotor < 0.0f) ? 0.0f : (leftMotor > 1.0f) ? 1.0f
                                                               : leftMotor;
    rightMotor = (rightMotor < 0.0f) ? 0.0f : (rightMotor > 1.0f) ? 1.0f
                                                                  : rightMotor;

    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

    const bool stopRequested = (leftMotor <= 0.0f && rightMotor <= 0.0f);
    bool applied = false;
    DWORD xinputConnectedCount = 0;
    DWORD xinputAppliedCount = 0;
    int dinputConnectedCount = 0;
    int dinputCandidateCount = 0;
    int dualSenseHidConnectedCount = 0;
    int dualSenseHidAppliedCount = 0;
    HRESULT lastDInputAcquireHr = S_OK;
    HRESULT lastDInputSetHr = S_OK;
    DWORD lastDualSenseHidError = ERROR_SUCCESS;

    // すべてのXInputスロットに適用（内部状態に依存しない）
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            ++xinputConnectedCount;
        }
        if (XInputSetState(i, &vibration) == ERROR_SUCCESS) {
            applied = true;
            ++xinputAppliedCount;
        }
    }

    // DirectInputのフォースフィードバックへフォールバック
    const float maxMotor = std::max(leftMotor, rightMotor);
    const LONG magnitude = static_cast<LONG>(maxMotor * static_cast<float>(DI_FFNOMINALMAX));
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        GamepadState &pad = gamepads_[i];
        if (pad.type != Type_DInput) {
            continue;
        }

        if (pad.dinputDevice) {
            ++dinputConnectedCount;
        }

        if (pad.dualSenseHidRumbleSupported && pad.dualSenseHidHandle != INVALID_HANDLE_VALUE && pad.dualSenseHidHandle != nullptr) {
            ++dualSenseHidConnectedCount;
            DWORD hidError = ERROR_SUCCESS;
            if (SendDualSenseHidVibration(pad.dualSenseHidHandle, leftMotor, rightMotor, &hidError)) {
                applied = true;
                ++dualSenseHidAppliedCount;
                continue;
            }
            lastDualSenseHidError = hidError;
        }

        if (!pad.dinputForceFeedbackSupported || !pad.dinputEffect || !pad.dinputDevice) {
            continue;
        }
        ++dinputCandidateCount;

        if (stopRequested || magnitude <= 0) {
            HRESULT stopHr = pad.dinputDevice->Acquire();
            lastDInputAcquireHr = stopHr;
            if (SUCCEEDED(stopHr)) {
                pad.dinputEffect->Stop();
            }
            applied = true;
            continue;
        }

        HRESULT hr = pad.dinputDevice->Acquire();
        if (FAILED(hr)) {
            hr = pad.dinputDevice->Acquire();
        }
        lastDInputAcquireHr = hr;
        if (FAILED(hr) && hr != DIERR_OTHERAPPHASPRIO) {
            continue;
        }
        pad.connected = true;
        pad.dinputDevice->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
        pad.dinputEffect->Download();

        DICONSTANTFORCE constantForce{};
        constantForce.lMagnitude = magnitude;
        DIEFFECT effect{};
        effect.dwSize = sizeof(DIEFFECT);
        effect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        effect.lpvTypeSpecificParams = &constantForce;

        hr = pad.dinputEffect->SetParameters(&effect, DIEP_TYPESPECIFICPARAMS | DIEP_START);
        if (FAILED(hr)) {
            if (SUCCEEDED(pad.dinputDevice->Acquire())) {
                pad.dinputEffect->Download();
                hr = pad.dinputEffect->SetParameters(&effect, DIEP_TYPESPECIFICPARAMS | DIEP_START);
            }
        }
        if (FAILED(hr)) {
            hr = pad.dinputEffect->SetParameters(&effect, DIEP_TYPESPECIFICPARAMS);
            if (SUCCEEDED(hr)) {
                hr = pad.dinputEffect->Start(1, 0);
            }
        }
        if (SUCCEEDED(hr)) {
            applied = true;
            lastDInputSetHr = S_OK;
        } else {
            lastDInputSetHr = hr;
        }
    }

#ifdef _DEBUG
    if (!applied && !stopRequested) {
        static int noTargetWarningCount = 0;
        static int noFfWarningCount = 0;

        if (xinputConnectedCount == 0 && dinputConnectedCount == 0) {
            if (noTargetWarningCount < 5) {
                std::ostringstream oss;
                oss << "GamepadSystem::SetVibration - 振動適用先が見つかりませんでした"
                    << " (XInputConnected=" << xinputConnectedCount
                    << ", XInputApplied=" << xinputAppliedCount
                    << ", DInputConnected=" << dinputConnectedCount
                    << ", DualSenseHidConnected=" << dualSenseHidConnectedCount
                    << ", DualSenseHidApplied=" << dualSenseHidAppliedCount
                    << ", DInputCandidates=" << dinputCandidateCount
                    << ", LastAcquireHr=0x" << std::hex << static_cast<unsigned long>(lastDInputAcquireHr)
                    << ", LastSetHr=0x" << std::hex << static_cast<unsigned long>(lastDInputSetHr)
                    << ", LastDualSenseHidError=" << std::dec << lastDualSenseHidError << ")";
                DEBUGLOG_WARNING(oss.str());
                noTargetWarningCount++;
            }
        } else if (xinputConnectedCount == 0 && dualSenseHidConnectedCount > 0 && dualSenseHidAppliedCount == 0) {
            if (noFfWarningCount < 5) {
                std::ostringstream oss;
                oss << "GamepadSystem::SetVibration - DualSense HID振動の送信に失敗しました"
                    << " (DualSenseHidConnected=" << dualSenseHidConnectedCount
                    << ", LastDualSenseHidError=" << std::dec << lastDualSenseHidError << ")";
                DEBUGLOG_WARNING(oss.str());
                noFfWarningCount++;
            }
        } else if (xinputConnectedCount == 0 && dinputConnectedCount > 0 && dinputCandidateCount == 0 && dualSenseHidConnectedCount == 0) {
            if (noFfWarningCount < 5) {
                std::ostringstream oss;
                oss << "GamepadSystem::SetVibration - DirectInputデバイスは検出済みですが、フォースフィードバック非対応のため振動できません"
                    << " (DInputConnected=" << dinputConnectedCount
                    << ", DInputCandidates=" << dinputCandidateCount << ")";
                DEBUGLOG_WARNING(oss.str());
                noFfWarningCount++;
            }
        }
    }
#endif
}

// ========================================================
// ユーティリティ
// ========================================================

void GamepadSystem::ApplyDeadzone(float x, float y, float &outX, float &outY, float deadzone) const {
    // 円形デッドゾーンの適用
    float magnitude = sqrtf(x * x + y * y);

    if (magnitude < deadzone) {
        // デッドゾーン内
        outX = 0.0f;
        outY = 0.0f;
    } else {
        // デッドゾーン外 - 正規化して再スケール
        float normalizedX = x / magnitude;
        float normalizedY = y / magnitude;

        // 最大値でクリップ
        if (magnitude > 1.0f)
            magnitude = 1.0f;

        // デッドゾーンからの相対値に調整
        magnitude = (magnitude - deadzone) / (1.0f - deadzone);

        outX = normalizedX * magnitude;
        outY = normalizedY * magnitude;
    }
}

// ========================================================
// DirectInput デバイス列挙
// ========================================================

BOOL CALLBACK GamepadSystem::EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
    GamepadSystem *pThis = static_cast<GamepadSystem *>(pvRef);
    if (!pThis)
        return DIENUM_STOP;

    const unsigned short productVid = ProductVid(lpddi->guidProduct);
    const unsigned short productPid = ProductPid(lpddi->guidProduct);

#ifdef _DEBUG
    static int enumDiagnosticLogCount = 0;
    if (enumDiagnosticLogCount < 64) {
        const unsigned int vid = static_cast<unsigned int>(productVid);
        const unsigned int pid = static_cast<unsigned int>(productPid);
        std::ostringstream oss;
        oss << "GamepadSystem::EnumDevicesCallback() - EnumDevice"
            << " Name=" << lpddi->tszProductName
            << ", dwDevType=0x" << std::hex << static_cast<unsigned long>(lpddi->dwDevType)
            << ", BaseType=0x" << std::hex << static_cast<unsigned long>(GET_DIDEVICE_TYPE(lpddi->dwDevType))
            << ", VID=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << vid
            << ", PID=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << pid
            << ", guidInstance=" << GuidToHexString(lpddi->guidInstance)
            << ", guidProduct=" << GuidToHexString(lpddi->guidProduct);
        DEBUGLOG_CATEGORY(DebugLog::Category::Input, oss.str());
        enumDiagnosticLogCount++;
    }
#endif

    const DWORD deviceType = GET_DIDEVICE_TYPE(lpddi->dwDevType);
    const bool isControllerType =
        (deviceType == DI8DEVTYPE_JOYSTICK) ||
        (deviceType == DI8DEVTYPE_GAMEPAD) ||
        (deviceType == DI8DEVTYPE_1STPERSON) ||
        (deviceType == DI8DEVTYPE_DRIVING) ||
        (deviceType == DI8DEVTYPE_FLIGHT);
    if (!isControllerType) {
        return DIENUM_CONTINUE;
    }

    // 空きスロットを探す（DirectInput優先）
    int slot = -1;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (pThis->gamepads_[i].type == Type_None) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        // 空きスロットなし
        DEBUGLOG_WARNING("DirectInputデバイスの空きスロットがありません");
        return DIENUM_CONTINUE;
    }

    // DirectInputデバイスを作成
    LPDIRECTINPUTDEVICE8 device = nullptr;
    HRESULT hr = pThis->dinput_->CreateDevice(lpddi->guidInstance, &device, nullptr);
    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::EnumDevicesCallback() - デバイス作成失敗: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        return DIENUM_CONTINUE;
    }

    // データフォーマットを設定
    hr = device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::EnumDevicesCallback() - データフォーマット設定失敗: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        SAFE_RELEASE(device);
        return DIENUM_CONTINUE;
    }

    // 協調レベルを設定（入力安定性優先で NONEXCLUSIVE を先に試す）
    HWND hwnd = pThis->windowHandle_;
    if (!hwnd) {
        hwnd = GetActiveWindow();
    }
    if (!hwnd) {
        hwnd = GetForegroundWindow();
    }
    DWORD coopFlags = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
    hr = device->SetCooperativeLevel(hwnd, coopFlags);
    if (FAILED(hr) && hwnd) {
        coopFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;
        hr = device->SetCooperativeLevel(hwnd, coopFlags);
    }
    if (FAILED(hr) && hwnd) {
        coopFlags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
        hr = device->SetCooperativeLevel(hwnd, coopFlags);
    }
    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::EnumDevicesCallback() - 協調レベル設定失敗: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        SAFE_RELEASE(device);
        return DIENUM_CONTINUE;
    }

    // 軸の範囲を設定（-32767 ～ +32767ではなく、0 ～65535）
    DIPROPRANGE diprg;
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_DEVICE;
    diprg.diph.dwObj = 0;
    diprg.lMin = 0;
    diprg.lMax = 65535;

    hr = device->SetProperty(DIPROP_RANGE, &diprg.diph);
    if (FAILED(hr)) {
        // 範囲設定失敗は致命的ではない（デフォルト値を使用）
#ifdef _DEBUG
        std::ostringstream oss;
        oss << "GamepadSystem::EnumDevicesCallback() - 軸範囲設定失敗（デフォルト使用）: HRESULT=0x" << std::hex << hr;
        DEBUGLOG_WARNING(oss.str());
#endif
    }

    // デバイスを取得
    hr = device->Acquire();
    if (FAILED(hr)) {
        //取得失敗は致命的ではない(後で再取得可能)
#ifdef _DEBUG
        DEBUGLOG_WARNING("DirectInputデバイスの初回Acquireに失敗（後で再取得を試みます）");
#endif
    }

    // 振動対応可否を確認（ドライバ差異を考慮し、軸列挙結果を優先）
    DWORD ffCapsFlags = 0;
    bool forceFeedbackSupported = false;
    ForceFeedbackAxisContext ffAxisContext{};
    DIDEVCAPS caps{};
    caps.dwSize = sizeof(DIDEVCAPS);
    if (SUCCEEDED(device->GetCapabilities(&caps))) {
        ffCapsFlags = caps.dwFlags;
    }
    HRESULT ffEnumHr = device->EnumObjects(
        EnumForceFeedbackAxesCallback,
        &ffAxisContext,
        DIDFT_AXIS | DIDFT_FFACTUATOR);
    if (SUCCEEDED(ffEnumHr) && ffAxisContext.count > 0) {
        forceFeedbackSupported = true;
    }

    if (forceFeedbackSupported) {
        if (SUCCEEDED(device->Acquire())) {
            device->SendForceFeedbackCommand(DISFFC_RESET);
            device->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
        }
    }

    LPDIRECTINPUTEFFECT ffEffect = nullptr;
    if (forceFeedbackSupported) {
        DIPROPDWORD autoCenterProp{};
        autoCenterProp.diph.dwSize = sizeof(DIPROPDWORD);
        autoCenterProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        autoCenterProp.diph.dwObj = 0;
        autoCenterProp.diph.dwHow = DIPH_DEVICE;
        autoCenterProp.dwData = FALSE;
        device->SetProperty(DIPROP_AUTOCENTER, &autoCenterProp.diph);

        DIPROPDWORD ffGainProp{};
        ffGainProp.diph.dwSize = sizeof(DIPROPDWORD);
        ffGainProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        ffGainProp.diph.dwObj = 0;
        ffGainProp.diph.dwHow = DIPH_DEVICE;
        ffGainProp.dwData = DI_FFNOMINALMAX;
        device->SetProperty(DIPROP_FFGAIN, &ffGainProp.diph);

        DWORD axes[2] = {ffAxisContext.axes[0], ffAxisContext.count > 1 ? ffAxisContext.axes[1] : ffAxisContext.axes[0]};
        LONG directions[2] = {DI_FFNOMINALMAX, 0};
        DICONSTANTFORCE constantForce{};
        constantForce.lMagnitude = 0;

        DIEFFECT effect{};
        effect.dwSize = sizeof(DIEFFECT);
        effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
        effect.cAxes = static_cast<DWORD>(ffAxisContext.count > 1 ? 2 : 1);
        effect.rgdwAxes = axes;
        effect.rglDirection = directions;
        effect.lpEnvelope = nullptr;
        effect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        effect.lpvTypeSpecificParams = &constantForce;
        effect.dwDuration = INFINITE;
        effect.dwGain = DI_FFNOMINALMAX;
        effect.dwTriggerButton = DIEB_NOTRIGGER;
        effect.dwTriggerRepeatInterval = 0;
        effect.dwStartDelay = 0;

        auto createConstantEffect = [&]() {
            return device->CreateEffect(GUID_ConstantForce, &effect, &ffEffect, nullptr);
        };

        hr = createConstantEffect();
        if (FAILED(hr) && hwnd && coopFlags != (DISCL_FOREGROUND | DISCL_EXCLUSIVE)) {
            device->Unacquire();
            HRESULT coopHr = device->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
            if (SUCCEEDED(coopHr)) {
                coopFlags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
                if (SUCCEEDED(device->Acquire())) {
                    hr = createConstantEffect();
                }
            }
        }
        if (FAILED(hr)) {
            forceFeedbackSupported = false;
        } else {
            ffEffect->Download();
            ffEffect->Stop();
        }
    }

    HANDLE dualSenseHidHandle = INVALID_HANDLE_VALUE;
    bool dualSenseHidRumbleSupported = false;
    if (!forceFeedbackSupported && productVid == 0x054C && IsDualSensePid(productPid)) {
        dualSenseHidHandle = OpenDualSenseHidHandle(productVid, productPid);
        dualSenseHidRumbleSupported = (dualSenseHidHandle != INVALID_HANDLE_VALUE && dualSenseHidHandle != nullptr);
    }

    // ゲームパッド状態に設定
    pThis->gamepads_[slot].type = Type_DInput;
    pThis->gamepads_[slot].connected = true;
    pThis->gamepads_[slot].dinputDevice = device;
    pThis->gamepads_[slot].dinputEffect = ffEffect;
    pThis->gamepads_[slot].dinputForceFeedbackSupported = (forceFeedbackSupported && ffEffect != nullptr);
    pThis->gamepads_[slot].dualSenseHidHandle = dualSenseHidHandle;
    pThis->gamepads_[slot].dualSenseHidRumbleSupported = dualSenseHidRumbleSupported;

#ifdef _DEBUG
    const unsigned int vid = static_cast<unsigned int>(productVid);
    const unsigned int pid = static_cast<unsigned int>(productPid);
    std::ostringstream oss;
    oss << "GamepadSystem::EnumDevicesCallback() - DirectInputデバイス登録: Slot=" << slot
        << ", Name=" << lpddi->tszProductName
        << ", FF=" << (pThis->gamepads_[slot].dinputForceFeedbackSupported ? "ON" : "OFF")
        << ", DualSenseHid=" << (pThis->gamepads_[slot].dualSenseHidRumbleSupported ? "ON" : "OFF")
        << ", FFCaps=0x" << std::hex << static_cast<unsigned long>(ffCapsFlags)
        << ", FFAxes=" << std::dec << ffAxisContext.count
        << ", VID=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << vid
        << ", PID=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << pid;
    DEBUGLOG_CATEGORY(DebugLog::Category::Input, oss.str());
#endif

    return DIENUM_CONTINUE;
}

bool GamepadSystem::IsXInputDevice(const GUID *pGuidProductFromDirectInput) {
    try {
    IWbemLocator *pIWbemLocator = nullptr;
    IEnumWbemClassObject *pEnumDevices = nullptr;
    IWbemClassObject *pDevices[20] = {};
    IWbemServices *pIWbemServices = nullptr;
    BSTR bstrNamespace = nullptr;
    BSTR bstrDeviceID = nullptr;
    BSTR bstrClassName = nullptr;
    bool bIsXinputDevice = false;

    // COM初期化（すでにMTA初期化済みならRPC_E_CHANGED_MODEを無視）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool bCleanupCOM = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        // 既に別のモードで初期化されている場合は終了処理不要
        bCleanupCOM = false;
        DEBUGLOG_WARNING("GamepadSystem::IsXInputDevice() - COM already initialized with different model (ignored)");
    } else if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "GamepadSystem::IsXInputDevice() - CoInitializeEx failed: hr=0x" << std::hex << hr;
        DEBUGLOG_ERROR(oss.str());
        return false;
    }

    VARIANT var = {};
    VariantInit(&var);

    // WMI作成
    hr = CoCreateInstance(__uuidof(WbemLocator),
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IWbemLocator),
                          (LPVOID *) &pIWbemLocator);
    if (FAILED(hr) || pIWbemLocator == nullptr) {
        DEBUGLOG_ERROR("GamepadSystem::IsXInputDevice() - CoCreateInstance(WbemLocator) failed");
        goto LCleanup;
    }

    bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2");
    if (bstrNamespace == nullptr)
        goto LCleanup;
    bstrClassName = SysAllocString(L"Win32_PNPEntity");
    if (bstrClassName == nullptr)
        goto LCleanup;
    bstrDeviceID = SysAllocString(L"DeviceID");
    if (bstrDeviceID == nullptr)
        goto LCleanup;

    // WMI接続
    hr = pIWbemLocator->ConnectServer(bstrNamespace, nullptr, nullptr, 0L,
                                      0L, nullptr, nullptr, &pIWbemServices);
    if (FAILED(hr) || pIWbemServices == nullptr) {
        DEBUGLOG_ERROR("GamepadSystem::IsXInputDevice() - ConnectServer failed");
        goto LCleanup;
    }

    // セキュリティレベル設定
    hr = CoSetProxyBlanket(pIWbemServices,
                           RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           nullptr, EOAC_NONE);
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("GamepadSystem::IsXInputDevice() - CoSetProxyBlanket failed");
        goto LCleanup;
    }

    hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, nullptr, &pEnumDevices);
    if (FAILED(hr) || pEnumDevices == nullptr) {
        DEBUGLOG_ERROR("GamepadSystem::IsXInputDevice() - CreateInstanceEnum failed");
        goto LCleanup;
    }

    // デバイスをループ
    for (;;) {
        ULONG uReturned = 0;
        hr = pEnumDevices->Next(10000, sizeof(pDevices) / sizeof(pDevices[0]), pDevices, &uReturned);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("GamepadSystem::IsXInputDevice() - IEnumWbemClassObject::Next failed");
            goto LCleanup;
        }
        if (uReturned == 0)
            break;

        for (size_t iDevice = 0; iDevice < uReturned; ++iDevice) {
            // デバイスIDを取得
            hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, nullptr, nullptr);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != nullptr) {
                // "IG_"が含まれているかチェック(XInputデバイスの印)
                if (wcsstr(var.bstrVal, L"IG_")) {
                    // VID/PIDを取得
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR *strVid = wcsstr(var.bstrVal, L"VID_");
                    if (strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1)
                        dwVid = 0;
                    WCHAR *strPid = wcsstr(var.bstrVal, L"PID_");
                    if (strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1)
                        dwPid = 0;

                    // DInputデバイスのGUIDと比較
                    DWORD dwVidPid = MAKELONG(dwVid, dwPid);
                    if (dwVidPid == pGuidProductFromDirectInput->Data1) {
                        bIsXinputDevice = true;
                        goto LCleanup;
                    }
                }
            }
            VariantClear(&var);
            SAFE_RELEASE(pDevices[iDevice]);
        }
    }

LCleanup:
    VariantClear(&var);

    if (bstrNamespace)
        SysFreeString(bstrNamespace);
    if (bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if (bstrClassName)
        SysFreeString(bstrClassName);

    for (size_t iDevice = 0; iDevice < sizeof(pDevices) / sizeof(pDevices[0]); ++iDevice)
        SAFE_RELEASE(pDevices[iDevice]);

    SAFE_RELEASE(pEnumDevices);
    SAFE_RELEASE(pIWbemLocator);
    SAFE_RELEASE(pIWbemServices);

    if (bCleanupCOM)
        CoUninitialize();

    return bIsXinputDevice;
    } catch (const _com_error& ex) {
        std::wostringstream woss;
        const wchar_t* wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
        woss << L"GamepadSystem::IsXInputDevice() - _com_error caught. hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
        //std::wstring w = woss.str();
        //std::string n(w.begin(), w.end());
        //DEBUGLOG_ERROR(n);
        return false;
    }
}

// ========================================================
// グローバルアクセス関数
// ========================================================

/**
 * @brief ServiceLocator経由でGamepadSystemインスタンスを取得
 */
GamepadSystem &GetGamepad() {
    return ServiceLocator::Get<GamepadSystem>();
}

