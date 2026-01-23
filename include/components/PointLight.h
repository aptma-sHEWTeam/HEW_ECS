/**
 * @file PointLight.h
 * @brief ポイントライトコンポーネント定義
 * @version 1.0
 */
#pragma once

#include "Component.h"
#include <DirectXMath.h>

/**
 * @brief ポイントライトコンポーネント
 * @details Entityにアタッチして点光源として機能させる
 */
struct PointLight : public IComponent {
    DirectX::XMFLOAT3 color = { 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float range = 10.0f;
    float constantAttenuation = 1.0f;
    float linearAttenuation = 0.09f;
    float quadraticAttenuation = 0.032f;
    DirectX::XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };
    bool enabled = true;

    PointLight() = default;

    PointLight(const DirectX::XMFLOAT3& col, float inten, float rng)
        : color(col), intensity(inten), range(rng) {}

    void SetAttenuation(float constant, float linear, float quadratic) {
        constantAttenuation = constant;
        linearAttenuation = linear;
        quadraticAttenuation = quadratic;
    }
};

inline DirectX::XMFLOAT3 ApplyPointLightOffset(const DirectX::XMFLOAT3& base,
                                               const PointLight& light) {
    return {
        base.x + light.offset.x,
        base.y + light.offset.y,
        base.z + light.offset.z
    };
}
