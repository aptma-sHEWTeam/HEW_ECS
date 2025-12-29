#include "systems/ShadowRenderSystem.h"
#include "app/ServiceLocator.h"
#include "graphics/RenderSystem.h"
#include "components/MeshRenderer.h"
#include "app/DebugLog.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

bool ShadowRenderSystem::Initialize(GfxDevice& gfx) {
    if (initialized_) return true;
    
    if (!CreateShaders(gfx)) {
        DEBUGLOG_ERROR("ShadowRenderSystem: Failed to compile shaders");
        return false;
    }

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(VSConstants);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(gfx.Dev()->CreateBuffer(&cbDesc, nullptr, constantBuffer_.GetAddressOf()))) return false;

    // Rasterizer (Depth Bias needed to avoid acne)
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK; 
    rsDesc.DepthBias = 1000;
    rsDesc.DepthBiasClamp = 0.0f;
    rsDesc.SlopeScaledDepthBias = 1.0f; // Typical bias
    if (FAILED(gfx.Dev()->CreateRasterizerState(&rsDesc, rasterState_.GetAddressOf()))) return false;

    initialized_ = true;
    return true;
}

void ShadowRenderSystem::Shutdown() {
    vs_.Reset();
    ps_.Reset();
    layout_.Reset();
    constantBuffer_.Reset();
    rasterState_.Reset();
    initialized_ = false;
}

void ShadowRenderSystem::RenderShadows(GfxDevice& gfx, World& world, const XMFLOAT3& lightPos, float lightRange, OmniShadowMap& shadowMap) {
    if (!initialized_) return;
    auto* context = gfx.Ctx();
    auto& renderSystem = ServiceLocator::Get<RenderSystem>();

    // Projection: 90 deg FOV, 1.0 Aspect, Near 0.1, Far = lightRange
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.1f, lightRange);

    struct FaceInfo {
        XMVECTOR target;
        XMVECTOR up;
    };
    
    XMVECTOR pos = XMLoadFloat3(&lightPos);
    
    // Check orientation carefully. HLSL Cubemap Sample uses:
    // +X (Right), -X (Left), +Y (Top), -Y (Bottom), +Z (Front), -Z (Back)
    // LookAtLH: Eye, At, Up.
    // +X: At(1,0,0), Up(0,1,0)
    // -X: At(-1,0,0), Up(0,1,0)
    // +Y: At(0,1,0), Up(0,0,-1)
    // -Y: At(0,-1,0), Up(0,0,1)
    // +Z: At(0,0,1), Up(0,1,0) 
    // -Z: At(0,0,-1), Up(0,1,0)
    FaceInfo faces[6] = {
        { XMVectorSet(1, 0, 0, 0), XMVectorSet(0, 1, 0, 0) }, // +X
        { XMVectorSet(-1,0, 0, 0), XMVectorSet(0, 1, 0, 0) }, // -X
        { XMVectorSet(0, 1, 0, 0), XMVectorSet(0, 0,-1, 0) }, // +Y
        { XMVectorSet(0,-1, 0, 0), XMVectorSet(0, 0, 1, 0) }, // -Y
        { XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 1, 0, 0) }, // +Z
        { XMVectorSet(0, 0,-1, 0), XMVectorSet(0, 1, 0, 0) }  // -Z
    };

    context->IASetInputLayout(layout_.Get());
    context->VSSetShader(vs_.Get(), nullptr, 0);
    context->PSSetShader(ps_.Get(), nullptr, 0);
    context->RSSetState(rasterState_.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Loop 6 faces
    for (int i = 0; i < 6; ++i) {
        shadowMap.BeginFace(context, i);

        // View Matrix
        XMVECTOR target = XMVectorAdd(pos, faces[i].target);
        XMMATRIX view = XMMatrixLookAtLH(pos, target, faces[i].up);
        XMMATRIX viewProj = view * proj;
        
        VSConstants cb;
        cb.viewProj = XMMatrixTranspose(viewProj);
        cb.lightPos = lightPos;
        cb.lightRange = lightRange;

        // Render MeshRenderers
        std::unordered_map<uint32_t, XMMATRIX> worldCache;

        // Recursive helper to calculate/cache world matrix
        std::function<XMMATRIX(Entity, Transform&)> GetWorldMatrix;
        GetWorldMatrix = [&](Entity e, Transform& t) -> XMMATRIX {
            if (worldCache.find(e.id) != worldCache.end()) return worldCache[e.id];

            XMMATRIX S = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
            XMMATRIX R = XMMatrixRotationRollPitchYaw(
                XMConvertToRadians(t.rotation.x),
                XMConvertToRadians(t.rotation.y),
                XMConvertToRadians(t.rotation.z));
            XMMATRIX T = XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
            XMMATRIX local = S * R * T;

            XMMATRIX worldMat = local;
            auto* h = world.TryGet<TransformHierarchy>(e);
            if (h && h->HasParent()) {
                auto pOpt = h->GetParent();
                if (pOpt && world.IsAlive(*pOpt)) {
                    if (auto* pt = world.TryGet<Transform>(*pOpt)) {
                        worldMat = local * GetWorldMatrix(*pOpt, *pt);
                    }
                }
            }
            worldCache[e.id] = worldMat;
            return worldMat;
        };
        
        world.ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
            // mr.enabled check removed as it doesn't exist
            
            XMMATRIX worldMat = GetWorldMatrix(e, t);
            cb.world = XMMatrixTranspose(worldMat);
            
            context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);
            context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

            renderSystem.DrawMesh(mr.meshType);
        });
    }
}

bool ShadowRenderSystem::CreateShaders(GfxDevice& gfx) {
    // Vertex Shader
    const char* vsCode = R"(
        cbuffer CB : register(b0) {
            float4x4 World;
            float4x4 ViewProj;
            float3 LightPos;
            float LightRange;
        };
        struct VS_IN { 
            float3 pos : POSITION; 
            float2 tex : TEXCOORD; // Layout compatibility
            float3 nrm : NORMAL;
            float3 tan : TANGENT;
            float3 bitan : BITANGENT;
            uint4 bones : BLENDINDICES;
            float4 weights : BLENDWEIGHT;
        };
        struct VS_OUT { float4 pos : SV_POSITION; float3 worldPos : TEXCOORD0; };
        
        VS_OUT main(VS_IN input) {
            VS_OUT output;
            float4 worldPos = mul(float4(input.pos, 1.0f), World);
            output.pos = mul(worldPos, ViewProj);
            output.worldPos = worldPos.xyz;
            return output;
        }
    )";
    
    // Pixel Shader: Write Linear Distance (0..1)
    const char* psCode = R"(
        cbuffer CB : register(b0) {
            float4x4 World;
            float4x4 ViewProj;
            float3 LightPos;
            float LightRange;
        };
        struct VS_OUT { float4 pos : SV_POSITION; float3 worldPos : TEXCOORD0; };
        
        float4 main(VS_OUT input) : SV_TARGET {
            float dist = length(input.worldPos - LightPos);
            float normDist = dist / LightRange;
            return float4(normDist, 0, 0, 1);
        }
    )";

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (FAILED(D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsBlob, &errBlob))) {
        DEBUGLOG_ERROR("Shadow VS Compile Error");
        return false;
    }
    if (FAILED(D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", flags, 0, &psBlob, &errBlob))) {
        DEBUGLOG_ERROR("Shadow PS Compile Error");
        if (errBlob) DEBUGLOG_ERROR((char*)errBlob->GetBufferPointer());
        return false;
    }

    gfx.Dev()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs_);
    gfx.Dev()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps_);

    // Use RenderSystem's layout logic: 
    // RenderSystem expects: POSITION, TEXCOORD, NORMAL, TANGENT, BITANGENT, BLENDINDICES, BLENDWEIGHT
    // My shader input struct mimics this.
    D3D11_INPUT_ELEMENT_DESC desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    gfx.Dev()->CreateInputLayout(desc, 7, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout_);

    return true;
}
