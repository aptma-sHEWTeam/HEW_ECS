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

static constexpr int MAX_POINT_LIGHTS = 64;

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
    int shadowLightIndex;
    DirectX::XMFLOAT3 paddingLight;
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
    
    // Access methods for ShadowRenderSystem
    int GetActiveLightCount() const { return lightingData_.activePointLights; }
    int GetShadowLightIndex() const { return lightingData_.shadowLightIndex; }
    PointLightGPU GetLightGPU(int index) const { 
        if(index >=0 && index < MAX_POINT_LIGHTS) return lightingData_.pointLights[index];
        return {};
    }

    bool Initialize(ID3D11Device* device) {
        if (!device) return false;

        std::memset(&lightingData_, 0, sizeof(lightingData_));
        lightingData_.ambientColor = { 0.15f, 0.15f, 0.2f };
        lightingData_.ambientIntensity = 1.0f;
        lightingData_.dirLightEnabled = 0.0f;
        lightingData_.dirLightColor = {1.0f,1.0f,1.0f};
        lightingData_.dirLightIntensity = 1.0f;
        lightingData_.shadowLightIndex = -1; // Default none

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
        lightingData_.shadowLightIndex = -1;

        // Directional light (take the first one)
        lightingData_.dirLightEnabled = 0.0f;
        world.ForEach<DirectionalLight>([this](Entity, DirectionalLight& dl){
            lightingData_.dirLightDirection = dl.direction;
            lightingData_.dirLightColor = { dl.color.x, dl.color.y, dl.color.z };
            lightingData_.dirLightIntensity = std::max(0.0f, dl.color.w);
            lightingData_.dirLightEnabled = 1.0f;
        });

        // Collect and sort Point Lights
        struct LightCandidate {
            Transform* t;
            PointLight* pl;
            float distSq;
        };
        std::vector<LightCandidate> candidates;
        candidates.reserve(MAX_POINT_LIGHTS * 2);

        world.ForEach<Transform, PointLight>([&](Entity, Transform& t, PointLight& pl) {
            if (!pl.enabled) return;
            float dx = t.position.x - cameraPos.x;
            float dy = t.position.y - cameraPos.y;
            float dz = t.position.z - cameraPos.z;
            float distSq = dx*dx + dy*dy + dz*dz;
            candidates.push_back({ &t, &pl, distSq });
        });

        // Sort by distance (ascending)
        std::sort(candidates.begin(), candidates.end(), [](const LightCandidate& a, const LightCandidate& b) {
            return a.distSq < b.distSq;
        });

        // Fill buffer with closest lights
        int count = 0;
        for (const auto& src : candidates) {
            if (count >= MAX_POINT_LIGHTS) break;
            
            auto& dst = lightingData_.pointLights[count];
              dst.position = ApplyPointLightOffset(src.t->position, *src.pl);
            dst.range = src.pl->range;
            dst.color = src.pl->color;
            dst.intensity = src.pl->intensity;
            dst.constantAtt = src.pl->constantAttenuation;
            dst.linearAtt = src.pl->linearAttenuation;
            dst.quadraticAtt = src.pl->quadraticAttenuation;
            dst.enabled = 1.0f;

            count++;
        }
        lightingData_.activePointLights = count;
        
        // Closest light casts shadow if exists
        if (count > 0) {
            lightingData_.shadowLightIndex = 0;
        }
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
