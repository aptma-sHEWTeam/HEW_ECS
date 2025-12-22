/**
 * @file LightConstantBuffer.h
 * @brief ライト用定数バッファ構造体
 * @version 1.0
 */
#pragma once

#include <DirectXMath.h>

static constexpr int MAX_POINT_LIGHTS = 64;

struct PointLightData {
    DirectX::XMFLOAT3 position;
    float range;
    DirectX::XMFLOAT3 color;
    float intensity;
    float constantAtt;
    float linearAtt;
    float quadraticAtt;
    float enabled;
};

struct LightConstantBuffer {
    DirectX::XMFLOAT3 ambientColor;
    float ambientIntensity;
    DirectX::XMFLOAT3 cameraPosition;
    int activePointLights;
    PointLightData pointLights[MAX_POINT_LIGHTS];
};
