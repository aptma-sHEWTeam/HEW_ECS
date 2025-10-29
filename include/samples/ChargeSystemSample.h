/**
 * @file ChargeSystemSample.h
 * @brief �`���[�W&�����[�X�V�X�e���̃T���v���R�[�h
 * @author �R���z
 * @date 2025
 * @version 6.0
 * 
 * @details
 * �Q�[���̃��C���V�X�e���ł���u�X�e�B�b�N���X���ă`���[�W�������ĉ���v
 * �̎�����������܂��B
 */

#pragma once
#include "input/GamepadSystem.h"
#include "components/Component.h"
#include "app/DebugLog.h"
#include <DirectXMath.h>
#include <sstream>
#include <cmath>

/**
 * @brief ��{�I�ȃ`���[�W�ˌ��V�X�e��
 * 
 * @details
 * ���X�e�B�b�N���X���ă`���[�W�A�����Ɣ��˂��܂��B
 * �`���[�W���Ԃɉ����Ďˌ��p���[���ω����܂��B
 */
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
        
 std::ostringstream oss;
        oss << "����! �p���[: " << power;
        DEBUGLOG(oss.str());
        
    GetGamepad().SetVibration(playerIndex, 0.8f, 0.8f);
    }
);

/**
 * @brief �_�u���X�e�B�b�N�`���[�W
 * 
 * @details
 * ���E�����𓯎��Ƀ����[�X����Ƌ��͂ȍU���B
 */
DEFINE_BEHAVIOUR(DualChargeController,
  int playerIndex = 0;
    float maxChargeTime = 2.0f;
    float syncWindow = 0.2f;
    float lastLeftRelease = -999.0f;
    float lastRightRelease = -999.0f;
    float totalTime = 0.0f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;
    totalTime += dt;

    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
      lastLeftRelease = totalTime;
      
        if ((totalTime - lastRightRelease) < syncWindow) {
            float leftCharge = GetGamepad().GetLeftStickChargeAmount(playerIndex, maxChargeTime);
      float rightCharge = GetGamepad().GetRightStickChargeAmount(playerIndex, maxChargeTime);
 float power = (leftCharge + rightCharge) * 15.0f;

            std::ostringstream oss;
            oss << "�����U��! �p���[: " << power;
   DEBUGLOG(oss.str());
            
            GetGamepad().SetVibration(playerIndex, 1.0f, 1.0f);
        }
    }

    if (GetGamepad().IsRightStickReleased(playerIndex)) {
        lastRightRelease = totalTime;
    }
);

/**
 * @brief �^�C�~���O����V�X�e��
 * 
 * @details
 * ����̃^�C�~���O�Ń����[�X����ƃ{�[�i�X�B
 */
DEFINE_BEHAVIOUR(ChargeTimingController,
    int playerIndex = 0;
    float perfectTime = 1.5f;
    float goodWindow = 0.2f;
    float greatWindow = 0.1f;
,
    if (!GetGamepad().IsConnected(playerIndex)) return;

    if (GetGamepad().IsLeftStickReleased(playerIndex)) {
        float chargeTime = GetGamepad().GetLeftStickChargeTime(playerIndex);
        float diff = fabsf(chargeTime - perfectTime);
        
        const char* judgment = "MISS";
        if (diff < greatWindow) {
          judgment = "PERFECT";
            GetGamepad().SetVibration(playerIndex, 1.0f, 1.0f);
        }
  else if (diff < goodWindow) {
            judgment = "GOOD";
      GetGamepad().SetVibration(playerIndex, 0.5f, 0.5f);
   }
        
        std::ostringstream oss;
        oss << "����: " << judgment;
        DEBUGLOG(oss.str());
    }
);
