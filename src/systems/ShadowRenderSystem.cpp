#include "systems/ShadowRenderSystem.h"
#include "app/ServiceLocator.h"
#include "graphics/RenderSystem.h"
#include "components/MeshRenderer.h"
#include "components/ModelComponent.h"
#include "components/TransformHierarchy.h"
#include "components/StageComponents.h"
#include "components/GameTags.h"
#include "app/DebugLog.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

namespace {
struct ShadowVertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 tex;
    DirectX::XMFLOAT3 nrm;
    DirectX::XMFLOAT3 tan;
    DirectX::XMFLOAT3 bitan;
    uint32_t boneIndices[4];
    float boneWeights[4];
};

struct ShadowSkinningConstants {
    DirectX::XMFLOAT4X4 boneTransforms[128];
};

void FillIdentitySkinning(ShadowSkinningConstants &skinning) {
    for (int i = 0; i < 128; ++i) {
        XMStoreFloat4x4(&skinning.boneTransforms[i], XMMatrixIdentity());
    }
}

bool IsShadowCaster(World &world, Entity entity) {
    Entity current = entity;
    for (int depth = 0; depth < 64; ++depth) {
        if (world.Has<StageElementTag>(current) || world.Has<PlayerTag>(current)) {
            return true;
        }
        auto *hier = world.TryGet<TransformHierarchy>(current);
        if (!hier || !hier->HasParent()) {
            return false;
        }
        auto parentOpt = hier->GetParent();
        if (!parentOpt || !world.IsAlive(*parentOpt)) {
            return false;
        }
        current = *parentOpt;
    }
    return false;
}
} // namespace

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

    D3D11_BUFFER_DESC skinDesc = {};
    skinDesc.ByteWidth = sizeof(ShadowSkinningConstants);
    skinDesc.Usage = D3D11_USAGE_DEFAULT;
    skinDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(gfx.Dev()->CreateBuffer(&skinDesc, nullptr, skinningBuffer_.GetAddressOf()))) return false;

    // Rasterizer (Depth Bias needed to avoid acne)
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK; 
    rsDesc.DepthBias = 1000;
    rsDesc.DepthBiasClamp = 0.0f;
    rsDesc.SlopeScaledDepthBias = 1.0f; // Typical bias
    if (FAILED(gfx.Dev()->CreateRasterizerState(&rsDesc, rasterState_.GetAddressOf()))) return false;

    #if defined(_DEBUG)
    static bool shadowTestRan = false;
    if (!shadowTestRan) {
        ShadowSkinningConstants skinning{};
        FillIdentitySkinning(skinning);
        assert(std::abs(skinning.boneTransforms[0]._11 - 1.0f) < 1e-6f);
        assert(std::abs(skinning.boneTransforms[0]._22 - 1.0f) < 1e-6f);
        assert(std::abs(skinning.boneTransforms[0]._33 - 1.0f) < 1e-6f);
        shadowTestRan = true;
    }
    #endif

    initialized_ = true;
    return true;
}

void ShadowRenderSystem::Shutdown() {
    vs_.Reset();
    ps_.Reset();
    layout_.Reset();
    constantBuffer_.Reset();
    skinningBuffer_.Reset();
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

    ShadowSkinningConstants identitySkinning{};
    FillIdentitySkinning(identitySkinning);
    
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

        auto BindSkinning = [&](const ModelComponent *mc) {
            if (mc && mc->isSkinned && !mc->skeleton.boneTransforms.empty()) {
                ShadowSkinningConstants skCbuf;
                size_t boneCount = std::min(mc->skeleton.boneTransforms.size(), size_t(128));
                for (size_t i = 0; i < boneCount; ++i) {
                    skCbuf.boneTransforms[i] = mc->skeleton.boneTransforms[i];
                }
                for (size_t i = boneCount; i < 128; ++i) {
                    XMStoreFloat4x4(&skCbuf.boneTransforms[i], XMMatrixIdentity());
                }
                context->UpdateSubresource(skinningBuffer_.Get(), 0, nullptr, &skCbuf, 0, 0);
            } else {
                context->UpdateSubresource(skinningBuffer_.Get(), 0, nullptr, &identitySkinning, 0, 0);
            }
            context->VSSetConstantBuffers(1, 1, skinningBuffer_.GetAddressOf());
        };
        
        world.ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
            // mr.enabled check removed as it doesn't exist
            if (!IsShadowCaster(world, e)) {
                return;
            }
            
            XMMATRIX worldMat = GetWorldMatrix(e, t);
            cb.world = XMMatrixTranspose(worldMat);
            
            context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);
            context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

            BindSkinning(nullptr);
            renderSystem.DrawMesh(mr.meshType);
        });

        world.ForEach<ModelComponent>([&](Entity e, ModelComponent &mc) {
            auto *t = world.TryGet<Transform>(e);
            if (!t || !mc.vertexBuffer || !mc.indexBuffer || mc.indexCount == 0) {
                return;
            }
            if (!IsShadowCaster(world, e)) {
                return;
            }

            XMMATRIX worldMat = GetWorldMatrix(e, *t);
            cb.world = XMMatrixTranspose(worldMat);

            context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);
            context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

            BindSkinning(&mc);

            UINT stride = sizeof(ShadowVertex);
            UINT offset = 0;
            context->IASetVertexBuffers(0, 1, mc.vertexBuffer.GetAddressOf(), &stride, &offset);
            context->IASetIndexBuffer(mc.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            context->DrawIndexed(mc.indexCount, 0, 0);
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
          cbuffer Skinning : register(b1) {
              row_major float4x4 gBoneTransforms[128];
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
              float3 posL = input.pos;
              float weights[4] = {input.weights.x, input.weights.y, input.weights.z, input.weights.w};
              uint indices[4] = {input.bones.x, input.bones.y, input.bones.z, input.bones.w};
              float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
              if (weightSum > 0.001f) {
                  float3 p = 0.0f;
                  [unroll]
                  for (int j = 0; j < 4; ++j) {
                      float w = weights[j];
                      if (w > 0.0f) {
                          p += mul(float4(posL, 1.0f), gBoneTransforms[indices[j]]).xyz * w;
                      }
                  }
                  posL = p;
              }
              float4 worldPos = mul(float4(posL, 1.0f), World);
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
