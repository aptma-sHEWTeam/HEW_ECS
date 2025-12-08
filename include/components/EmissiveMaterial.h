/**
 * @file EmissiveMaterial.h
 * @brief エミッション（自己発光）マテリアルコンポーネント
 * @version 1.0
 */
#pragma once

#include "Component.h"
#include <DirectXMath.h>

/**
 * @brief エミッションマテリアルコンポーネント
 * @details オブジェクトに自己発光効果を付与する
 */
struct EmissiveMaterial : public IComponent {
    DirectX::XMFLOAT3 emissiveColor = { 1.0f, 1.0f, 1.0f };
    float emissiveIntensity = 1.0f;
    bool enabled = true;

    EmissiveMaterial() = default;

    EmissiveMaterial(const DirectX::XMFLOAT3& color, float intensity)
        : emissiveColor(color), emissiveIntensity(intensity) {}

    void SetColor(float r, float g, float b) {
        emissiveColor = { r, g, b };
    }

    void SetIntensity(float intensity) {
        emissiveIntensity = intensity;
    }
};
