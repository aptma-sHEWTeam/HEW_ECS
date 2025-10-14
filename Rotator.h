#pragma once
#include "Component.h"
#include "Entity.h"
#include "World.h"
#include "Transform.h"
#include <DirectXMath.h>

// ========================================================
// Rotator - ������]���s��Behaviour�i�����̂���R���|�[�l���g�j
// ========================================================
// �y�����z
// �G���e�B�e�B�������I��Y������ŉ�]������
//
// �y�����o�ϐ��z
// - speedDegY: ��]���x�i�x/�b�j
//
// �y�g�����z
// Entity e = world.CreateEntity();
// world.Add<Transform>(e, Transform{...});
// world.Add<Rotator>(e, Rotator{45.0f}); // ���b45�x��]
//
// �y�d�g�݁z
// ���t���[��OnUpdate()���Ă΂�ATransform�̉�]�l���X�V����
// ========================================================
struct Rotator : Behaviour {
    float speedDegY = 45.0f; // Y������̉�]���x�i�x/�b�j

    // �f�t�H���g�R���X�g���N�^
    Rotator() = default;
    
    // ��]���x���w�肷��R���X�g���N�^
    explicit Rotator(float s) : speedDegY(s) {}

    // ���t���[���Ă΂��X�V����
    void OnUpdate(World& w, Entity self, float dt) override {
        // ���̃G���e�B�e�B��Transform���擾
        auto* t = w.TryGet<Transform>(self);
        if (!t) return; // Transform���Ȃ���Ή������Ȃ�
        
        // ��]�l���X�V�idt = �f���^�^�C�� = �O�t���[������̌o�ߕb���j
        t->rotation.y += speedDegY * dt;
        
        // 360�x�𒴂����琳�K���i���₷���̂��߁A�Ȃ��Ă�OK�j
        while (t->rotation.y >= 360.0f) t->rotation.y -= 360.0f;
        while (t->rotation.y < 0.0f) t->rotation.y += 360.0f;
    }
};
