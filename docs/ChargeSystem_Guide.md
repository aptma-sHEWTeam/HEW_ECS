# �`���[�W&�����[�X�V�X�e�� - �Q�[�����C���V�X�e��

## �T�v

**�`���[�W&�����[�X�V�X�e��**�́A�X�e�B�b�N���X���ă`���[�W���A�j���[�g�����ɖ߂����Ƃŉ������A�Q�[���̃��C���V�X�e���ł��B�ȒP�Ɏ����ł���悤�A��pAPI��񋟂��Ă��܂��B

## ��{�I�Ȏg����

### �`���[�W���o

```cpp
// �X�e�B�b�N���`���[�W�����ǂ���
if (GetGamepad().IsLeftStickCharging(0)) {
    // �`���[�W���̏���
}
```

### �����[�X���o

```cpp
// �X�e�B�b�N�������[�X���ꂽ�u��
if (GetGamepad().IsLeftStickReleased(0)) {
    // ������̏���
}
```

### �`���[�W�ʂ̎擾

```cpp
// �`���[�W�ʂ�0.0�`1.0�Ŏ擾
float chargeAmount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);  // 3�b�ōő�
```

## ���S�Ȏ�����

### ��{�I�Ȏˌ��V�X�e��

```cpp
DEFINE_BEHAVIOUR(ChargeShootController,
    int playerIndex = 0;
    float maxChargeTime = 3.0f;
    float maxPower = 20.0f;
    float minPower = 5.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    // �`���[�W��
    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
   float charge = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        
        // UI�Ƀ`���[�W�Q�[�W��\��
 // DrawChargeGauge(charge);
        
     // �ő�`���[�W�Ńo�C�u���[�V����
        if (charge >= 1.0f) {
 GetGamepad().SetVibration(playerIndex, 0.3f, 0.3f);
        }
    }
    else {
        GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }

    // �����[�X���ɔ���
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
     float charge = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        float power = minPower + (maxPower - minPower) * charge;
        
   // �e�𔭎�
        // world.EnqueueSpawn<Projectile>(transform->position, power);
      
        // ���˃G�t�F�N�g
        GetGamepad().SetVibration(playerIndex, 0.8f, 0.8f);
    }
);
```

## ���p�\��API

### �`���[�W��Ԃ̎擾

| API | ���� | �߂�l |
|-----|------|--------|
| `IsLeftStickCharging(index)` | ���X�e�B�b�N���`���[�W���� | bool |
| `IsRightStickCharging(index)` | �E�X�e�B�b�N���`���[�W���� | bool |
| `IsLeftStickReleased(index)` | ���X�e�B�b�N�������[�X���ꂽ�� | bool |
| `IsRightStickReleased(index)` | �E�X�e�B�b�N�������[�X���ꂽ�� | bool |

### �`���[�W�f�[�^�̎擾

| API | ���� | �߂�l |
|-----|------|--------|
| `GetLeftStickChargeTime(index)` | ���X�e�B�b�N�̃`���[�W����(�b) | float |
| `GetRightStickChargeTime(index)` | �E�X�e�B�b�N�̃`���[�W����(�b) | float |
| `GetLeftStickChargeAmount(index, maxTime)` | ���X�e�B�b�N�̃`���[�W��(0.0�`1.0) | float |
| `GetRightStickChargeAmount(index, maxTime)` | �E�X�e�B�b�N�̃`���[�W��(0.0�`1.0) | float |
| `GetLeftStickAverageIntensity(index)` | ���X�e�B�b�N�̕��ϓ��͋��x | float |
| `GetRightStickAverageIntensity(index)` | �E�X�e�B�b�N�̕��ϓ��͋��x | float |

## ���x�Ȏg�p��

### �_�u���X�e�B�b�N�����`���[�W

```cpp
DEFINE_BEHAVIOUR(DualChargeController,
    int playerIndex = 0;
    float syncWindow = 0.2f;  // ��������̎��ԑ�
    float lastLeftRelease = -999.0f;
float lastRightRelease = -999.0f;
    float totalTime = 0.0f;
,
    totalTime += dt;

    // �������[�X
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
      lastLeftRelease = totalTime;
     
        // �E���ŋ߃����[�X����Ă����瓯���U��
        if ((totalTime - lastRightRelease) < syncWindow) {
    float leftCharge = GetGamepad().GetLeftStickChargeAmount(playerIndex, 2.0f);
     float rightCharge = GetGamepad().GetRightStickChargeAmount(playerIndex, 2.0f);
     float combinedPower = (leftCharge + rightCharge) * 15.0f;
     
            // ���͂ȍU���𔭓�
          // ExecuteSpecialAttack(combinedPower);
        }
    }

    if (GetGamepad().IsRightStickReleased(playerIndex)) {
        lastRightRelease = totalTime;
    }
);
```

### �^�C�~���O����V�X�e��

```cpp
DEFINE_BEHAVIOUR(ChargeTimingController,
    int playerIndex = 0;
    float perfectTiming = 1.5f;      // �����ȃ^�C�~���O(�b)
    float goodWindow = 0.2f;         // GOOD����̑�
  float greatWindow = 0.1f; // GREAT����̑�
,
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float chargeTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
      float diff = fabsf(chargeTime - perfectTiming);
        
        if (diff < greatWindow) {
       // PERFECT! �{�[�i�X x2.0
  GetGamepad().SetVibration(playerIndex, 1.0f, 1.0f);
        }
        else if (diff < goodWindow) {
            // GOOD! �{�[�i�X x1.5
 GetGamepad().SetVibration(playerIndex, 0.5f, 0.5f);
        }
 else {
        // MISS...
        }
    }
);
```

### ���͋��x���l�������V�X�e��

```cpp
if (GetGamepad().IsLeftStickReleased(0)) {
    float chargeAmount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    float avgIntensity = GetGamepad().GetLeftStickAverageIntensity(0);
    
    // �`���[�W���ԂƓ��͋��x�̗������l��
    float finalPower = chargeAmount * (0.5f + avgIntensity * 0.5f) * 20.0f;
    
 // �X�e�B�b�N�������|���������ꍇ�A��苭�͂ɂȂ�
    // ShootProjectile(finalPower);
}
```

## �Z�p�d�l

### �`���[�W���o臒l

- **�f�t�H���g臒l**: 0.1 (�X�e�B�b�N�̌X����10%�ȏ�Ń`���[�W�Ɣ���)
- **�萔**: `GamepadSystem::CHARGE_DETECTION_THRESHOLD`

### �v���f�[�^

�`���[�W���A�ȉ��̃f�[�^�������I�ɋL�^����܂�:

1. **�`���[�W����**: �X�e�B�b�N���X���n�߂Ă���̌o�ߎ���(�b)
2. **���͋��x�̗ݐ�**: �X�e�B�b�N�̌X����̍��v
3. **�T���v����**: �v���t���[����

### �����[�X����

- **�O�t���[��**: �`���[�W��
- **���̃t���[��**: �j���[�g����(臒l�ȉ�)

���̏����𖞂��������A`IsLeftStickReleased()` �� **1�t���[������** `true` ��Ԃ��܂��B

### �f�[�^�̃��Z�b�g

�`���[�W�f�[�^�͈ȉ��̃^�C�~���O�Ń��Z�b�g����܂�:

1. �X�e�B�b�N���j���[�g�����ɖ߂��� **2�t���[���o�ߌ�**
2. �V���Ƀ`���[�W���J�n������

����ɂ��A�����[�X����̃t���[���ł����m�ȃ`���[�W�ʂ��擾�ł��܂��B

## �f�o�b�O�ƃ`���[�j���O

### �`���[�W���̃��O�o��

```cpp
if (GetGamepad().IsLeftStickCharging(0)) {
  float time = GetGamepad().GetLeftStickChargeTime(0);
    float amount = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    float intensity = GetGamepad().GetLeftStickAverageIntensity(0);
 
    std::ostringstream oss;
    oss << "�`���[�W: " << (amount * 100.0f) << "% "
        << "(" << time << "�b, ���x:" << (intensity * 100.0f) << "%)";
 DEBUGLOG(oss.str());
}
```

### �����p�����[�^

| �Q�[���W������ | �ő�`���[�W���� | �����ȃ^�C�~���O | �p�r |
|--------------|-----------------|-----------------|------|
| �A�N�V���� | 1.0�`2.0�b | 0.8�`1.2�b | �f�������� |
| �p�Y�� | 2.0�`3.0�b | 1.5�`2.5�b | ��������l���� |
| ���Y���Q�[�� | 0.5�`1.5�b | 1.0�b | ���y�ɍ��킹�� |
| �V���[�e�B���O | 1.5�`3.0�b | �Ȃ� | �`���[�W�ʏd�� |

## �x�X�g�v���N�e�B�X

### ? ����

```cpp
// �����[�X���Ƀ`���[�W�ʂ��擾
if (GetGamepad().IsLeftStickReleased(0)) {
    float charge = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    Shoot(charge);
}

// �`���[�W���Ƀ��A���^�C���\��
if (GetGamepad().IsLeftStickCharging(0)) {
  float charge = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);
    UpdateChargeGaugeUI(charge);
}
```

### ? �񐄏�

```cpp
// �����[�X��Ƀ`���[�W�ʂ��擾���悤�Ƃ���(0�ɂȂ��Ă���)
if (GetGamepad().IsLeftStickReleased(0)) {
    // �������Ȃ�
}
// ���̃t���[��
float charge = GetGamepad().GetLeftStickChargeAmount(0, 3.0f);  // 0.0!
```

## �g���u���V���[�e�B���O

### Q: �`���[�W�ʂ����0�ɂȂ�

A: `IsLeftStickReleased()` �� `true` �� **�����t���[��** �� `GetLeftStickChargeAmount()` ���Ăяo���Ă��������B

### Q: �����ȃX�e�B�b�N����ł��`���[�W���n�܂�

A: `CHARGE_DETECTION_THRESHOLD` �̒l��傫�����܂�(�f�t�H���g0.1��0.2�Ȃ�)�B

### Q: �����[�X���肪2�񔭐�����

A: �����[�X��1�t���[���̂� `true` ��Ԃ��̂ŁA�t���O�ŊǗ����Ă��������B

## �T���v���R�[�h

���S�Ȏ�����͈ȉ��̃t�@�C�����Q�Ƃ��Ă�������:

- `include/samples/ChargeSystemSample.h` - ��{�I�Ȏg�p��
- `include/samples/GamepadSample.h` - ���p��

## �쐬��

�R���z - 2025
