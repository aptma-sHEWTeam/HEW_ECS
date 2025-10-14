#pragma once
#include <DirectXMath.h>

// ========================================================
// Transform - �ʒu�E��]�E�X�P�[���R���|�[�l���g
// ========================================================
// �y�����z
// 3D��Ԃł́u�ꏊ�v�u�����v�u�傫���v��\���f�[�^�R���|�[�l���g
//
// �y�����o�ϐ��z
// - position: �ǂ��ɂ��邩�iX, Y, Z���W�j
// - rotation: �ǂ̕����������Ă��邩�i�x���@�j
// - scale: �ǂꂭ�炢�̑傫�����i1.0 = ���{�j
//
// �y�g�����z
// Transform t;
// t.position = DirectX::XMFLOAT3{0, 5, 0};  // Y��������5��
// t.rotation = DirectX::XMFLOAT3{0, 45, 0}; // Y�������45�x��]
// t.scale = DirectX::XMFLOAT3{2, 2, 2};     // 2�{�̑傫��
// ========================================================
struct Transform {
    DirectX::XMFLOAT3 position{ 0, 0, 5 }; // �ʒu�i�f�t�H���g: �J�����O��5m�j
    DirectX::XMFLOAT3 rotation{ 0, 0, 0 }; // ��]�i�x���@: X=�s�b�`, Y=���[, Z=���[���j
    DirectX::XMFLOAT3 scale{ 1, 1, 1 };    // �X�P�[���i1.0 = ���{�j
};
