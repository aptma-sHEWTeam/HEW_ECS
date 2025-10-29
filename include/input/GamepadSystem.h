#pragma once
/**
 * @file GamepadSystem.h
 * @brief XInput �� DirectInput �𒊏ۉ������Q�[���p�b�h���͊Ǘ��V�X�e��
 * @author �R���z
 * @date 2025
 * @version 6.0
 * 
 * @details
 * XInput �� DirectInput �𓝍����A�ő�4�̃Q�[���p�b�h�̓��͂��Ǘ����܂��B
 * XInput�f�o�C�X��D��I�Ɏg�p���AXInput�ŔF������Ȃ��f�o�C�X��DirectInput�ŏ������܂��B
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <dinput.h>
#include <cstdint>
#include <cstring>
#include <cmath>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

/**
 * @class GamepadSystem
 * @brief �Q�[���p�b�h���͂𓝍��Ǘ�����N���X
 * 
 * @details
 * XInput �� DirectInput �̗������T�|�[�g���A�ڑ����ꂽ�Q�[���p�b�h���������o���܂��B
 * �ő�4�̃Q�[���p�b�h�𓯎��ɃT�|�[�g���܂��B
 * 
 * @par �g�p��
 * @code
 * GamepadSystem gamepad;
 * gamepad.Init();
 * 
 * while (running) {
 *  gamepad.Update();
 *     
 *     if (gamepad.GetButtonDown(0, GamepadSystem::Button_A)) {
 *       // �v���C���[0��A�{�^��������
 *     }
 *     
 *   float leftX = gamepad.GetLeftStickX(0);
 *     float leftY = gamepad.GetLeftStickY(0);
 * }
 * 
 * gamepad.Shutdown();
 * @endcode
 * 
 * @author �R���z
 */
class GamepadSystem {
public:
    /**
     * @brief �T�|�[�g����ő�Q�[���p�b�h��
     */
    static const int MAX_GAMEPADS = 4;

    /**
     * @enum GamepadButton
     * @brief �Q�[���p�b�h�{�^���̎��ʎq(Xbox�z�u)
     */
    enum GamepadButton {
        Button_A = 0,     ///< A�{�^��(��)
        Button_B = 1,      ///< B�{�^��(�E)
        Button_X = 2,           ///< X�{�^��(��)
        Button_Y = 3,        ///< Y�{�^��(��)
        Button_LB = 4,          ///< ���o���p�[
        Button_RB = 5, ///< �E�o���p�[
        Button_Back = 6,        ///< Back�{�^��
        Button_Start = 7,     ///< Start�{�^��
        Button_LS = 8,          ///< ���X�e�B�b�N��������
        Button_RS = 9,          ///< �E�X�e�B�b�N��������
        Button_DPad_Up = 10,    ///< �\���L�[��
        Button_DPad_Down = 11,  ///< �\���L�[��
      Button_DPad_Left = 12,  ///< �\���L�[��
        Button_DPad_Right = 13, ///< �\���L�[�E
        Button_Count = 14 ///< �{�^������
    };

    /**
     * @brief �f�t�H���g�R���X�g���N�^
     */
    GamepadSystem();

    /**
     * @brief �f�X�g���N�^
     */
    ~GamepadSystem();

    /**
   * @brief �Q�[���p�b�h�V�X�e���̏�����
   * @return true ����������, false ���������s
     * 
     * @details
     * DirectInput�̏������ƃQ�[���p�b�h�̗񋓂��s���܂��B
     */
  bool Init();

    /**
     * @brief �Q�[���p�b�h�V�X�e���̃V���b�g�_�E��
     * 
   * @details
     * DirectInput�Ƃ��ׂẴf�o�C�X���\�[�X��������܂��B
     */
    void Shutdown();

    /**
     * @brief ���͏�Ԃ̍X�V(���t���[���Ă�)
     * 
     * @details
     * XInput��DirectInput�̏�Ԃ��擾���A������Ԃ��X�V���܂��B
     * �{�^���̉����E�����ꂽ�u�Ԃ̔���͂��̍X�V�����ɂ���čs���܂��B
     */
    void Update();

    /**
     * @brief �Q�[���p�b�h���ڑ�����Ă��邩
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return true �ڑ���, false ���ڑ�
*/
    bool IsConnected(int index) const;

  /**
     * @brief �{�^����������Ă��邩
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] button �{�^�����ʎq
     * @return true ������Ă���, false ������Ă��Ȃ�
     * 
     * @details
     * �{�^���������ꑱ���Ă���ԁA�܂��͂��̃t���[���ŉ����ꂽ�u�Ԃ�true��Ԃ��܂��B
     */
    bool GetButton(int index, GamepadButton button) const;

    /**
   * @brief �{�^�������̃t���[���ŉ����ꂽ�u�Ԃ�
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] button �{�^�����ʎq
     * @return true �����ꂽ�u��, false ����ȊO
     * 
     * @details
  * �{�^���������ꂽ�ŏ��̃t���[���̂�true��Ԃ��܂��B
     */
    bool GetButtonDown(int index, GamepadButton button) const;

    /**
* @brief �{�^�������̃t���[���ŗ����ꂽ�u�Ԃ�
  * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] button �{�^�����ʎq
* @return true �����ꂽ�u��, false ����ȊO
     */
    bool GetButtonUp(int index, GamepadButton button) const;

    /**
     * @brief ���X�e�B�b�N��X���l���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
 * @return float X���l(-1.0 �` +1.0)
     * 
     * @details
     * �f�b�h�]�[�������ς݂̒l��Ԃ��܂��B
     * -1.0�����A+1.0���E��\���܂��B
  */
    float GetLeftStickX(int index) const;

    /**
     * @brief ���X�e�B�b�N��Y���l���擾
  * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
   * @return float Y���l(-1.0 �` +1.0)
     * 
     * @details
     * �f�b�h�]�[�������ς݂̒l��Ԃ��܂��B
     * -1.0�����A+1.0�����\���܂��B
  */
    float GetLeftStickY(int index) const;

    /**
     * @brief �E�X�e�B�b�N��X���l���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float X���l(-1.0 �` +1.0)
     */
    float GetRightStickX(int index) const;

    /**
     * @brief �E�X�e�B�b�N��Y���l���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float Y���l(-1.0 �` +1.0)
     */
    float GetRightStickY(int index) const;

    /**
     * @brief ���g���K�[�̒l���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float �g���K�[�l(0.0 �` 1.0)
     * 
     * @details
     * 0.0���������A1.0���ő剟����\���܂��B
     */
    float GetLeftTrigger(int index) const;

    /**
     * @brief �E�g���K�[�̒l���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float �g���K�[�l(0.0 �` 1.0)
     */
    float GetRightTrigger(int index) const;

    /**
     * @brief �o�C�u���[�V����(�U��)��ݒ�
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] leftMotor �����[�^�[���x(0.0 �` 1.0)
     * @param[in] rightMotor �E���[�^�[���x(0.0 �` 1.0)
     * 
     * @details
     * XInput�f�o�C�X�̂݃T�|�[�g���܂��BDirectInput�f�o�C�X�ł͉����N���܂���B
     * �����[�^�[�͒���g�A�E���[�^�[�͍����g�̐U���𐶐����܂��B
     */
    void SetVibration(int index, float leftMotor, float rightMotor);

    // ========================================================
    // �`���[�W&�����[�X�V�X�e��(�Q�[�����C���V�X�e��)
// ========================================================

    /**
 * @brief ���X�e�B�b�N���`���[�W�����ǂ���
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return true �`���[�W��, false �j���[�g����
     * 
     * @details
   * �X�e�B�b�N���X�����Ă����Ԃ�Ԃ��܂��B
     */
    bool IsLeftStickCharging(int index) const;

    /**
     * @brief �E�X�e�B�b�N���`���[�W�����ǂ���
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return true �`���[�W��, false �j���[�g����
  */
    bool IsRightStickCharging(int index) const;

    /**
     * @brief ���X�e�B�b�N�̃`���[�W���Ԃ��擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
   * @return float �`���[�W����(�b)
 * 
     * @details
     * �X�e�B�b�N���X�������Ă��鎞�Ԃ�Ԃ��܂��B
     * �j���[�g�����ɖ߂�ƃ��Z�b�g����܂��B
     */
    float GetLeftStickChargeTime(int index) const;

    /**
     * @brief �E�X�e�B�b�N�̃`���[�W���Ԃ��擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float �`���[�W����(�b)
     */
    float GetRightStickChargeTime(int index) const;

    /**
   * @brief ���X�e�B�b�N�����̃t���[���Ń����[�X���ꂽ��
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return true �����[�X���ꂽ, false �����[�X����Ă��Ȃ�
     * 
     * @details
     * �X�e�B�b�N���`���[�W��Ԃ���j���[�g�����ɖ߂����u�Ԃ�true��Ԃ��܂��B
     * ���̃t���[���̂�true�ŁA���̃t���[������false�ɂȂ�܂��B
     * 
     * @par �Q�[�����C���V�X�e���ł̎g�p��
* @code
     * if (GetGamepad().IsLeftStickReleased(0)) {
     *     float chargeTime = GetGamepad().GetLeftStickChargeTime(0);
  *     float power = min(chargeTime * 2.0f, 10.0f); // �ő�10.0
     *  ShootProjectile(power);
     * }
     * @endcode
     */
    bool IsLeftStickReleased(int index) const;

    /**
     * @brief �E�X�e�B�b�N�����̃t���[���Ń����[�X���ꂽ��
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return true �����[�X���ꂽ, false �����[�X����Ă��Ȃ�
     */
    bool IsRightStickReleased(int index) const;

    /**
     * @brief ���X�e�B�b�N�̃`���[�W�ʂ��擾(0.0 �` 1.0)
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] maxChargeTime �ő�`���[�W����(�b�A�f�t�H���g3.0�b)
     * @return float �`���[�W��(0.0 �` 1.0)
     * 
     * @details
     * �`���[�W���Ԃ�0.0�`1.0�͈̔͂ɐ��K�����ĕԂ��܂��B
     * maxChargeTime�b��1.0(100%)�ɒB���܂��B
 * 
     * @par �Q�[�����C���V�X�e���ł̎g�p��
     * @code
     * // ���A���^�C���Ń`���[�W�ʂ�\��
     * if (GetGamepad().IsLeftStickCharging(0)) {
     *     float charge = GetGamepad().GetLeftStickChargeAmount(0, 2.0f);
     *     DrawChargeGauge(charge);
     * }
     * 
     * // �����[�X���ɔ���
     * if (GetGamepad().IsLeftStickReleased(0)) {
     *     float charge = GetGamepad().GetLeftStickChargeAmount(0, 2.0f);
     *     ShootWithPower(charge);
     * }
     * @endcode
     */
    float GetLeftStickChargeAmount(int index, float maxChargeTime = 3.0f) const;

    /**
     * @brief �E�X�e�B�b�N�̃`���[�W�ʂ��擾(0.0 �` 1.0)
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @param[in] maxChargeTime �ő�`���[�W����(�b�A�f�t�H���g3.0�b)
     * @return float �`���[�W��(0.0 �` 1.0)
     */
    float GetRightStickChargeAmount(int index, float maxChargeTime = 3.0f) const;

    /**
     * @brief ���X�e�B�b�N�̕��ϓ��͋��x���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float ���ϓ��͋��x(0.0 �` 1.0)
     * 
     * @details
     * �`���[�W���̃X�e�B�b�N�̕��ϓI�ȌX�����Ԃ��܂��B
     * �����X����قǒl���傫���Ȃ�܂��B
     * �j���[�g�����܂��̓`���[�W���Ă��Ȃ��ꍇ��0.0��Ԃ��܂��B
     */
    float GetLeftStickAverageIntensity(int index) const;

    /**
     * @brief �E�X�e�B�b�N�̕��ϓ��͋��x���擾
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X(0-3)
     * @return float ���ϓ��͋��x(0.0 �` 1.0)
     */
    float GetRightStickAverageIntensity(int index) const;

private:
    /**
     * @enum ButtonState
     * @brief �{�^���̏��
     */
    enum ButtonState : uint8_t {
        None = 0,      ///< ����������Ă��Ȃ�
      Down = 1,      ///< ���̃t���[���ŉ����ꂽ
        Pressed = 2,   ///< �����ꑱ���Ă���
        Up = 3         ///< ���̃t���[���ŗ����ꂽ
    };

    /**
     * @enum DeviceType
     * @brief �f�o�C�X�^�C�v
     */
    enum DeviceType {
        Type_None,      ///< ���ڑ�
    Type_XInput,    ///< XInput�f�o�C�X
        Type_DInput     ///< DirectInput�f�o�C�X
    };

    /**
     * @struct GamepadState
     * @brief �Q�[���p�b�h�̏��
     */
    struct GamepadState {
        DeviceType type;   ///< �f�o�C�X�^�C�v
        bool connected; ///< �ڑ����
    uint8_t buttons[Button_Count];  ///< �{�^�����
   uint8_t prevButtons[Button_Count];///< �O�t���[���̃{�^�����
        float leftStickX;     ///< ���X�e�B�b�NX
     float leftStickY;   ///< ���X�e�B�b�NY
        float rightStickX;        ///< �E�X�e�B�b�NX
        float rightStickY;    ///< �E�X�e�B�b�NY
        float leftTrigger; ///< ���g���K�[
      float rightTrigger;          ///< �E�g���K�[
    LPDIRECTINPUTDEVICE8 dinputDevice;      ///< DirectInput�f�o�C�X
   DWORD xinputIndex; ///< XInput�C���f�b�N�X

        // �`���[�W&�����[�X�V�X�e���p
        bool leftStickWasCharging;   ///< �O�t���[���ō��X�e�B�b�N���`���[�W����������
        bool rightStickWasCharging;  ///< �O�t���[���ŉE�X�e�B�b�N���`���[�W����������
     float leftStickChargeTime;   ///< ���X�e�B�b�N�`���[�W����
        float rightStickChargeTime;  ///< �E�X�e�B�b�N�`���[�W����
        float leftStickIntensitySum; ///< ���X�e�B�b�N���x�ݐϒl
        float rightStickIntensitySum;///< �E�X�e�B�b�N���x�ݐϒl
        int leftStickChargeSamples;  ///< ���X�e�B�b�N�`���[�W�T���v����
        int rightStickChargeSamples; ///< �E�X�e�B�b�N�`���[�W�T���v����

     GamepadState() {
            type = Type_None;
 connected = false;
   memset(buttons, 0, sizeof(buttons));
        memset(prevButtons, 0, sizeof(prevButtons));
  leftStickX = leftStickY = 0.0f;
 rightStickX = rightStickY = 0.0f;
      leftTrigger = rightTrigger = 0.0f;
   dinputDevice = nullptr;
        xinputIndex = 0;
            
          // �`���[�W�V�X�e��������
            leftStickWasCharging = false;
    rightStickWasCharging = false;
          leftStickChargeTime = 0.0f;
   rightStickChargeTime = 0.0f;
  leftStickIntensitySum = 0.0f;
   rightStickIntensitySum = 0.0f;
leftStickChargeSamples = 0;
            rightStickChargeSamples = 0;
        }
    };

    /**
     * @brief XInput�f�o�C�X�̏�Ԃ��X�V
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X
     */
    void UpdateXInput(int index);

    /**
     * @brief DirectInput�f�o�C�X�̏�Ԃ��X�V
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X
     */
    void UpdateDInput(int index);

    /**
     * @brief �`���[�W&�����[�X�V�X�e���̍X�V
     * @param[in] index �Q�[���p�b�h�C���f�b�N�X
     * @param[in] dt �f���^�^�C��(�b)
     * 
     * @details
     * �X�e�B�b�N�̃`���[�W���ԂƋ��x���v�����܂��B
     */
    void UpdateChargeSystem(int index, float dt);

    /**
     * @brief �X�e�B�b�N�l�Ƀf�b�h�]�[����K�p
     * @param[in] x X�����l
     * @param[in] y Y�����l
   * @param[out] outX �����ς�X���l
 * @param[out] outY �����ς�Y���l
     * @param[in] deadzone �f�b�h�]�[�����a
     * 
     * @details
     * �~�`�f�b�h�]�[����K�p���A���K�����܂��B
 */
    void ApplyDeadzone(float x, float y, float& outX, float& outY, float deadzone) const;

    /**
     * @brief DirectInput�f�o�C�X�̗񋓃R�[���o�b�N
     */
    static BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);

    /**
     * @brief XInput�f�o�C�X���ǂ����𔻒�
   * @param[in] pGuidProductFromDirectInput DirectInput�f�o�C�X��GUID
     * @return true XInput�f�o�C�X, false DirectInput�f�o�C�X
   * 
     * @details
     * WMI���g�p���ăf�o�C�XID���m�F���AXInput�f�o�C�X�����ʂ��܂��B
     * XInput�f�o�C�X�̏ꍇ��DirectInput�ŗ񋓂��Ȃ��悤�ɂ��܂��B
     */
  static bool IsXInputDevice(const GUID* pGuidProductFromDirectInput);

    GamepadState gamepads_[MAX_GAMEPADS];  ///< �Q�[���p�b�h���
    LPDIRECTINPUT8 dinput_;        ///< DirectInput8�C���^�[�t�F�[�X
    int nextDInputSlot_;       ///< ���Ɏg�p����DirectInput�X���b�g
    float deltaTime_;    ///< �O�t���[���̃f���^�^�C��

    // �f�b�h�]�[���萔
    static constexpr float XINPUT_LEFT_DEADZONE = 7849.0f / 32767.0f;   ///< ���X�e�B�b�N�f�b�h�]�[��
  static constexpr float XINPUT_RIGHT_DEADZONE = 8689.0f / 32767.0f;  ///< �E�X�e�B�b�N�f�b�h�]�[��
    static constexpr float XINPUT_TRIGGER_THRESHOLD = 30.0f / 255.0f;   ///< �g���K�[臒l
    static constexpr float CHARGE_DETECTION_THRESHOLD = 0.1f; ///< �`���[�W���o臒l
};

/**
 * @brief �O���[�o���ȃQ�[���p�b�h�V�X�e���C���X�^���X���擾
 * @return GamepadSystem& �V���O���g���C���X�^���X
 * 
 * @details
 * �ǂ�����ł��A�N�Z�X�\�ȃQ�[���p�b�h�V�X�e���̃C���X�^���X��Ԃ��܂��B
 * 
 * @par �g�p��
 * @code
 * if (GetGamepad().GetButtonDown(0, GamepadSystem::Button_A)) {
 *  // �v���C���[0��A�{�^���������ꂽ
 * }
 * @endcode
 * 
 * @author �R���z
 */
inline GamepadSystem& GetGamepad() {
    static GamepadSystem instance;
    return instance;
}
