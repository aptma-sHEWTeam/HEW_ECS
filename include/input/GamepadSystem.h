#pragma once
/**
 * @file GamepadSystem.h
 * @brief XInput と DirectInput を抽象化したゲームパッド入力管理システム
 * @author 山内陽
 * @date 2025
 * @version 6.0
 * 
 * @details
 * XInput と DirectInput を統合し、最大4つのゲームパッドの入力を管理します。
 * DirectInputデバイスを優先し、空きスロットにXInputデバイスを割り当てます。
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <initializer_list>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

/**
 * @class GamepadSystem
 * @brief ゲームパッド入力を統合管理するクラス
 * 
 * @details
 * XInput と DirectInput の両方をサポートし、接続されたゲームパッドを自動検出します。
 * 最大4つのゲームパッドを同時にサポートします。
 * 
 * @par 使用例
 * @code
 * GamepadSystem gamepad;
 * gamepad.Init();
 * 
 * while (running) {
 *  gamepad.Update();
 *     
 *     if (gamepad.GetButtonDown(0, GamepadSystem::Button_A)) {
 *       // プレイヤー0のAボタン押下時
 *     }
 *     
 *   float leftX = gamepad.GetLeftStickX(0);
 *     float leftY = gamepad.GetLeftStickY(0);
 * }
 * 
 * gamepad.Shutdown();
 * @endcode
 * 
 * @author 山内陽
 */
class GamepadSystem {
  public:
    /**
     * @brief サポートする最大ゲームパッド数
     */
    static const int MAX_GAMEPADS = 4;

    /**
     * @enum GamepadButton
     * @brief ゲームパッドボタンの識別子(Xbox配置)
     */
    enum GamepadButton {
        Button_A = 0,           ///< Aボタン(下) / ○
        Button_B = 1,           ///< Bボタン(右) / ×
        Button_X = 2,           ///< Xボタン(左) / □
        Button_Y = 3,           ///< Yボタン(上) / △
        Button_LB = 4,          ///< 左バンパー
        Button_RB = 5,          ///< 右バンパー
        Button_Back = 6,        ///< Backボタン
        Button_Start = 7,       ///< Startボタン
        Button_LS = 8,          ///< 左スティック押し込み
        Button_RS = 9,          ///< 右スティック押し込み
        Button_DPad_Up = 10,    ///< 十字キー上
        Button_DPad_Down = 11,  ///< 十字キー下
        Button_DPad_Left = 12,  ///< 十字キー左
        Button_DPad_Right = 13, ///< 十字キー右
        Button_Option = 14,    ///< オプションボタン
        Button_Count = 15       ///< ボタン総数
    };

    /**
     * @brief デフォルトコンストラクタ
     */
    GamepadSystem();

    /**
     * @brief デストラクタ
     */
    ~GamepadSystem();

    /**
   * @brief ゲームパッドシステムの初期化
   * @return true 初期化成功, false 初期化失敗
     * 
     * @details
     * DirectInputの初期化とゲームパッドの列挙を行います。
     */
    bool Init();

    /**
     * @brief 協調レベル設定に使用するウィンドウハンドルを設定
     * @param[in] hwnd メインウィンドウハンドル
     */
    void SetWindowHandle(HWND hwnd) { windowHandle_ = hwnd; }

    /**
     * @brief ゲームパッドシステムのシャットダウン
     * 
   * @details
     * DirectInputとすべてのデバイスリソースを解放します。
     */
    void Shutdown();

    /**
     * @brief 入力状態の更新(毎フレーム呼ぶ)
     * 
     * @details
     * XInputとDirectInputの状態を取得し、内部状態を更新します。
   * ボタンの押下・離された瞬間の判定はこの更新処理によって行われます。
     */
    void Update();

    // ========================================================
    // 統合入力取得（全コントローラー対応）
    // ========================================================

    /**
     * @brief すべての接続されているゲームパッドの左スティックX軸値を統合
     * @return float 統合されたX軸値(-1.0 ～ +1.0)
     * 
     * @details
     * すべてのゲームパッド（XInput + DirectInput）の左スティックX軸値を合算します。
     * 複数のコントローラーの入力を統合して1つの値として返します。
     */
    float GetLeftStickX() const;

    /**
     * @brief すべての接続されているゲームパッドの左スティックY軸値を統合
     * @return float 統合されたY軸値(-1.0 ～ +1.0)
     */
    float GetLeftStickY() const;

    /**
     * @brief すべての接続されているゲームパッドの右スティックX軸値を統合
     * @return float 統合されたX軸値(-1.0 ～ +1.0)
     */
    float GetRightStickX() const;

    /**
     * @brief すべての接続されているゲームパッドの右スティックY軸値を統合
     * @return float 統合されたY軸値(-1.0 ～ +1.0)
 */
    float GetRightStickY() const;

    /**
     * @brief すべての接続されているゲームパッドの左トリガー値を統合
     * @return float 統合されたトリガー値(0.0 ～ 1.0)
     */
    float GetLeftTrigger() const;

    /**
   * @brief すべての接続されているゲームパッドの右トリガー値を統合
     * @return float 統合されたトリガー値(0.0 ～ 1.0)
 */
    float GetRightTrigger() const;

    /**
     * @brief すべての接続されているゲームパッドでボタンが押されているか
     * @param[in] button ボタン識別子
     * @return true いずれかのゲームパッドで押されている, false すべて押されていない
     */
    bool GetButton(GamepadButton button) const;

    /**
   * @brief すべての接続されているゲームパッドでボタンが押された瞬間か
 * @param[in] button ボタン識別子
   * @return true いずれかのゲームパッドで押された, false すべて押されていない
     */
    bool GetButtonDown(GamepadButton button) const;

    /**
     * @brief すべての接続されているゲームパッドでボタンが離された瞬間か
     * @param[in] button ボタン識別子
     * @return true いずれかのゲームパッドで離された, false すべて離されていない
     */
    bool GetButtonUp(GamepadButton button) const;

    // 複数ボタンのエイリアス取得（いずれかが真なら真）
    /**
     * @brief 複数のボタンのいずれかが押されているか
     * @param[in] buttons ボタン識別子のリスト
     * @return true いずれかのゲームパッドで押されている, false すべて押されていない
     */
    bool GetAnyButton(std::initializer_list<GamepadButton> buttons) const;

    /**
     * @brief 複数のボタンのいずれかが押された瞬間か
     * @param[in] buttons ボタン識別子のリスト
     * @return true いずれかのゲームパッドで押された, false すべて押されていない
     */
    bool GetAnyButtonDown(std::initializer_list<GamepadButton> buttons) const;

    /**
     * @brief 複数のボタンのいずれかが離された瞬間か
     * @param[in] buttons ボタン識別子のリスト
     * @return true いずれかのゲームパッドで離された, false すべて離されていない
     */
    bool GetAnyButtonUp(std::initializer_list<GamepadButton> buttons) const;

    // ========================================================
    // チャージ&リリースシステム（統合版）
    // ========================================================

    /**
   * @brief 左スティックがチャージ中かどうか
     * @return true チャージ中, false ニュートラル
     * 
     * @details
     * いずれかのゲームパッドの左スティックが傾けられている状態を返します。
     */
    bool IsLeftStickCharging() const;

    /**
     * @brief 右スティックがチャージ中かどうか
     * @return true チャージ中, false ニュートラル
     */
    bool IsRightStickCharging() const;

    /**
     * @brief 左スティックの最大チャージ時間を取得
     * @return float チャージ時間(秒)
     * 
     * @details
     * すべての接続されているゲームパッドの中で最も長いチャージ時間を返します。
     */
    float GetLeftStickChargeTime() const;

    /**
     * @brief 右スティックの最大チャージ時間を取得
     * @return float チャージ時間(秒)
     */
    float GetRightStickChargeTime() const;

    /**
     * @brief 左スティックがこのフレームでリリースされたか
     * @return true リリースされた, false リリースされていない
     * 
 * @details
     * いずれかのゲームパッドの左スティックがチャージ状態からニュートラルに戻った瞬間にtrueを返します。
     */
    bool IsLeftStickReleased() const;

    /**
     * @brief 右スティックがこのフレームでリリースされたか
     * @return true リリースされた, false リリースされていない
  */
    bool IsRightStickReleased() const;

    /**
     * @brief 左スティックのチャージ量を取得(0.0 ～ 1.0)
     * @param[in] maxChargeTime 最大チャージ時間(秒、デフォルト3.0秒)
     * @return float チャージ量(0.0 ～ 1.0)
     * 
     * @details
* すべてのゲームパッドの中で最も長いチャージ時間を0.0～1.0の範囲に正規化して返します。
     * 
     * @par 使用例
     * @code
     * if (GetGamepad().IsLeftStickCharging()) {
     *     float charge = GetGamepad().GetLeftStickChargeAmount(2.0f);
     *     DrawChargeGauge(charge);
  * }
     * 
     * if (GetGamepad().IsLeftStickReleased()) {
     *     float charge = GetGamepad().GetLeftStickChargeAmount(2.0f);
     *     ShootWithPower(charge);
     * }
     * @endcode
     */
    float GetLeftStickChargeAmount(float maxChargeTime = 3.0f) const;
    
    /**
     * @brief 右スティックのチャージ量を取得(0.0 ～ 1.0)
     * @param[in] maxChargeTime 最大チャージ時間(秒、デフォルト3.0秒)
     * @return float チャージ量(0.0 ～ 1.0)
     */
    float GetRightStickChargeAmount(float maxChargeTime = 3.0f) const;

    /**
     * @brief 左スティックの最大入力強度を取得
     * @return float 最大入力強度(0.0 ～ 1.0)
     * 
     * @details
     * すべてのゲームパッドの中で最も強い左スティックの平均入力強度を返します。
     */
    float GetLeftStickAverageIntensity() const;

    /**
     * @brief 右スティックの最大入力強度を取得
     * @return float 最大入力強度(0.0 ～ 1.0)
     */
    float GetRightStickAverageIntensity() const;

    /**
     * @brief バイブレーション(振動)を全コントローラーに設定
   * @param[in] leftMotor 左モーター強度(0.0 ～ 1.0)
     * @param[in] rightMotor 右モーター強度(0.0 ～ 1.0)
   * 
     * @details
     * すべての接続されているXInputデバイスに振動を設定します。
     */
    void SetVibration(float leftMotor, float rightMotor);

  private:
    /**
     * @enum ButtonState
     * @brief ボタンの状態
     */
    enum ButtonState : uint8_t {
        None = 0,      ///< 何も押されていない
        Down = 1,      ///< このフレームで押された
        Pressed = 2,   ///< 押され続けている
        Up = 3         ///< このフレームで離された
    };

    /**
     * @enum DeviceType
     * @brief デバイスタイプ
     */
    enum DeviceType {
        Type_None,   ///< 未接続
        Type_XInput, ///< XInputデバイス
        Type_DInput  ///< DirectInputデバイス
    };

    /**
     * @struct GamepadState
     * @brief ゲームパッドの状態
     */
    struct GamepadState {
        DeviceType type;                   ///< デバイスタイプ
        bool connected;                    ///< 接続状態
        uint8_t buttons[Button_Count];     ///< ボタン状態
        uint8_t prevButtons[Button_Count]; ///< 前フレームのボタン状態
        float leftStickX;                  ///< 左スティックX
        float leftStickY;                  ///< 左スティックY
        float rightStickX;                 ///< 右スティックX
        float rightStickY;                 ///< 右スティックY
        float leftTrigger;                 ///< 左トリガー
        float rightTrigger;                ///< 右トリガー
        LPDIRECTINPUTDEVICE8 dinputDevice; ///< DirectInputデバイス
        LPDIRECTINPUTEFFECT dinputEffect;  ///< DirectInput振動エフェクト
        bool dinputForceFeedbackSupported; ///< DirectInput振動対応有無
        DWORD xinputIndex;                 ///< XInputインデックス

        // チャージ&リリースシステム用
        bool leftStickWasCharging;    ///< 前フレームで左スティックがチャージ中だったか
        bool rightStickWasCharging;   ///< 前フレームで右スティックがチャージ中だったか
        float leftStickChargeTime;    ///< 左スティックチャージ時間
        float rightStickChargeTime;   ///< 右スティックチャージ時間
        float leftStickIntensitySum;  ///< 左スティック強度累積値
        float rightStickIntensitySum; ///< 右スティック強度累積値
        int leftStickChargeSamples;   ///< 左スティックチャージサンプル数
        int rightStickChargeSamples;  ///< 右スティックチャージサンプル数
        float leftStickReleaseTimer;  ///< 左スティックリリースタイマー
        float rightStickReleaseTimer; ///<右スティックリリースタイマー
        bool leftStickJustReleased;   ///< 左スティックが直前フレームで離されたか
        bool rightStickJustReleased;  ///< 右スティックが直前フレームで離されたか

        GamepadState() {
            type = Type_None;
            connected = false;
            memset(buttons, 0, sizeof(buttons));
            memset(prevButtons, 0, sizeof(prevButtons));
            leftStickX = leftStickY = 0.0f;
            rightStickX = rightStickY = 0.0f;
            leftTrigger = rightTrigger = 0.0f;
            dinputDevice = nullptr;
            dinputEffect = nullptr;
            dinputForceFeedbackSupported = false;
            xinputIndex = 0;

            // チャージシステム初期化
            leftStickWasCharging = false;
            rightStickWasCharging = false;
            leftStickChargeTime = 0.0f;
            rightStickChargeTime = 0.0f;
            leftStickIntensitySum = 0.0f;
            rightStickIntensitySum = 0.0f;
            leftStickChargeSamples = 0;
            rightStickChargeSamples = 0;
            leftStickReleaseTimer = 0.0f;
            rightStickReleaseTimer = 0.0f;
            leftStickJustReleased = false;
            rightStickJustReleased = false;
        }
    };
    /**
     * @brief XInputデバイスの状態を更新
     * @param[in] index ゲームパッドインデックス
     */
    void UpdateXInput(int index);

    /**
     * @brief DirectInputデバイスの状態を更新
     * @param[in] index ゲームパッドインデックス
     */
    void UpdateDInput(int index);

    /**
     * @brief チャージ&リリースシステムの更新
     * @param[in] index ゲームパッドインデックス
     * @param[in] dt デルタタイム(秒)
     * 
     * @details
     * スティックのチャージ時間と強度を計測します。
     */
    void UpdateChargeSystem(int index, float dt);

    /**
     * @brief スティック値にデッドゾーンを適用
     * @param[in] x X軸生値
     * @param[in] y Y軸生値
     * @param[out] outX 処理済みX軸値
     * @param[out] outY 処理済みY軸値
     * @param[in] deadzone デッドゾーン半径
     * 
     * @details
     * 円形デッドゾーンを適用し、正規化します。
     */
    void ApplyDeadzone(float x, float y, float &outX, float &outY, float deadzone) const;

    /**
     * @brief DirectInputデバイスの列挙コールバック
     */
    static BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);

    /**
     * @brief XInputデバイスかどうかを判定
     * @param[in] pGuidProductFromDirectInput DirectInputデバイスのGUID
     * @return true XInputデバイス, false DirectInputデバイス
     * 
     * @details
     * WMIを使用してデバイスIDを確認し、XInputデバイスを識別します。
     * XInputデバイスの場合はDirectInputで列挙しないようにします。
     */
    static bool IsXInputDevice(const GUID *pGuidProductFromDirectInput);

    GamepadState gamepads_[MAX_GAMEPADS]; ///< ゲームパッド状態
    LPDIRECTINPUT8 dinput_;               ///< DirectInput8インターフェース
    HWND windowHandle_;                   ///< 協調レベル設定用ウィンドウハンドル
    int nextDInputSlot_;                  ///< 次に使用するDirectInputスロット
    float deltaTime_;                     ///< 前フレームのデルタタイム

    // デッドゾーン定数
    static constexpr float XINPUT_LEFT_DEADZONE = 7849.0f / 32767.0f;  ///< 左スティックデッドゾーン
    static constexpr float XINPUT_RIGHT_DEADZONE = 8689.0f / 32767.0f; ///< 右スティックデッドゾーン
    static constexpr float XINPUT_TRIGGER_THRESHOLD = 30.0f / 255.0f;  ///< トリガー閾値
    static constexpr float CHARGE_DETECTION_THRESHOLD = 0.1f;          ///< チャージ検出閾値
};

/**
 * @brief グローバルなゲームパッドシステムインスタンスを取得
 * @return GamepadSystem& シングルトンインスタンス
 * 
 * @details
 * どこからでもアクセス可能なゲームパッドシステムのインスタンスを返します。
 * 
 * @par 使用例
 * @code
 * if (GetGamepad().GetButtonDown(0, GamepadSystem::Button_A)) {
 *  // プレイヤー0のAボタンが押された
 * }
 * @endcode
 * 
 * @author 山内陽
 */
GamepadSystem &GetGamepad();
