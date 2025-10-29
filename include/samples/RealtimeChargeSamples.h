/**
 * @file RealtimeChargeSamples.h
 * @brief ���A���^�C���`���[�W�V�X�e���̃T���v���R�[�h�W
 * @author �R���z
 * @date 2025
 * @version 6.0
 *
 * @details
 * �`���[�W��Ԃƃ`���[�W���Ԃ����A���^�C���Ŏ擾����T���v�������B
 */

#pragma once
#include "input/GamepadSystem.h"
#include "components/Component.h"
#include "app/DebugLog.h"
#include <DirectXMath.h>
#include <sstream>
#include <cmath>

/**
 * @brief ���A���^�C���`���[�W�Q�[�W�\��
 *
 * @details
 * �X�e�B�b�N���X���Ă���ԁA���t���[���`���[�W�ʂ��擾���ĕ\�����܂��B
 * �Q�[�WUI�A�G�t�F�N�g�A�o�C�u���[�V���������A���^�C���X�V�B
 */
DEFINE_BEHAVIOUR(RealtimeChargeGauge,
    int playerIndex = 0;
    float maxChargeTime = 3.0f;
    float logInterval = 0.5f;
    float logTimer = 0.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

 // �`���[�W���̏���(���t���[�����s)
    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        // ���A���^�C���Ń`���[�W�f�[�^���擾
        float currentTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
        float currentAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        float currentIntensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);

        // ������UI���X�V(���t���[��)
 // DrawChargeGauge(currentAmount);
        // SetGaugeColor(currentIntensity);

        // �ő�`���[�W�Ńo�C�u���[�V����
  if (currentAmount >= 1.0f) {
            GetGamepad().SetVibration(playerIndex, 0.3f, 0.3f);
        }
  else {
  GetGamepad().SetVibration(playerIndex, currentAmount * 0.2f, 0.0f);
        }

        // �f�o�b�O���O(0.5�b����)
        logTimer += dt;
        if (logTimer >= logInterval) {
  logTimer = 0.0f;
        std::ostringstream oss;
   oss << "[���A���^�C��] �`���[�W: "
             << (currentAmount * 100.0f) << "% ("
          << currentTime << "�b, ���x:"
      << (currentIntensity * 100.0f) << "%)";
    DEBUGLOG(oss.str());
        }
 }
    else {
        logTimer = 0.0f;
     GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
      // HideChargeGauge();
    }

    // �����[�X���̏���(1�t���[���̂�)
    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float finalTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
        float finalAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);

        std::ostringstream oss;
        oss << "[�����[�X!] �ŏI�`���[�W: "
   << (finalAmount * 100.0f) << "% (" << finalTime << "�b)";
        DEBUGLOG(oss.str());

        // ���ˏ���
        float power = finalAmount * 20.0f;
   // ShootProjectile(power);

     GetGamepad().SetVibration(playerIndex, 0.8f, 0.8f);
    }
);

/**
 * @brief �`���[�W�i�K�V�X�e��(�と������)
 *
 * @details
 * �`���[�W���Ԃɉ����Ēi�K���ω����A���A���^�C���ŃG�t�F�N�g��؂�ւ��܂��B
 */
DEFINE_BEHAVIOUR(ChargeStageSystem,
    int playerIndex = 0;
    int currentStage = 0;
    float weakThreshold = 0.5f;
  float mediumThreshold = 1.5f;
    float strongThreshold = 2.5f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        // ���A���^�C���Ń`���[�W���Ԃ��擾
        float chargeTime = GetGamepad().GetLeftStickChargeTime(playerIndex);

  // �i�K����(���A���^�C���X�V)
        int newStage = 0;
        if (chargeTime >= strongThreshold) newStage = 3;
        else if (chargeTime >= mediumThreshold) newStage = 2;
        else if (chargeTime >= weakThreshold) newStage = 1;

        // �i�K���オ�����u�Ԃ̏���
        if (newStage > currentStage) {
            currentStage = newStage;

            std::ostringstream oss;
   const char* stageName = "";
            switch (currentStage) {
    case 1: stageName = "��`���[�W"; break;
           case 2: stageName = "���`���[�W"; break;
         case 3: stageName = "���`���[�W"; break;
       }
            oss << "[�i�KUP!] " << stageName << " (����:" << chargeTime << "�b)";
            DEBUGLOG(oss.str());

 // �G�t�F�N�g�؂�ւ�
      // SwitchChargeEffect(currentStage);

            // �o�C�u���[�V�����t�B�[�h�o�b�N
   float vibPower = currentStage * 0.25f;
    GetGamepad().SetVibration(playerIndex, vibPower, vibPower);
        }
    }
    else {
      currentStage = 0;
     GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }

    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        const char* attackName = "";
     switch (currentStage) {
            case 1: attackName = "��U��"; break;
  case 2: attackName = "���U��"; break;
   case 3: attackName = "���U��"; break;
   default: attackName = "���U��"; break;
        }

        std::ostringstream oss;
        oss << "[����!] " << attackName;
  DEBUGLOG(oss.str());

 // ExecuteAttack(currentStage);
    }
);

/**
 * @brief ���A���^�C���p�[�e�B�N���G�~�b�^�[
 *
 * @details
 * �`���[�W�ʂɉ����ăp�[�e�B�N�������p�x�ƌ����ڂ�ω������܂��B
 */
DEFINE_BEHAVIOUR(ChargeParticleEmitter,
    int playerIndex = 0;
    float maxChargeTime = 3.0f;
    float particleTimer = 0.0f;
    float baseInterval = 0.1f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
     // ���A���^�C���Ń`���[�W�f�[�^���擾
        float chargeAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
      float intensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);

        // �`���[�W�ʂŐ����p�x��ω�(0%=10fps, 100%=40fps)
        float interval = baseInterval / (1.0f + chargeAmount * 3.0f);

        particleTimer += dt;
        if (particleTimer >= interval) {
     particleTimer -= interval;

   // �p�[�e�B�N������(�`���[�W�ʂŐF�E�T�C�Y�E���x��ύX)
    // SpawnChargeParticle(chargeAmount, intensity);

   // 25%���ƂɃ��O�o��
      int percentage = static_cast<int>(chargeAmount * 100.0f);
        if (percentage % 25 == 0 && percentage > 0) {
         std::ostringstream oss;
     oss << "[�p�[�e�B�N��] " << percentage << "%�`���[�W";
     // DEBUGLOG(oss.str());  // �p�ɂ�����̂ŃR�����g�A�E�g
         }
        }

        // �o�C�u���[�V�������`���[�W�ʂŕω�
        GetGamepad().SetVibration(playerIndex, chargeAmount * 0.3f, 0.0f);
    }
    else {
        particleTimer = 0.0f;
        GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }
);

/**
 * @brief �`���[�W�����R���g���[���[
 *
 * @details
 * �`���[�W�ʂɉ����ĉ��̃s�b�`�ƃ{�����[�������A���^�C���ω��B
 * �}�C���X�g�[��(25%, 50%, 75%, 100%)��SE�Đ��B
 */
DEFINE_BEHAVIOUR(ChargeAudioController,
    int playerIndex = 0;
    float maxChargeTime = 3.0f;
    float previousAmount = 0.0f;
    bool milestone25 = false;
    bool milestone50 = false;
    bool milestone75 = false;
    bool milestone100 = false;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        // ���A���^�C���Ń`���[�W�ʂ��擾
        float currentAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);

        // ���̃s�b�`��ω�(1.0 �� 1.5)
        float pitch = 1.0f + currentAmount * 0.5f;
        // SetChargeLoopPitch(pitch);

        // ���ʂ�ω�(0.3 �� 1.0)
        float volume = 0.3f + currentAmount * 0.7f;
     // SetChargeLoopVolume(volume);

        // �}�C���X�g�[�����o(25%����)
      if (!milestone25 && currentAmount >= 0.25f) {
            milestone25 = true;
      DEBUGLOG("[����] 25%�`���[�W���B!");
 // PlayChargeMilestone(1);
  GetGamepad().SetVibration(playerIndex, 0.2f, 0.2f);
        }
    if (!milestone50 && currentAmount >= 0.50f) {
   milestone50 = true;
        DEBUGLOG("[����] 50%�`���[�W���B!");
   // PlayChargeMilestone(2);
       GetGamepad().SetVibration(playerIndex, 0.4f, 0.4f);
        }
        if (!milestone75 && currentAmount >= 0.75f) {
            milestone75 = true;
 DEBUGLOG("[����] 75%�`���[�W���B!");
         // PlayChargeMilestone(3);
  GetGamepad().SetVibration(playerIndex, 0.6f, 0.6f);
        }
 if (!milestone100 && currentAmount >= 1.00f) {
 milestone100 = true;
    DEBUGLOG("[����] 100%�`���[�W���B!");
            // PlayChargeMilestone(4);
 GetGamepad().SetVibration(playerIndex, 0.8f, 0.8f);
 }

  previousAmount = currentAmount;
    }
    else {
        // ���Z�b�g
        previousAmount = 0.0f;
 milestone25 = false;
        milestone50 = false;
        milestone75 = false;
   milestone100 = false;
        // StopChargeLoop();
  GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }

 if (GetGamepad().IsLeftStickReleased(playerIndex)) {
   float finalAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        // PlayReleaseSound(finalAmount);
    }
);

/**
 * @brief ���A���^�C���`���[�W�f�o�b�O�\��
 *
 * @details
 * ���ׂẴ`���[�W�f�[�^�����A���^�C���Ń��O�o��(�f�o�b�O�p)�B
 */
DEFINE_BEHAVIOUR(ChargeDebugMonitor,
    int playerIndex = 0;
    float maxChargeTime = 3.0f;
    float updateInterval = 0.1f;
    float updateTimer = 0.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    updateTimer += dt;

    if (GetGamepad().IsLeftStickCharging(playerIndex)) {
        if (updateTimer >= updateInterval) {
            updateTimer = 0.0f;

   // ���ׂẴf�[�^�����A���^�C���擾
            float chargeTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
  float chargeAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        float intensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);
         float stickX = GetGamepad().GetLeftStickX(playerIndex);
        float stickY = GetGamepad().GetLeftStickY(playerIndex);

        std::ostringstream oss;
        oss << "[�f�o�b�O] ����:" << chargeTime << "s"
 << ", ��:" << (chargeAmount * 100.0f) << "%"
     << ", ���x:" << (intensity * 100.0f) << "%"
  << ", �X�e�B�b�N:(" << stickX << ", " << stickY << ")";
DEBUGLOG(oss.str());
        }
    }

    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float finalTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
    float finalAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
        float finalIntensity = GetGamepad().GetLeftStickAverageIntensity(playerIndex);

     std::ostringstream oss;
        oss << "[�����[�X!] �ŏI�f�[�^ - "
       << "����:" << finalTime << "s, "
            << "��:" << (finalAmount * 100.0f) << "%, "
         << "���x:" << (finalIntensity * 100.0f) << "%";
        DEBUGLOG(oss.str());
    }
);

/**
 * @brief ���X�e�B�b�N���A���^�C���Ď�
 *
 * @details
 * ���E�����̃X�e�B�b�N�̃`���[�W��Ԃ����A���^�C���ŊĎ��B
 * �����`���[�W���o�ȂǂɎg�p�B
 */
DEFINE_BEHAVIOUR(DualStickRealtimeMonitor,
    int playerIndex = 0;
    float maxChargeTime = 2.0f;
    float logInterval = 0.5f;
    float logTimer = 0.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    bool leftCharging = GetGamepad().IsLeftStickCharging(playerIndex);
    bool rightCharging = GetGamepad().IsRightStickCharging(playerIndex);

    if (leftCharging || rightCharging) {
        logTimer += dt;
        if (logTimer >= logInterval) {
            logTimer = 0.0f;

            std::ostringstream oss;
         oss << "[���X�e�B�b�N] ";

    if (leftCharging) {
    float leftTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
     float leftAmount = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
            oss << "��:" << (leftAmount * 100.0f) << "% ";
            }
       else {
       oss << "��:-- ";
     }

            if (rightCharging) {
 float rightTime = GetGamepad().GetRightStickChargeTime(playerIndex);
   float rightAmount = GetGamepad().GetRightStickChargeAmount(playerIndex, maxChargeTime);
    oss << "�E:" << (rightAmount * 100.0f) << "%";
            }
            else {
     oss << "�E:--";
     }

            // �����`���[�W���Ȃ���ʂȕ\��
            if (leftCharging && rightCharging) {
      oss << " [�����`���[�W��!]";
      GetGamepad().SetVibration(playerIndex, 0.3f, 0.3f);
            }

        DEBUGLOG(oss.str());
        }
 }
    else {
   logTimer = 0.0f;
   GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
  }
);
