#pragma once
#include <DirectXMath.h>
#include "TextureManager.h"

// ========================================================
// MeshRenderer - ���b�V�������_�����O�R���|�[�l���g
// ========================================================
// �y�����z
// �I�u�W�F�N�g�́u�����ځv���`����f�[�^�R���|�[�l���g
//
// �y�����o�ϐ��z
// - color: ��{�F�iR, G, B: 0.0�`1.0�j
// - texture: �\�ʂɓ\��e�N�X�`���摜
// - uvOffset: �e�N�X�`���̈ʒu���炵�i�A�j���[�V�����p�j
// - uvScale: �e�N�X�`���̊g��k��
//
// �y�g�����z
// MeshRenderer mr;
// mr.color = DirectX::XMFLOAT3{1, 0, 0};  // �ԐF
// mr.texture = texManager.LoadFromFile("brick.png"); // �����K�̃e�N�X�`��
// ========================================================
struct MeshRenderer {
    // �J���[�i�e�N�X�`�����Ȃ��ꍇ�Ɏg�p�A�܂��͐F�t���j
    DirectX::XMFLOAT3 color{ 0.3f, 0.7f, 1.0f }; // �f�t�H���g: ���F
    
    // �e�N�X�`���i�摜��\��t����j
    TextureManager::TextureHandle texture = TextureManager::INVALID_TEXTURE;
    
    // UV���W�̃I�t�Z�b�g�i�e�N�X�`�������炷: UV�A�j���[�V�����p�j
    DirectX::XMFLOAT2 uvOffset{ 0.0f, 0.0f };
    
    // UV���W�̃X�P�[���i�e�N�X�`�����g��k���j
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };
};
