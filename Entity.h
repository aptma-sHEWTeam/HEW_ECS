#pragma once
#include <cstdint>

// ========================================================
// Entity - ECS�̃G���e�B�e�B�i�Q�[���I�u�W�F�N�g�j
// ========================================================
// �y�G���e�B�e�B�Ƃ́H�z
// �Q�[�����E�ɑ��݂���u���m�v��\���P�Ȃ�ID�ԍ�
// �G���e�B�e�B���̂ɂ͋@�\���Ȃ��A�R���|�[�l���g�����t���邱�Ƃŋ@�\������
//
// �y��z
// - �v���C���[ = Entity(id=1) + Transform + MeshRenderer + PlayerController
// - �G = Entity(id=2) + Transform + MeshRenderer + EnemyAI
// - �e = Entity(id=3) + Transform + MeshRenderer + Bullet
// ========================================================
struct Entity {
    uint32_t id; // �G���e�B�e�B�̎��ʔԍ��iWorld�����j�[�N��ID���������蓖�āj
};
