/**
 * @file LightSystem.h
 * @brief ポイントライト管理システム
 * @version 1.0
 */
#pragma once

#include "ecs/World.h"
#include "graphics/LightConstantBuffer.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>

class LightSystem {
public:
    static LightSystem& GetInstance();

    bool Initialize(ID3D11Device* device);
    void Shutdown();
    void Update(World& world, const DirectX::XMFLOAT3& cameraPos);
    void Bind(ID3D11DeviceContext* context, UINT slot = 1);

    void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity);

private:
    LightSystem() = default;
    ~LightSystem() = default;
    LightSystem(const LightSystem&) = delete;
    LightSystem& operator=(const LightSystem&) = delete;

    Microsoft::WRL::ComPtr<ID3D11Buffer> lightBuffer_;
    LightConstantBuffer lightData_;
    bool initialized_ = false;
};
