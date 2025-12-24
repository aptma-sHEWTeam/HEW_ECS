#include "graphics/SkyboxSystem.h"
#include "app/ServiceLocator.h"
#include "app/DebugLog.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

bool SkyboxSystem::Initialize(GfxDevice& gfx) {
    if (initialized_) return true;

    if (!CreateShaders(gfx)) {
        DEBUGLOG_ERROR("SkyboxSystem: Failed to compile shaders");
        return false;
    }

    if (!CreateCubeMesh(gfx)) {
        DEBUGLOG_ERROR("SkyboxSystem: Failed to create cube mesh");
        return false;
    }

    if (!CreateStates(gfx)) {
        DEBUGLOG_ERROR("SkyboxSystem: Failed to create states");
        return false;
    }

    // Create Constant Buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VSConstants);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    
    if (FAILED(gfx.Dev()->CreateBuffer(&bd, nullptr, constantBuffer_.GetAddressOf()))) {
        DEBUGLOG_ERROR("SkyboxSystem: Failed to create constant buffer");
        return false;
    }

    initialized_ = true;
    DEBUGLOG("SkyboxSystem initialized");
    return true;
}

void SkyboxSystem::Shutdown() {
    vs_.Reset();
    ps_.Reset();
    layout_.Reset();
    constantBuffer_.Reset();
    vertexBuffer_.Reset();
    indexBuffer_.Reset();
    depthStencilState_.Reset();
    rasterizerState_.Reset();
    samplerState_.Reset();
    initialized_ = false;
}

void SkyboxSystem::Render(GfxDevice& gfx, const Camera& camera, float rotationY) {
    if (!initialized_ || textureHandle_ == TextureManager::INVALID_TEXTURE) return;

    auto* context = gfx.Ctx();
    auto& texMgr = ServiceLocator::Get<TextureManager>();
    auto* srv = texMgr.GetSRV(textureHandle_);
    if (!srv) return;

    // Save previous state (optional, but good practice. For now assume full overwrite)
    
    // Calculate Matrix
    // Skybox should be centered at camera, so view matrix translation is removed (or just use position = 0 relative to camera)
    // Actually, Skybox is usually rendered with View(Rotation) * Proj
    
    DirectX::XMMATRIX view = camera.View; // Use copy to modify
    DirectX::XMMATRIX proj = camera.Proj;
    
    // Remove translation from View Matrix
    DirectX::XMFLOAT4X4 viewF;
    DirectX::XMStoreFloat4x4(&viewF, view);
    viewF._41 = 0.0f; viewF._42 = 0.0f; viewF._43 = 0.0f;
    view = DirectX::XMLoadFloat4x4(&viewF);

    DirectX::XMMATRIX world = DirectX::XMMatrixRotationY(rotationY);
    DirectX::XMMATRIX wvp = world * view * proj;
    wvp = DirectX::XMMatrixTranspose(wvp);

    VSConstants cb;
    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&cb.WVP.r[0]), wvp);
    context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);

    // Set States
    context->OMSetDepthStencilState(depthStencilState_.Get(), 0);
    context->RSSetState(rasterizerState_.Get());
    
    // Set Shaders & Buffers
    context->IASetInputLayout(layout_.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(DirectX::XMFLOAT3);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);

    context->VSSetShader(vs_.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

    context->PSSetShader(ps_.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, samplerState_.GetAddressOf());

    // Draw
    context->DrawIndexed(indexCount_, 0, 0);

    // Reset Depth State (Important for subsequent opaque rendering if this was first, but usually this is last)
    // If last, we are fine. If first, logic is different.
    // If we rely on default state being DepthWrite=On, DepthFunc=Less, we should restore it.
    // D3D11 has no "Restore Default" easily. 
    // Usually RenderSystem sets its own state every frame. RenderSystem::SetupPipeline does.
    // So if RenderSystem is called BEFORE Skybox, and Skybox is LAST, it's fine.
    context->OMSetDepthStencilState(nullptr, 0);
}

bool SkyboxSystem::CreateShaders(GfxDevice& gfx) {
    const char* vsCode = R"(
        cbuffer SkyboxConstant : register(b0) {
            float4x4 WVP;
        };
        struct VSIn { float3 pos : POSITION; };
        struct VSOut { float4 pos : SV_POSITION; float3 tex : TEXCOORD; };
        VSOut main(VSIn input) {
            VSOut output;
            float4 pos = mul(float4(input.pos, 1.0f), WVP);
            output.pos = pos.xyww; // Force Z to be on far plane
            output.tex = input.pos;
            return output;
        }
    )";
    const char* psCode = R"(
        Texture2D gTexture : register(t0);
        SamplerState gSampler : register(s0);
        static const float PI = 3.14159265f;
        float4 main(float4 pos : SV_POSITION, float3 tex : TEXCOORD) : SV_Target {
            float3 dir = normalize(tex);
            float2 uv = float2((atan2(dir.x, dir.z) / PI) * 0.5f + 0.5f, (asin(dir.y) / PI) + 0.5f);
            uv.y = 1.0f - uv.y; 
            return gTexture.Sample(gSampler, uv);
        }
    )";

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (FAILED(D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsBlob, &errBlob))) {
        DEBUGLOG_ERROR("Skybox VS Compile Error");
        return false;
    }
    if (FAILED(D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", flags, 0, &psBlob, &errBlob))) {
        DEBUGLOG_ERROR("Skybox PS Compile Error");
        return false;
    }

    gfx.Dev()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs_);
    gfx.Dev()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps_);

    D3D11_INPUT_ELEMENT_DESC desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    gfx.Dev()->CreateInputLayout(desc, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout_);

    return true;
}

bool SkyboxSystem::CreateCubeMesh(GfxDevice& gfx) {
    float size = 10.0f; // Size doesn't matter much as it's centered and always far plane, but give it some size.
    // 8 vertices
    DirectX::XMFLOAT3 vertices[] = {
        {-size, -size, -size},
        {-size,  size, -size},
        { size,  size, -size},
        { size, -size, -size},
        {-size, -size,  size},
        {-size,  size,  size},
        { size,  size,  size},
        { size, -size,  size},
    };

    // CCW Winding for inside view? No, usually indices are for outside.
    // If we view from inside, we need CW winding or CullFront.
    // Let's use CullFront in Rasterizer state, or adjust indices.
    // Indices for standard cube (TRIANGLELIST)
    uint32_t indices[] = {
        0, 1, 2, 0, 2, 3, // Front
        4, 6, 5, 4, 7, 6, // Back
        4, 5, 1, 4, 1, 0, // Left
        3, 2, 6, 3, 6, 7, // Right
        1, 5, 6, 1, 6, 2, // Top
        4, 0, 3, 4, 3, 7  // Bottom
    };
    
    indexCount_ = 36;

    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = sizeof(DirectX::XMFLOAT3) * 8;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {};
    vinit.pSysMem = vertices;
    gfx.Dev()->CreateBuffer(&vbd, &vinit, &vertexBuffer_);

    D3D11_BUFFER_DESC ibd = {};
    ibd.ByteWidth = sizeof(uint32_t) * 36;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {};
    iinit.pSysMem = indices;
    gfx.Dev()->CreateBuffer(&ibd, &iinit, &indexBuffer_);

    return true;
}

bool SkyboxSystem::CreateStates(GfxDevice& gfx) {
    // Depth State: Less Equal, ZWrite Off (since we are at far plane/background)
    // Actually, if we draw LAST, we want LessEqual (pass if equal to far plane).
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    gfx.Dev()->CreateDepthStencilState(&dsd, &depthStencilState_);

    // Rasterizer: Cull Front (since we are inside) or Cull None.
    D3D11_RASTERIZER_DESC rsd = {};
    rsd.FillMode = D3D11_FILL_SOLID;
    rsd.CullMode = D3D11_CULL_NONE; // Safe choice
    gfx.Dev()->CreateRasterizerState(&rsd, &rasterizerState_);

    // Sampler
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    gfx.Dev()->CreateSamplerState(&samp, &samplerState_);

    return true;
}
