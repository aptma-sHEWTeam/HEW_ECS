#include "systems/EmissiveRenderSystem.h"
#include "components/EmissiveMaterial.h"
#include <cstring>

EmissiveRenderSystem& EmissiveRenderSystem::GetInstance() {
    static EmissiveRenderSystem instance;
    return instance;
}

bool EmissiveRenderSystem::Initialize(ID3D11Device* device) {
    if (!device) return false;

    std::memset(&materialData_, 0, sizeof(materialData_));
    materialData_.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_.specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_.emissiveColor = { 0.0f, 0.0f, 0.0f };
    materialData_.emissiveIntensity = 0.0f;
    materialData_.shininess = 32.0f;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(MaterialConstantBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &materialData_;

    HRESULT hr = device->CreateBuffer(&desc, &initData, materialBuffer_.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return false;

    initialized_ = true;
    return true;
}

void EmissiveRenderSystem::Shutdown() {
    materialBuffer_.Reset();
    initialized_ = false;
}

void EmissiveRenderSystem::UpdateMaterial(ID3D11DeviceContext* context, Entity entity, World& world) {
    if (!initialized_ || !context) return;

    auto* emissive = world.TryGet<EmissiveMaterial>(entity);
    if (emissive && emissive->enabled) {
        materialData_.emissiveColor = emissive->emissiveColor;
        materialData_.emissiveIntensity = emissive->emissiveIntensity;
    } else {
        materialData_.emissiveColor = { 0.0f, 0.0f, 0.0f };
        materialData_.emissiveIntensity = 0.0f;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context->Map(materialBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        std::memcpy(mapped.pData, &materialData_, sizeof(materialData_));
        context->Unmap(materialBuffer_.Get(), 0);
    }
}

void EmissiveRenderSystem::Bind(ID3D11DeviceContext* context, UINT slot) {
    if (!initialized_ || !context) return;
    context->PSSetConstantBuffers(slot, 1, materialBuffer_.GetAddressOf());
}
