/**
 * @file PlayerComponents.h
 * @brief �v���C���[��p�R���|�[�l���g�W
 * @author �R���z
 * @date 2025
 * @version 1.0
 *
 * @details
 * ���̃t�@�C���̓v���C���[�L�����N�^�[�Ɋ֘A����R���|�[�l���g���`���܂��B
 * �ړ��A�ˌ��A�X�e�[�^�X�Ǘ��Ȃǂ̃v���C���[��p�@�\��񋟂��܂��B
 */
#pragma once

#include "components/Component.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "input/InputSystem.h"
#include "input/GamepadSystem.h"
#include <DirectXMath.h>

// ========================================================
// �v���C���[�ړ��R���|�[�l���g
// ========================================================

/**
 * @struct PlayerMovement
 * @brief �v���C���[�̈ړ����Ǘ�����Behaviour
 *
 * @details
 * �X�e�B�b�N���͂Ɋ�Â��ăv���C���[�̈ړ��𐧌䂵�܂��B
 * �X�e�B�b�N��|���Ă���Ԃ͂��̕����Ɉړ����A�j���[�g�����ɖ߂����ۂɂ͍Ō�ɓ��͂��ꂽ�����Ɋ�Â��Ċ����i�x���V�e�B�j��^���܂��B
 * �܂��A��ʊO�ɏo�Ȃ��悤�Ɏ����I�ɋ��E�������s���܂��B
 *
 * @par �g�p��
 * @code
 * Entity player = world.Create()
 * .With<Transform>()
 * .With<MeshRenderer>()
 * .With<PlayerTag>()
 * .Build();
 *
 * auto& movement = world.Add<PlayerMovement>(player);
 * movement.input_ = &GetInput();
 * movement.speed =8.0f;
 * @endcode
 *
 * @note InputSystem�ւ̎Q�Ƃ�ݒ肷��K�v������܂�
 * @see InputSystem
 */
struct PlayerMovement : Behaviour {
    InputSystem *input_ = nullptr;             ///< ���̓V�X�e���ւ̃|�C���^
    GamepadSystem *gamepad_ = nullptr;         ///< �Q�[���p�b�h�V�X�e���ւ̃|�C���^
    float speed = 5.0f;                        ///< �ړ����x(�P��/�b)
    DirectX::XMFLOAT2 velocity = {0.0f, 0.0f}; ///< ���݂̈ړ��x���V�e�B

    /**
 * @brief ���t���[���X�V����
 * @param[in,out] w ���[���h�Q��
 * @param[in] self ���̃R���|�[�l���g���t���Ă���G���e�B�e�B
 * @param[in] dt �f���^�^�C��(�O�t���[������̌o�ߎ���)
 *
 * @details
 * �X�e�B�b�N���͂�ǂݎ��A�v���C���[�̈ʒu�ƃx���V�e�B���X�V���܂��B
 * ���͂��Ȃ��ꍇ�͍Ō�̃x���V�e�B�Ɋ�Â��Ĉړ��𑱂��܂��B
 */
    void OnUpdate(World &w, Entity self, float dt) override {
        auto *t = w.TryGet<Transform>(self);
        if (!t || (!input_ && !gamepad_))
            return;

        DirectX::XMFLOAT2 inputDir = {0.0f, 0.0f};

        // �L�[�{�[�h���͂̏���
        if (input_) {
            if (input_->GetKey('W') || input_->GetKey(VK_UP)) {
                inputDir.y += 1.0f;
            }
            if (input_->GetKey('S') || input_->GetKey(VK_DOWN)) {
                inputDir.y -= 1.0f;
            }
            if (input_->GetKey('A') || input_->GetKey(VK_LEFT)) {
                inputDir.x -= 1.0f;
            }
            if (input_->GetKey('D') || input_->GetKey(VK_RIGHT)) {
                inputDir.x += 1.0f;
            }
        }

        // �Q�[���p�b�h���͂̏���
        if (gamepad_ && gamepad_->IsConnected(0)) {
            float leftStickX = gamepad_->GetLeftStickX(0);
            float leftStickY = gamepad_->GetLeftStickY(0);

            // �f�b�h�]�[�������ς݂̒l���g�p
            inputDir.x += leftStickX;
            inputDir.y += leftStickY;
        }

        // ���͂�����ꍇ�̓x���V�e�B���X�V
        if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
            velocity = {inputDir.x * speed, inputDir.y * speed};
        }

        // �x���V�e�B�Ɋ�Â��Ĉʒu���X�V
        t->position.x += velocity.x * dt;
        t->position.y += velocity.y * dt;

        // ��ʊO�ɏo�Ȃ��悤�ɐ���
        const float limitX = 8.0f;
        const float limitY = 10.0f;
        if (t->position.x < -limitX)
            t->position.x = -limitX;
        if (t->position.x > limitX)
            t->position.x = limitX;
        if (t->position.y < -limitY)
            t->position.y = -limitY;
        if (t->position.y > limitY)
            t->position.y = limitY;
    }
};
