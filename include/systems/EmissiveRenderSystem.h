/**
 * @file EmissiveRenderSystem.h
 * @brief エミッションマテリアル描画システム
 * @version 1.0
 */
#pragma once

#include "ecs/World.h"
#include "graphics/MaterialConstantBuffer.h"
#include <d3d11.h>
#include <wrl/client.h>

class EmissiveRenderSystem {
public:
    static EmissiveRenderSystem& GetInstance();

    bool Initialize(ID3D11Device* device);
    void Shutdown();
    void UpdateMaterial(ID3D11DeviceContext* context, Entity entity, World& world);
    void Bind(ID3D11DeviceContext* context, UINT slot = 2);

private:
    EmissiveRenderSystem() = default;
    ~EmissiveRenderSystem() = default;
    EmissiveRenderSystem(const EmissiveRenderSystem&) = delete;
    EmissiveRenderSystem& operator=(const EmissiveRenderSystem&) = delete;

    Microsoft::WRL::ComPtr<ID3D11Buffer> materialBuffer_;
    MaterialConstantBuffer materialData_;
    bool initialized_ = false;
};
