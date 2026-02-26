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
#include <vector>
#include <array>

static constexpr int MAX_POINT_LIGHTS = 30;

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
            Entity e;
            Transform* t;
            PointLight* pl;
            float distSq;
            float score;
        };
        std::vector<LightCandidate> candidates;
        candidates.reserve(MAX_POINT_LIGHTS * 2);
        std::array<Entity, MAX_POINT_LIGHTS> selectedEntities{};

        auto isSameEntity = [](const Entity& a, const Entity& b) {
            return a.id == b.id && a.gen == b.gen;
        };
        auto wasSelectedLastFrame = [&](const Entity& e) {
            for (int i = 0; i < previousSelectedLightCount_; ++i) {
                if (isSameEntity(previousSelectedLights_[i], e)) {
                    return true;
                }
            }
            return false;
        };
        const float selectionHoldBiasSq = 64.0f;

        world.ForEach<Transform, PointLight>([&](Entity e, Transform& t, PointLight& pl) {
            if (!pl.enabled) return;
            float dx = t.position.x - cameraPos.x;
            float dy = t.position.y - cameraPos.y;
            float dz = t.position.z - cameraPos.z;
            float distSq = dx*dx + dy*dy + dz*dz;
            const bool wasSelected = wasSelectedLastFrame(e);
            const float score = distSq - (wasSelected ? selectionHoldBiasSq : 0.0f);
            candidates.push_back({ e, &t, &pl, distSq, score });
        });

        // Sort by distance (ascending)
        std::sort(candidates.begin(), candidates.end(), [](const LightCandidate& a, const LightCandidate& b) {
            if (a.score != b.score) return a.score < b.score;
            if (a.distSq != b.distSq) return a.distSq < b.distSq;
            if (a.e.id != b.e.id) return a.e.id < b.e.id;
            return a.e.gen < b.e.gen;
        });

        auto WriteLightToBuffer = [&](int index, const LightCandidate& src) {
            auto& dst = lightingData_.pointLights[index];
            dst.position = ApplyPointLightOffset(src.t->position, *src.pl);
            dst.range = src.pl->range;
            dst.color = src.pl->color;
            dst.intensity = src.pl->intensity;
            dst.constantAtt = src.pl->constantAttenuation;
            dst.linearAtt = src.pl->linearAttenuation;
            dst.quadraticAtt = src.pl->quadraticAttenuation;
            dst.enabled = 1.0f;
            selectedEntities[index] = src.e;
        };

        // Fill buffer with closest lights
        int count = 0;
        for (const auto& src : candidates) {
            if (count >= MAX_POINT_LIGHTS) break;
            WriteLightToBuffer(count, src);
            count++;
        }
        lightingData_.activePointLights = count;
        
        // Closest light casts shadow if exists
        if (count > 0) {
            int shadowIndex = -1;
            if (hasPreviousShadowLight_) {
                for (int i = 0; i < count; ++i) {
                    if (isSameEntity(selectedEntities[i], previousShadowLight_)) {
                        shadowIndex = i;
                        break;
                    }
                }
                if (shadowIndex < 0) {
                    for (const auto& src : candidates) {
                        if (!isSameEntity(src.e, previousShadowLight_)) {
                            continue;
                        }
                        const int insertIndex = (count < MAX_POINT_LIGHTS) ? count : (MAX_POINT_LIGHTS - 1);
                        WriteLightToBuffer(insertIndex, src);
                        if (count < MAX_POINT_LIGHTS) {
                            ++count;
                            lightingData_.activePointLights = count;
                        }
                        shadowIndex = insertIndex;
                        break;
                    }
                }
            }
            if (shadowIndex < 0) {
                shadowIndex = 0;
            }
            lightingData_.shadowLightIndex = shadowIndex;
            previousShadowLight_ = selectedEntities[shadowIndex];
            hasPreviousShadowLight_ = true;
        } else {
            hasPreviousShadowLight_ = false;
        }

        previousSelectedLightCount_ = count;
        for (int i = 0; i < count; ++i) {
            previousSelectedLights_[i] = selectedEntities[i];
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
    std::array<Entity, MAX_POINT_LIGHTS> previousSelectedLights_{};
    int previousSelectedLightCount_ = 0;
    Entity previousShadowLight_{};
    bool hasPreviousShadowLight_ = false;
    bool initialized_ = false;
};
