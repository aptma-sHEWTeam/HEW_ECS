#pragma once
#include "graphics/GfxDevice.h"
#include "graphics/OmniShadowMap.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/PointLight.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>

class ShadowRenderSystem {
public:
    bool Initialize(GfxDevice& gfx);
    void Shutdown();
    void RenderShadows(GfxDevice& gfx, World& world, const DirectX::XMFLOAT3& lightPos, float lightRange, OmniShadowMap& shadowMap);

private:
    struct VSConstants {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX viewProj;
        DirectX::XMFLOAT3 lightPos;
        float lightRange;
    };

    bool CreateShaders(GfxDevice& gfx);
    void RenderScene(ID3D11DeviceContext* context, World& world);

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_; 
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState_;
    
    bool initialized_ = false;
};
