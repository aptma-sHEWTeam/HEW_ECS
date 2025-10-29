# GamepadSystem - XInput & DirectInput �������̓V�X�e��

## �T�v

`GamepadSystem` �́AXInput �� DirectInput �𒊏ۉ������Q�[���p�b�h���͊Ǘ��V�X�e���ł��B�ő�4�̃Q�[���p�b�h�𓯎��ɃT�|�[�g���AXInput�f�o�C�X��D��I�Ɏg�p���܂��B

## ����

- **XInput �� DirectInput �̓���**: XInput�f�o�C�X��D�悵�A��XInput�f�o�C�X��DirectInput�ŏ���
- **�ő�4�v���C���[�Ή�**: ������4�̃Q�[���p�b�h���T�|�[�g
- **�����f�o�C�X���o**: �ڑ����ꂽ�Q�[���p�b�h�������ŔF��
- **�f�b�h�]�[������**: �X�e�B�b�N�ƃg���K�[�ɓK�؂ȃf�b�h�]�[����K�p
- **�U���T�|�[�g**: XInput�f�o�C�X�̐U���@�\���T�|�[�g
- **C++14����**: �v���W�F�N�g�̃R�[�f�B���O�K��Ɋ��S����

## �g�p���@

### �������ƃV���b�g�_�E��

```cpp
#include "input/GamepadSystem.h"

// ������
GamepadSystem& gamepad = GetGamepad();
gamepad.Init();

// ���C�����[�v
while (running) {
    gamepad.Update();
    
    // �Q�[������
}

// �V���b�g�_�E��
gamepad.Shutdown();
```

### �{�^�����͂̎擾

```cpp
// �v���C���[0��A�{�^����������Ă��邩
if (GetGamepad().GetButton(0, GamepadSystem::Button_A)) {
 // �����ꑱ���Ă���Ԃ̏���
}

// A�{�^���������ꂽ�u��
if (GetGamepad().GetButtonDown(0, GamepadSystem::Button_A)) {
    // �W�����v�ȂǁA1�񂾂����s����������
}

// A�{�^���������ꂽ�u��
if (GetGamepad().GetButtonUp(0, GamepadSystem::Button_A)) {
    // �{�^���𗣂������̏���
}
```

### �X�e�B�b�N���͂̎擾

```cpp
// ���X�e�B�b�N�̒l���擾(-1.0 �` +1.0)
float leftX = GetGamepad().GetLeftStickX(0);
float leftY = GetGamepad().GetLeftStickY(0);

// �v���C���[���ړ�
transform->position.x += leftX * moveSpeed * deltaTime;
transform->position.z += leftY * moveSpeed * deltaTime;

// �E�X�e�B�b�N�ŃJ������]
float rightX = GetGamepad().GetRightStickX(0);
float rightY = GetGamepad().GetRightStickY(0);

cameraRotation.y += rightX * sensitivity * deltaTime;
cameraRotation.x += rightY * sensitivity * deltaTime;
```

### �g���K�[���͂̎擾

```cpp
// ���g���K�[�ƉE�g���K�[�̒l(0.0 �` 1.0)
float leftTrigger = GetGamepad().GetLeftTrigger(0);
float rightTrigger = GetGamepad().GetRightTrigger(0);

// �_�b�V������
if (rightTrigger > 0.5f) {
    currentSpeed = dashSpeed;
}

// �A�N�Z��/�u���[�L
vehicleSpeed += rightTrigger * acceleration;
vehicleSpeed -= leftTrigger * braking;
```

### �U��(�o�C�u���[�V����)

```cpp
// �U����ݒ�(0.0 �` 1.0)
// �����[�^�[: ����g�U��
// �E���[�^�[: �����g�U��
GetGamepad().SetVibration(0, 0.5f, 0.5f);

// �U�����~
GetGamepad().SetVibration(0, 0.0f, 0.0f);
```

### �ڑ��m�F

```cpp
// �Q�[���p�b�h���ڑ�����Ă��邩�m�F
if (GetGamepad().IsConnected(0)) {
    // �v���C���[0�̃Q�[���p�b�h���ڑ���
}
```

## ECS�ł̎g�p��

### �v���C���[�R���g���[���[

```cpp
#include "samples/GamepadSample.h"

// �v���C���[�G���e�B�e�B�쐬
Entity player = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
    .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
    .With<GamepadPlayerController>(0)  // �v���C���[0
    .Build();
```

### �J�����R���g���[���[

```cpp
Entity camera = world.Create()
    .With<Transform>(DirectX::XMFLOAT3{0, 2, -5})
  .With<GamepadCameraController>(0)
    .Build();
```

### �f�o�b�O�\��

```cpp
Entity debugMonitor = world.Create()
    .With<GamepadDebugDisplay>(0)
    .Build();
```

## �{�^���}�b�s���O(Xbox�z�u)

| �{�^�����ʎq | Xbox | PlayStation |
|----------|------|-------------|
| `Button_A` | A | �~(�o�c) |
| `Button_B` | B | ��(�}��) |
| `Button_X` | X | ��(�l�p) |
| `Button_Y` | Y | ��(�O�p) |
| `Button_LB` | LB | L1 |
| `Button_RB` | RB | R1 |
| `Button_Back` | Back/View | Share |
| `Button_Start` | Start/Menu | Options |
| `Button_LS` | ���X�e�B�b�N���� | L3 |
| `Button_RS` | �E�X�e�B�b�N���� | R3 |
| `Button_DPad_Up` | �\���L�[�� | �\���L�[�� |
| `Button_DPad_Down` | �\���L�[�� | �\���L�[�� |
| `Button_DPad_Left` | �\���L�[�� | �\���L�[�� |
| `Button_DPad_Right` | �\���L�[�E | �\���L�[�E |

## �f�b�h�]�[���ݒ�

- **���X�e�B�b�N**: 7849 / 32767 ? 0.24
- **�E�X�e�B�b�N**: 8689 / 32767 ? 0.27
- **�g���K�[**: 30 / 255 ? 0.12

�����̒l��XInput�̕W���I�ȃf�b�h�]�[���ɏ������Ă��܂��B

## �Z�p�d�l

### �Ή��f�o�C�X

- **XInput**: Xbox 360/One/Series �R���g���[���[
- **DirectInput**: ���K�V�[�Q�[���p�b�h�A�t���C�g�X�e�B�b�N�A���̑�

### XInput��DirectInput�̗D�揇��

1. �ŏ���XInput�f�o�C�X(�X���b�g0-3)���`�F�b�N
2. XInput�ŔF������Ȃ��f�o�C�X��DirectInput�ŗ�
3. �e�t���[����XInput�̐ڑ���Ԃ��Ď�
4. DirectInput�f�o�C�X�͏��������Ɍ��o

### �X���b�h�Z�[�t�e�B

- `Update()` �̓��C���X���b�h���疈�t���[��1��Ăяo�����Ƃ�z��
- WMI�ɂ��XInput�f�o�C�X����͏��������̂ݎ��s

## �f�o�b�O���O

`_DEBUG` �r���h�ł́A�ȉ��̏�񂪃��O�o�͂���܂�:

- �Q�[���p�b�h�ڑ�/�ؒf�C�x���g
- �f�o�C�X�������G���[
- DirectInput�f�o�C�X�o�^���

```cpp
#ifdef _DEBUG
DEBUGLOG_CATEGORY(DebugLog::Category::Input, "XInput�f�o�C�X�ڑ�: Index=0");
#endif
```

## ���ӎ���

### XInput�f�o�C�X�̏d�����

DirectInput�ł�XInput�f�o�C�X���񋓂����ꍇ�����邽�߁A`IsXInputDevice()` �֐���WMI���g�p���ăf�o�C�X�𔻒肵�A�d����������Ă��܂��B

### �������x��

DirectInput�f�o�C�X�� `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE` �ŏ���������邽�߁A�E�B���h�E���t�H�[�J�X�������Ă����͂��󂯕t���܂��B

### �U���@�\

�U����XInput�f�o�C�X�̂݃T�|�[�g���܂��BDirectInput�f�o�C�X�� `SetVibration()` ���Ăяo���Ă������N���܂���B

## �t�@�C���\��

```
include/input/
  ������ GamepadSystem.h       # �w�b�_�[�t�@�C��

src/input/
  ������ GamepadSystem.cpp     # �����t�@�C��

include/samples/
  ������ GamepadSample.h       # �g�p��ƃT���v���R���|�[�l���g
```

## ���C�Z���X

���̃R�[�h��HEW_GAME�v���W�F�N�g�̈ꕔ�ł��B

## �쐬��

�R���z - 2025

## �o�[�W��������

- **v6.0** (2025): ���Ń����[�X
  - XInput �� DirectInput �̓���
  - �ő�4�v���C���[�Ή�
  - ECS Behaviour�T���v���ǉ�
