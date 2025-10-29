/**
 * @file GamepadSample.h
 * @brief �Q�[���p�b�h���̓V�X�e���̎g�p��
 * @author �R���z
 * @date 2025
 * @version 6.0
 * 
 * @details
 * GamepadSystem�̎g�p���@�������T���v���R�[�h�ł��B
 */

#pragma once
#include "input/GamepadSystem.h"
#include "components/Component.h"
#include "app/DebugLog.h"
#include <DirectXMath.h>
#include <sstream>
#include <cmath>

/**
 * @brief �Q�[���p�b�h�Ńv���C���[�𑀍삷��Behaviour
 * 
 * @details
 * �Q�[���p�b�h�̍��X�e�B�b�N�ňړ��AA�{�^���ŃW�����v�A
 * �E�g���K�[�Ń_�b�V�����s���܂��B
 * 
 * @par �g�p��
 * @code
 * Entity player = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0,0,0})
 *     .With<GamepadPlayerController>(0) // �v���C���[0
 *     .Build();
 * @endcode
 */
DEFINE_BEHAVIOUR(GamepadPlayerController,
    int playerIndex = 0;    ///< �Q�[���p�b�h�C���f�b�N�X(0-3)
    float moveSpeed = 5.0f;     ///< �ړ����x
    float dashSpeed = 10.0f;    ///< �_�b�V�����x
    float rotateSpeed = 180.0f; ///< ��]���x(�x/�b)
    bool isJumping = false;     ///< �W�����v���t���O
,
  // Transform���擾
    auto* transform = w.TryGet<Transform>(self);
    if (!transform) return;

    // �Q�[���p�b�h�ڑ��m�F
    if (!GetGamepad().IsConnected(playerIndex)) {
        // �ؒf��
        return;
    }

    // ���X�e�B�b�N�ňړ�
    float leftX = GetGamepad().GetLeftStickX(playerIndex);
    float leftY = GetGamepad().GetLeftStickY(playerIndex);

    // �ړ����x������(�E�g���K�[�Ń_�b�V��)
  float currentSpeed = moveSpeed;
    float rightTrigger = GetGamepad().GetRightTrigger(playerIndex);
    if (rightTrigger > 0.5f) {
        currentSpeed = dashSpeed;
    }

    // �ړ���K�p
    transform->position.x += leftX * currentSpeed * dt;
    transform->position.z += leftY * currentSpeed * dt;

    // �ړ������ɉ�]
    if (leftX != 0.0f || leftY != 0.0f) {
      float targetAngle = atan2f(leftX, leftY);
        float currentAngle = transform->rotation.y;
        
        // �p�x�����v�Z(-�� �` +��)
        float diff = targetAngle - currentAngle;
     while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
        while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

        // ���炩�ɉ�]
    float maxRotate = rotateSpeed * dt * DirectX::XM_PI / 180.0f;
        if (fabsf(diff) < maxRotate) {
      transform->rotation.y = targetAngle;
    } else {
  transform->rotation.y += (diff > 0 ? maxRotate : -maxRotate);
    }
    }

    // A�{�^���ŃW�����v
 if (GetGamepad().GetButtonDown(playerIndex, GamepadSystem::Button_A)) {
   if (!isJumping) {
       transform->position.y += 2.0f; // �ȈՃW�����v
isJumping = true;
  }
    }

    // �n�ʂɒ��n������W�����v�t���O���Z�b�g(�ȈՎ���)
    if (transform->position.y <= 0.0f) {
  transform->position.y = 0.0f;
      isJumping = false;
    } else if (isJumping) {
 // �d�͓K�p
    transform->position.y -= 9.8f * dt;
    }

    // B�{�^���Ńo�C�u���[�V����
    if (GetGamepad().GetButton(playerIndex, GamepadSystem::Button_B)) {
   GetGamepad().SetVibration(playerIndex, 0.5f, 0.5f);
    } else {
        GetGamepad().SetVibration(playerIndex, 0.0f, 0.0f);
    }
);

/**
 * @brief �Q�[���p�b�h�Ŏ��_�𑀍삷��Behaviour
 * 
 * @details
 * �Q�[���p�b�h�̉E�X�e�B�b�N�ŃJ�����̉�]�𐧌䂵�܂��B
 * 
 * @par �g�p��
 * @code
 * Entity camera = world.Create()
 *   .With<Transform>(DirectX::XMFLOAT3{0,2,-5})
 *     .With<GamepadCameraController>(0)
 *     .Build();
 * @endcode
 */
DEFINE_BEHAVIOUR(GamepadCameraController,
    int playerIndex = 0;     ///< �Q�[���p�b�h�C���f�b�N�X
    float sensitivity = 2.0f;       ///< ���x
    float minPitch = -80.0f;        ///< �ŏ��s�b�`�p(�x)
    float maxPitch = 80.0f;         ///< �ő�s�b�`�p(�x)
,
    auto* transform = w.TryGet<Transform>(self);
    if (!transform) return;

    if (!GetGamepad().IsConnected(playerIndex)) return;

    // �E�X�e�B�b�N�Ŏ��_��]
    float rightX = GetGamepad().GetRightStickX(playerIndex);
    float rightY = GetGamepad().GetRightStickY(playerIndex);

    // ���[(Y����])
    transform->rotation.y += rightX * sensitivity * dt;

    // �s�b�`(X����])�𐧌��t���ōX�V
    float pitchDelta = rightY * sensitivity * dt;
    float newPitch = transform->rotation.x + pitchDelta;
    
    float minPitchRad = minPitch * DirectX::XM_PI / 180.0f;
    float maxPitchRad = maxPitch * DirectX::XM_PI / 180.0f;
    
    if (newPitch < minPitchRad) newPitch = minPitchRad;
    if (newPitch > maxPitchRad) newPitch = maxPitchRad;
    
    transform->rotation.x = newPitch;
);

/**
 * @brief �Q�[���p�b�h�̓��͏�Ԃ�\������Behaviour
 * 
 * @details
 * �f�o�b�O�p:�Q�[���p�b�h�̑S�{�^���ƃX�e�B�b�N�̏�Ԃ����O�o�͂��܂��B
 */
DEFINE_BEHAVIOUR(GamepadDebugDisplay,
    int playerIndex = 0;
    float logInterval = 1.0f;  ///< ���O�o�͊Ԋu(�b)
    float timer = 0.0f;
,
    timer += dt;
    
    if (timer >= logInterval) {
     timer = 0.0f;

        if (!GetGamepad().IsConnected(playerIndex)) {
     DEBUGLOG("�Q�[���p�b�h���ڑ�");
         return;
        }

 // �{�^����Ԃ����O�o��
        if (GetGamepad().GetButton(playerIndex, GamepadSystem::Button_A)) {
          DEBUGLOG("A�{�^��������");
        }
  if (GetGamepad().GetButtonDown(playerIndex, GamepadSystem::Button_B)) {
     DEBUGLOG("B�{�^�������ꂽ");
  }

        // �X�e�B�b�N�l�����O�o��
        float lx = GetGamepad().GetLeftStickX(playerIndex);
        float ly = GetGamepad().GetLeftStickY(playerIndex);
        if (lx != 0.0f || ly != 0.0f) {
       std::ostringstream oss;
oss << "���X�e�B�b�N: X=" << lx << ", Y=" << ly;
DEBUGLOG(oss.str());
        }

      // �g���K�[�l�����O�o��
        float lt = GetGamepad().GetLeftTrigger(playerIndex);
        float rt = GetGamepad().GetRightTrigger(playerIndex);
      if (lt > 0.0f || rt > 0.0f) {
        std::ostringstream oss;
         oss << "�g���K�[: L=" << lt << ", R=" << rt;
      DEBUGLOG(oss.str());
        }
    }
);

/**
 * @brief �����v���C���[�p�̃Q�[���p�b�h�ڑ��Ď�
 * 
 * @details
 * 4�l�܂ł̃v���C���[�̃Q�[���p�b�h�ڑ���Ԃ��Ď����A
 * �ڑ�/�ؒf���Ƀ��O���o�͂��܂��B
 */
DEFINE_DATA_COMPONENT(GamepadConnectionMonitor,
    bool wasConnected[4] = {false, false, false, false};

    void Update() {
        for (int i = 0; i < 4; ++i) {
         bool isNowConnected = GetGamepad().IsConnected(i);
       
     if (isNowConnected && !wasConnected[i]) {
     // �ڑ����ꂽ
        std::ostringstream oss;
oss << "�v���C���[" << i << "�̃Q�[���p�b�h�ڑ�";
                DEBUGLOG(oss.str());
  wasConnected[i] = true;
   }
       else if (!isNowConnected && wasConnected[i]) {
         // �ؒf���ꂽ
       std::ostringstream oss;
         oss << "�v���C���[" << i << "�̃Q�[���p�b�h�ؒf";
   DEBUGLOG(oss.str());
                wasConnected[i] = false;
        }
      }
    }
);
