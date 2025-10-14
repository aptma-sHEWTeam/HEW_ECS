#pragma once
#include <DirectXMath.h>

// ========================================================
// Transform - �ʒu�A��]�A�X�P�[���R���|�[�l���g
// ========================================================
struct Transform {
    DirectX::XMFLOAT3 position{ 0, 0, 5 }; // z=+5 (�J�����O)
    DirectX::XMFLOAT3 rotation{ 0, 0, 0 }; // degrees
    DirectX::XMFLOAT3 scale{ 1, 1, 1 };
};
