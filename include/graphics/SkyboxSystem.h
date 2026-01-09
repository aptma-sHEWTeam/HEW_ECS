#pragma once

#include "graphics/GfxDevice.h"
#include "graphics/Camera.h"
#include "graphics/TextureManager.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

class SkyboxSystem {
public:
    bool Initialize(GfxDevice& gfx);
    void Shutdown();
    void Render(GfxDevice& gfx, const Camera& camera, float rotationY = 0.0f);
    
    // Set skybox texture (must be a Cubemap texture handle)
    void SetTexture(TextureManager::TextureHandle handle) { textureHandle_ = handle; }

private:
    struct VSConstants {
        DirectX::XMMATRIX WVP;
        DirectX::XMMATRIX WorldView;
    };

    bool CreateShaders(GfxDevice& gfx);
    bool CreateCubeMesh(GfxDevice& gfx);
    bool CreateStates(GfxDevice& gfx);

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
    UINT indexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;

    TextureManager::TextureHandle textureHandle_ = TextureManager::INVALID_TEXTURE;
    bool initialized_ = false;
};
