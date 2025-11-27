#pragma once
#include <DirectXMath.h>

/**
 * @file Light.h
 * @brief ライト（光源）コンポーネントの定義
 * @author 山内陽
 * @date 2025
 * @version 6.0
 */

// 現状は指向性ライトのみを想定
struct DirectionalLight {
    DirectX::XMFLOAT3 direction{ 0.577f, -0.577f, 0.577f }; // デフォルトのライト方向
    float padding; // 16バイトアライメント用
    DirectX::XMFLOAT4 color{ 1.0f, 0.0f, 0.0f, 0.0f };      // ライトの色
};