/**
 * @file MaterialConstantBuffer.h
 * @brief マテリアル用定数バッファ構造体
 * @version 1.0
 */
#pragma once

#include <DirectXMath.h>

struct MaterialConstantBuffer {
    DirectX::XMFLOAT4 diffuseColor;
    DirectX::XMFLOAT4 specularColor;
    DirectX::XMFLOAT3 emissiveColor;
    float emissiveIntensity;
    float shininess;
    float padding[3];
};
