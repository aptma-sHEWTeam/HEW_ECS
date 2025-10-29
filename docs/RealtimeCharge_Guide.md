# ���A���^�C���`���[�W�擾�K�C�h

## �T�v

GamepadSystem�̃`���[�W&�����[�X�V�X�e���́A**���A���^�C���Ń`���[�W��Ԃ��擾**�ł��܂��B�X�e�B�b�N���X���Ă���ԁA���t���[���ŐV�̃`���[�W���ԂƏ�Ԃ��擾�\�ł��B

## ���A���^�C���擾API

### �`���[�W��Ԃ̊m�F

```cpp
// �`���[�W�����ǂ���(���t���[���m�F�\)
bool isCharging = GetGamepad().IsLeftStickCharging(0);
```

### �`���[�W���Ԃ̎擾

```cpp
// ���݂̃`���[�W����(�b)�����A���^�C���擾
float chargeTime = GetGamepad().GetLeftStickChargeTime(0);

// �`���[�W���͑�����������
// ��: 0.0s �� 0.016s �� 0.033s �� ... �� 2.5s �� ...
```

### �`���[�W�ʂ̎擾

```cpp
// �`���[�W��(0.0�`1.0)�����A���^�C���擾
float chargeAmount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);

// 3�b��100%�ɒB����
// ��: 0.0 �� 0.01 �� 0.02 �� ... �� 1.0
```

## ���S�ȃ��A���^�C��������

### ���A���^�C���Q�[�W�\��

```cpp
DEFINE_BEHAVIOUR(RealtimeChargeGauge,
    int playerIndex = 0;
  float maxChargeTime = 3.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    // ���A���^�C���Ń`���[�W��Ԃ��擾
    bool isCharging = GetGamepad().IsLeftStickCharging(playerIndex);
    
    if (isCharging) {
  // ���݂̃`���[�W����(���A���^�C���X�V)
        float currentTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
   
  // ���݂̃`���[�W��(���A���^�C���X�V)
        float currentAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        
  // ���݂̕��ϋ��x(���A���^�C���X�V)
        float currentIntensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);
        
   // UI�ɕ\��(���t���[���X�V)
        // DrawChargeGauge(currentAmount);
      // DrawChargeTime(currentTime);
        // DrawIntensityMeter(currentIntensity);
    
        // �ő�`���[�W���̃t�B�[�h�o�b�N
        if (currentAmount >= 1.0f) {
            GetGamepad().SetVibration(playerIndex, 0.3f, 0.3f);
        }
    }
    else {
        GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
        // HideChargeGauge();
    }

    // �����[�X���ɔ���
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
  float finalTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
   float finalAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        
        // ShootProjectile(finalAmount * 20.0f);
    GetGamepad().SetVibration(playerIndex, 0.8f, 0.8f);
    }
);
```

### �`���[�W�i�K�V�X�e��

```cpp
DEFINE_BEHAVIOUR(ChargeStageSystem,
    int playerIndex = 0;
    int currentStage = 0;  // 0=���`���[�W, 1=��, 2=��, 3=��
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
    // ���A���^�C���Ń`���[�W���Ԃ��擾
        float chargeTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
        
        // �`���[�W�i�K�𔻒�(���A���^�C���X�V)
        int newStage = 0;
        if (chargeTime >= 2.5f) newStage = 3;       // ���`���[�W
        else if (chargeTime >= 1.5f) newStage = 2;  // ���`���[�W
        else if (chargeTime >= 0.5f) newStage = 1;  // ��`���[�W
        
        // �i�K���ς�����u�ԂɃt�B�[�h�o�b�N
        if (newStage > currentStage) {
     currentStage = newStage;
    GetGamepad().SetVibration(playerIndex, 0.5f, 0.5f);
            
            // PlayStageUpSound(currentStage);
            // ShowStageEffect(currentStage);
        }
    }
    else {
      currentStage = 0;
    }

    // �����[�X���ɒi�K�ɉ������U��
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        switch (currentStage) {
 case 1: /* ��U�� */ break;
        case 2: /* ���U�� */ break;
            case 3: /* ���U�� */ break;
  }
    }
);
```

### ���A���^�C���p�[�e�B�N������

```cpp
DEFINE_BEHAVIOUR(ChargeParticleEmitter,
    int playerIndex = 0;
    float particleSpawnTimer = 0.0f;
    float particleInterval = 0.1f;  // �p�[�e�B�N�������Ԋu
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        // ���A���^�C���Ń`���[�W�ʂ��擾
        float chargeAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, 3.0f);
 float intensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);
        
      // �`���[�W�ʂɉ����ăp�[�e�B�N�������p�x��ύX
  float adjustedInterval = particleInterval / (1.0f + chargeAmount * 3.0f);
 
    particleSpawnTimer += dt;
  if (particleSpawnTimer >= adjustedInterval) {
            particleSpawnTimer = 0.0f;
            
            // �p�[�e�B�N������(�`���[�W�ʂŐF��傫����ύX)
          // SpawnChargeParticle(chargeAmount, intensity);
        }
        
        // �o�C�u���[�V�������`���[�W�ʂɉ����ĕω�
  GetGamepad().SetVibration(playerIndex, chargeAmount * 0.3f, chargeAmount * 0.3f);
    }
    else {
        particleSpawnTimer = 0.0f;
    GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }
);
```

### ���A���^�C�������t�B�[�h�o�b�N

```cpp
DEFINE_BEHAVIOUR(ChargeAudioController,
  int playerIndex = 0;
    float previousChargeAmount = 0.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        // ���A���^�C���Ń`���[�W�ʂ��擾
        float currentAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, 3.0f);
        
    // �`���[�W�ʂɉ����ăs�b�`��ω�������
        float pitch = 1.0f + currentAmount * 0.5f;  // 1.0 �` 1.5
        // SetChargeLoopPitch(pitch);
     
        // �`���[�W�ʂ̕ω��ɉ����ă{�����[�����ω�
float volume = 0.3f + currentAmount * 0.7f;  // 0.3 �` 1.0
    // SetChargeLoopVolume(volume);
        
        // 25%�A50%�A75%�A100%�Ń����V���b�gSE
        if (previousChargeAmount < 0.25f && currentAmount >= 0.25f) {
        // PlayChargeMilestoneSound(1);
        }
        else if (previousChargeAmount < 0.50f && currentAmount >= 0.50f) {
    // PlayChargeMilestoneSound(2);
        }
        else if (previousChargeAmount < 0.75f && currentAmount >= 0.75f) {
// PlayChargeMilestoneSound(3);
     }
        else if (previousChargeAmount < 1.00f && currentAmount >= 1.00f) {
      // PlayChargeMilestoneSound(4);
            GetGamepad().SetVibration(playerIndex, 0.5f, 0.5f);
 }
     
        previousChargeAmount = currentAmount;
    }
  else {
        previousChargeAmount = 0.0f;
        // StopChargeLoop();
    }

  if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float finalAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, 3.0f);
  // PlayReleaseSound(finalAmount);
    }
);
```

## �f�[�^�X�V�^�C�~���O

### �`���[�W��

```
�t���[��1: IsCharging=true,  Time=0.016s, Amount=0.005
�t���[��2: IsCharging=true,  Time=0.032s, Amount=0.011
�t���[��3: IsCharging=true,  Time=0.048s, Amount=0.016
...
�t���[��60: IsCharging=true, Time=1.000s, Amount=0.333
...
```

### �����[�X��

```
�t���[��180: IsCharging=true,  Released=false, Time=3.000s, Amount=1.000
�t���[��181: IsCharging=false, Released=true,  Time=3.000s, Amount=1.000 �� �����[�X���o
�t���[��182: IsCharging=false, Released=false, Time=0.000s, Amount=0.000 �� ���Z�b�g
```

## �x�X�g�v���N�e�B�X

### ? ���������g����

```cpp
// �`���[�W���̃��A���^�C���X�V
if (GetGamepad().IsLeftStickCharging(0)) {
    float current = GetGamepad().GetLeftStickChargeTime(0);  // ���t���[���ŐV�l
    UpdateUI(current);
}

// �����[�X���̍ŏI�l�擾
if (GetGamepad().IsLeftStickReleased(0)) {
    float final = GetGamepad().GetLeftStickChargeTime(0);    // �����[�X���̒l
    Execute(final);
}
```

### ? ������ׂ��g����

```cpp
// �`���[�W��Ԃ��m�F�����Ɏ��Ԃ��擾
float time = GetGamepad().GetLeftStickChargeTime(0);  // �`���[�W���łȂ����0.0

// �����[�X��̃t���[���Œl���擾
if (GetGamepad().IsLeftStickReleased(0)) {
    // �������Ȃ�
}
// ���̃t���[��
float time = GetGamepad().GetLeftStickChargeTime(0);  // ����0.0�Ƀ��Z�b�g�ς�
```

## �p�t�H�[�}���X�m�[�g

- ���ׂĂ�API�Ăяo����**O(1)**�̒萔����
- ���t���[���Ăяo���Ă����Ȃ�
- �����v�Z�� `Update()` �Ŋ������Ă���Agetter�֐��͒l��Ԃ��̂�

## �܂Ƃ�

| API | ���A���^�C�� | �����[�X�� |
|-----|------------|----------|
| `IsLeftStickCharging()` | ? ���t���[���X�V | ? false |
| `GetLeftStickChargeTime()` | ? ���t���[������ | ? �ŏI�l��1�t���[���ێ� |
| `GetLeftStickChargeAmount()` | ? ���t���[������ | ? �ŏI�l��1�t���[���ێ� |
| `IsLeftStickReleased()` | ? false | ? 1�t���[���̂�true |

����ɂ��A�`���[�W���̃Q�[�W�\���A�i�K�I�ȃG�t�F�N�g�A�����t�B�[�h�o�b�N�ȂǁA���A���^�C���ȉ��o���ȒP�Ɏ����ł��܂�!
