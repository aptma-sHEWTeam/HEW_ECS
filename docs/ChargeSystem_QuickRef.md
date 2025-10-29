# �`���[�W�V�X�e�� �N�C�b�N���t�@�����X

## ��{�I�Ȏg����

### �`���[�W�����ǂ����m�F

```cpp
if (GetGamepad().IsLeftStickCharging(0)) {
    // �`���[�W���̏���(���t���[�����s)
}
```

### �`���[�W���Ԃ��擾(���A���^�C��)

```cpp
float time = GetGamepad().GetLeftStickChargeTime(0);  // �b�P��
```

### �`���[�W�ʂ��擾(���A���^�C���A0.0�`1.0)

```cpp
float amount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);  // 3�b��100%
```

### �����[�X�����o

```cpp
if (GetGamepad().IsLeftStickReleased(0)) {
    // �����[�X���ꂽ�u�Ԃ̏���(1�t���[���̂�)
    float finalTime = GetGamepad().GetLeftStickChargeTime(0);
    float finalAmount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
}
```

## ���S�Ȏ�����

```cpp
DEFINE_BEHAVIOUR(MyChargeSystem,
    int playerIndex = 0;
 float maxChargeTime = 3.0f;
,
    // �`���[�W��: ���A���^�C����UI�X�V
    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
   float amount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        // DrawChargeGauge(amount);  // ���t���[���X�V
        
        if (amount >= 1.0f) {
 GetGamepad().SetVibration(playerIndex, 0.3f, 0.3f);
        }
    }
    else {
    GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }

    // �����[�X��: ����
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float amount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        float power = amount * 20.0f;
        // ShootProjectile(power);
    }
);
```

## ���p�\��API�ꗗ

| API | �߂�l | ���A���^�C�� | ���� |
|-----|--------|------------|------|
| `IsLeftStickCharging(index)` | bool | ? | �`���[�W���� |
| `GetLeftStickChargeTime(index)` | float | ? | �`���[�W����(�b) |
| `GetLeftStickChargeAmount(index, maxTime)` | float | ? | �`���[�W��(0.0�`1.0) |
| `GetLeftStickAverageIntensity(index)` | float | ? | ���ϓ��͋��x |
| `IsLeftStickReleased(index)` | bool | ? | �����[�X���ꂽ��(1�t���[���̂�) |

���E�X�e�B�b�N�����l��API�����p�\

## �悭����g����

### �p�^�[��1: �Q�[�W�\��

```cpp
if (GetGamepad().IsLeftStickCharging(0)) {
    float amount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    DrawChargeGauge(amount);  // 0.0�`1.0���Q�[�W�ɔ��f
}
```

### �p�^�[��2: �i�K�I�ȃG�t�F�N�g

```cpp
float time = GetGamepad().GetLeftStickChargeTime(0);
if (time >= 2.5f) {
    ShowMaxChargeEffect();
}
else if (time >= 1.5f) {
    ShowMediumChargeEffect();
}
```

### �p�^�[��3: ���̃s�b�`�ω�

```cpp
if (GetGamepad().IsLeftStickCharging(0)) {
    float amount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    float pitch = 1.0f + amount * 0.5f;  // 1.0�`1.5
    SetSoundPitch(pitch);
}
```

## �T���v���R�[�h

- `include/samples/ChargeSystemSample.h` - ��{��
- `include/samples/RealtimeChargeSamples.h` - ���A���^�C���X�V��

## �h�L�������g

- `docs/ChargeSystem_Guide.md` - �ڍ׃K�C�h
- `docs/RealtimeCharge_Guide.md` - ���A���^�C���擾�K�C�h
- `docs/GamepadSystem_README.md` - �V�X�e���S�̂̃h�L�������g
