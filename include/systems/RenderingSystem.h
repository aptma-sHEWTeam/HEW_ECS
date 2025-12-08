/**
 * @file RenderingSystem.h
 * @brief ライトとエミッションを統合した描画システム
 * @version 1.0
 */
#pragma once

#include "ecs/World.h"
#include "components/Transform.h"
#include "components/EmissiveMaterial.h"
#include "components/EmissivePulse.h"
#include "components/PointLight.h"
#include "components/Light.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <cstring>
#include <cmath>

static constexpr int MAX_POINT_LIGHTS = 8;

struct PointLightGPU {
    DirectX::XMFLOAT3 position;
    float range;
    DirectX::XMFLOAT3 color;
    float intensity;
    float constantAtt;
    float linearAtt;
    float quadraticAtt;
    float enabled;
};

struct LightingBuffer {
    DirectX::XMFLOAT3 ambientColor;
    float ambientIntensity;
    DirectX::XMFLOAT3 cameraPosition;
    int activePointLights;
    DirectX::XMFLOAT3 dirLightDirection; // world-space direction of light
    float dirLightEnabled;
    DirectX::XMFLOAT3 dirLightColor;
    float dirLightIntensity;
    PointLightGPU pointLights[MAX_POINT_LIGHTS];
};

struct MaterialBuffer {
    DirectX::XMFLOAT4 diffuseColor;
    DirectX::XMFLOAT3 emissiveColor;
    float emissiveIntensity;
};

class RenderingSystem {
public:
    static RenderingSystem& GetInstance() {
        static RenderingSystem instance;
        return instance;
    }

    bool Initialize(ID3D11Device* device) {
        if (!device) return false;

        std::memset(&lightingData_, 0, sizeof(lightingData_));
        lightingData_.ambientColor = { 0.15f, 0.15f, 0.2f };
        lightingData_.ambientIntensity = 1.0f;
        lightingData_.dirLightEnabled = 0.0f;
        lightingData_.dirLightColor = {1.0f,1.0f,1.0f};
        lightingData_.dirLightIntensity = 1.0f;

        D3D11_BUFFER_DESC lightDesc = {};
        lightDesc.ByteWidth = sizeof(LightingBuffer);
        lightDesc.Usage = D3D11_USAGE_DYNAMIC;
        lightDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        lightDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA lightInit = {};
        lightInit.pSysMem = &lightingData_;

        if (FAILED(device->CreateBuffer(&lightDesc, &lightInit, lightBuffer_.ReleaseAndGetAddressOf()))) {
            return false;
        }

        std::memset(&materialData_, 0, sizeof(materialData_));
        materialData_.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };

        D3D11_BUFFER_DESC matDesc = {};
        matDesc.ByteWidth = sizeof(MaterialBuffer);
        matDesc.Usage = D3D11_USAGE_DYNAMIC;
        matDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        matDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA matInit = {};
        matInit.pSysMem = &materialData_;

        if (FAILED(device->CreateBuffer(&matDesc, &matInit, materialBuffer_.ReleaseAndGetAddressOf()))) {
            return false;
        }

        initialized_ = true;
        return true;
    }

    void Shutdown() {
        lightBuffer_.Reset();
        materialBuffer_.Reset();
        initialized_ = false;
    }

    void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity) {
        lightingData_.ambientColor = color;
        lightingData_.ambientIntensity = intensity;
    }

    void UpdateLights(World& world, const DirectX::XMFLOAT3& cameraPos) {
        if (!initialized_) return;

        lightingData_.cameraPosition = cameraPos;
        lightingData_.activePointLights = 0;

        // Directional light (take the first one)
        lightingData_.dirLightEnabled = 0.0f;
        world.ForEach<DirectionalLight>([this](Entity, DirectionalLight& dl){
            lightingData_.dirLightDirection = dl.direction;
            lightingData_.dirLightColor = { dl.color.x, dl.color.y, dl.color.z };
            lightingData_.dirLightIntensity = std::max(0.0f, dl.color.w);
            lightingData_.dirLightEnabled = 1.0f;
        });

        world.ForEach<Transform, PointLight>([this](Entity, Transform& t, PointLight& pl) {
            if (!pl.enabled) return;
            if (lightingData_.activePointLights >= MAX_POINT_LIGHTS) return;

            int idx = lightingData_.activePointLights;
            lightingData_.pointLights[idx].position = t.position;
            lightingData_.pointLights[idx].range = pl.range;
            lightingData_.pointLights[idx].color = pl.color;
            lightingData_.pointLights[idx].intensity = pl.intensity;
            lightingData_.pointLights[idx].constantAtt = pl.constantAttenuation;
            lightingData_.pointLights[idx].linearAtt = pl.linearAttenuation;
            lightingData_.pointLights[idx].quadraticAtt = pl.quadraticAttenuation;
            lightingData_.pointLights[idx].enabled = 1.0f;

            lightingData_.activePointLights++;
        });
    }

    void UpdateEmissivePulse(World& world, float deltaTime) {
        world.ForEach<EmissiveMaterial, EmissivePulse>([deltaTime](Entity, EmissiveMaterial& em, EmissivePulse& pulse) {
            if (!em.enabled) return;
            pulse.phase += pulse.speed * deltaTime;
            float t = (std::sin(pulse.phase) + 1.0f) * 0.5f;
            em.emissiveIntensity = pulse.minIntensity + t * (pulse.maxIntensity - pulse.minIntensity);
        });
    }

    void SetMaterialForEntity(ID3D11DeviceContext* context, Entity entity, World& world) {
        if (!initialized_ || !context) return;

        materialData_.emissiveColor = { 0.0f, 0.0f, 0.0f };
        materialData_.emissiveIntensity = 0.0f;

        auto* emissive = world.TryGet<EmissiveMaterial>(entity);
        if (emissive && emissive->enabled) {
            materialData_.emissiveColor = emissive->emissiveColor;
            materialData_.emissiveIntensity = emissive->emissiveIntensity;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(context->Map(materialBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, &materialData_, sizeof(materialData_));
            context->Unmap(materialBuffer_.Get(), 0);
        }
    }

    void BindLightBuffer(ID3D11DeviceContext* context, UINT slot = 1) {
        if (!initialized_ || !context) return;

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(context->Map(lightBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, &lightingData_, sizeof(lightingData_));
            context->Unmap(lightBuffer_.Get(), 0);
        }

        context->PSSetConstantBuffers(slot, 1, lightBuffer_.GetAddressOf());
    }

    void BindMaterialBuffer(ID3D11DeviceContext* context, UINT slot = 2) {
        if (!initialized_ || !context) return;
        context->PSSetConstantBuffers(slot, 1, materialBuffer_.GetAddressOf());
    }

private:
    RenderingSystem() = default;
    ~RenderingSystem() = default;
    RenderingSystem(const RenderingSystem&) = delete;
    RenderingSystem& operator=(const RenderingSystem&) = delete;

    Microsoft::WRL::ComPtr<ID3D11Buffer> lightBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> materialBuffer_;
    LightingBuffer lightingData_;
    MaterialBuffer materialData_;
    bool initialized_ = false;
};
