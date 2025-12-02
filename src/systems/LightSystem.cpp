#include "systems/LightSystem.h"
#include "components/PointLight.h"
#include "components/Transform.h"
#include <algorithm>
#include <cstring>

LightSystem& LightSystem::GetInstance() {
    static LightSystem instance;
    return instance;
}

bool LightSystem::Initialize(ID3D11Device* device) {
    if (!device) return false;

    std::memset(&lightData_, 0, sizeof(lightData_));
    lightData_.ambientColor = { 0.1f, 0.1f, 0.1f };
    lightData_.ambientIntensity = 1.0f;
    lightData_.activePointLights = 0;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(LightConstantBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &lightData_;

    HRESULT hr = device->CreateBuffer(&desc, &initData, lightBuffer_.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return false;

    initialized_ = true;
    return true;
}

void LightSystem::Shutdown() {
    lightBuffer_.Reset();
    initialized_ = false;
}

void LightSystem::Update(World& world, const DirectX::XMFLOAT3& cameraPos) {
    if (!initialized_) return;

    lightData_.cameraPosition = cameraPos;
    lightData_.activePointLights = 0;

    world.ForEach<Transform, PointLight>([this](Entity, Transform& t, PointLight& pl) {
        if (!pl.enabled) return;
        if (lightData_.activePointLights >= MAX_POINT_LIGHTS) return;

        int idx = lightData_.activePointLights;
        lightData_.pointLights[idx].position = t.position;
        lightData_.pointLights[idx].range = pl.range;
        lightData_.pointLights[idx].color = pl.color;
        lightData_.pointLights[idx].intensity = pl.intensity;
        lightData_.pointLights[idx].constantAtt = pl.constantAttenuation;
        lightData_.pointLights[idx].linearAtt = pl.linearAttenuation;
        lightData_.pointLights[idx].quadraticAtt = pl.quadraticAttenuation;
        lightData_.pointLights[idx].enabled = 1.0f;

        lightData_.activePointLights++;
    });
}

void LightSystem::Bind(ID3D11DeviceContext* context, UINT slot) {
    if (!initialized_ || !context) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context->Map(lightBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        std::memcpy(mapped.pData, &lightData_, sizeof(lightData_));
        context->Unmap(lightBuffer_.Get(), 0);
    }

    context->PSSetConstantBuffers(slot, 1, lightBuffer_.GetAddressOf());
}

void LightSystem::SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity) {
    lightData_.ambientColor = color;
    lightData_.ambientIntensity = intensity;
}
