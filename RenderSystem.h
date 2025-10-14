#pragma once
#include "GfxDevice.h"
#include "Camera.h"
#include "World.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "TextureManager.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

// ========================================================
// RenderSystem - �e�N�X�`���Ή������_�����O�V�X�e��
// ========================================================
struct RenderSystem {
    // �p�C�v���C���I�u�W�F�N�g
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cb_; // �萔�o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11Buffer> vb_; // ���_�o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11Buffer> ib_; // �C���f�b�N�X�o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;
    UINT indexCount_ = 0;

    TextureManager* texManager_ = nullptr;

    // ���_�V�F�[�_�[�萔�o�b�t�@
    struct VSConstants {
        DirectX::XMMATRIX WVP;
        DirectX::XMFLOAT4 uvTransform; // xy=offset, zw=scale
    };

    // �s�N�Z���V�F�[�_�[�萔�o�b�t�@
    struct PSConstants {
        DirectX::XMFLOAT4 color;
        float useTexture; // 0=�J���[, 1=�e�N�X�`��
        float padding[3];
    };

    Microsoft::WRL::ComPtr<ID3D11Buffer> psCb_; // PS�̒萔�o�b�t�@

    ~RenderSystem() {
        vs_.Reset();
        ps_.Reset();
        layout_.Reset();
        cb_.Reset();
        psCb_.Reset();
        vb_.Reset();
        ib_.Reset();
        rasterState_.Reset();
        samplerState_.Reset();
    }

    bool Init(GfxDevice& gfx, TextureManager& texMgr) {
        texManager_ = &texMgr;

        // �e�N�X�`���Ή��V�F�[�_�[
        const char* VS = R"(
            cbuffer CB : register(b0) { 
                float4x4 gWVP; 
                float4 gUVTransform; // xy=offset, zw=scale
            };
            struct VSIn { 
                float3 pos : POSITION; 
                float2 tex : TEXCOORD; 
            };
            struct VSOut { 
                float4 pos : SV_POSITION; 
                float2 tex : TEXCOORD; 
            };
            VSOut main(VSIn i){
                VSOut o;
                o.pos = mul(float4(i.pos,1), gWVP);
                o.tex = i.tex * gUVTransform.zw + gUVTransform.xy;
                return o;
            }
        )";
        
        const char* PS = R"(
            cbuffer CB : register(b0) {
                float4 gColor;
                float gUseTexture;
                float3 padding;
            };
            Texture2D gTexture : register(t0);
            SamplerState gSampler : register(s0);
            struct VSOut { 
                float4 pos : SV_POSITION; 
                float2 tex : TEXCOORD; 
            };
            float4 main(VSOut i) : SV_Target { 
                if (gUseTexture > 0.5) {
                    return gTexture.Sample(gSampler, i.tex) * gColor;
                }
                return gColor;
            }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> vsb, psb, err;
        HRESULT hr = D3DCompile(VS, strlen(VS), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                char msg[512];
                sprintf_s(msg, "Vertex Shader compile error:\n%s", (char*)err->GetBufferPointer());
                MessageBoxA(nullptr, msg, "Shader Error", MB_OK | MB_ICONERROR);
            }
            return false;
        }
        
        hr = D3DCompile(PS, strlen(PS), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                char msg[512];
                sprintf_s(msg, "Pixel Shader compile error:\n%s", (char*)err->GetBufferPointer());
                MessageBoxA(nullptr, msg, "Shader Error", MB_OK | MB_ICONERROR);
            }
            return false;
        }

        if (FAILED(gfx.Dev()->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, vs_.GetAddressOf()))) {
            return false;
        }
        
        if (FAILED(gfx.Dev()->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, ps_.GetAddressOf()))) {
            return false;
        }

        // ���̓��C�A�E�g�iPosition + TexCoord�j
        D3D11_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        if (FAILED(gfx.Dev()->CreateInputLayout(il, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), layout_.GetAddressOf()))) {
            return false;
        }

        // VS�萔�o�b�t�@
        D3D11_BUFFER_DESC cbd{};
        cbd.ByteWidth = sizeof(VSConstants);
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(gfx.Dev()->CreateBuffer(&cbd, nullptr, cb_.GetAddressOf()))) {
            return false;
        }

        // PS�萔�o�b�t�@
        cbd.ByteWidth = sizeof(PSConstants);
        if (FAILED(gfx.Dev()->CreateBuffer(&cbd, nullptr, psCb_.GetAddressOf()))) {
            return false;
        }

        // �T���v���[�X�e�[�g
        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.MaxAnisotropy = 1;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(gfx.Dev()->CreateSamplerState(&sampDesc, &samplerState_))) {
            return false;
        }

        // ���X�^���C�U�X�e�[�g
        D3D11_RASTERIZER_DESC rsd{};
        rsd.FillMode = D3D11_FILL_SOLID;
        rsd.CullMode = D3D11_CULL_BACK;
        rsd.FrontCounterClockwise = FALSE;
        rsd.DepthClipEnable = TRUE;
        if (FAILED(gfx.Dev()->CreateRasterizerState(&rsd, rasterState_.GetAddressOf()))) {
            return false;
        }

        // �W�I���g���iUV���W�t���L���[�u�j
        struct V { DirectX::XMFLOAT3 pos; DirectX::XMFLOAT2 tex; };
        const float c = 0.5f;
        V verts[] = {
            // �w��
            {{-c,-c,-c}, {0,1}}, {{-c,+c,-c}, {0,0}}, {{+c,+c,-c}, {1,0}}, {{+c,-c,-c}, {1,1}},
            // �O��
            {{-c,-c,+c}, {1,1}}, {{-c,+c,+c}, {1,0}}, {{+c,+c,+c}, {0,0}}, {{+c,-c,+c}, {0,1}},
        };
        uint16_t idx[] = {
            0,1,2, 0,2,3,  // �w��
            4,6,5, 4,7,6,  // �O��
            4,5,1, 4,1,0,  // ��
            3,2,6, 3,6,7,  // �E
            1,5,6, 1,6,2,  // ��
            4,0,3, 4,3,7   // ��
        };
        indexCount_ = (UINT)(sizeof(idx) / sizeof(idx[0]));

        D3D11_BUFFER_DESC vbd{};
        vbd.ByteWidth = (UINT)sizeof(verts);
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA vinit{ verts, 0, 0 };
        if (FAILED(gfx.Dev()->CreateBuffer(&vbd, &vinit, vb_.GetAddressOf()))) {
            return false;
        }

        D3D11_BUFFER_DESC ibd{};
        ibd.ByteWidth = (UINT)sizeof(idx);
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA iinit{ idx, 0, 0 };
        if (FAILED(gfx.Dev()->CreateBuffer(&ibd, &iinit, ib_.GetAddressOf()))) {
            return false;
        }

        return true;
    }

    void Render(GfxDevice& gfx, World& w, const Camera& cam) {
        gfx.Ctx()->IASetInputLayout(layout_.Get());
        gfx.Ctx()->VSSetShader(vs_.Get(), nullptr, 0);
        gfx.Ctx()->PSSetShader(ps_.Get(), nullptr, 0);
        gfx.Ctx()->VSSetConstantBuffers(0, 1, cb_.GetAddressOf());
        gfx.Ctx()->PSSetConstantBuffers(0, 1, psCb_.GetAddressOf());
        gfx.Ctx()->PSSetSamplers(0, 1, samplerState_.GetAddressOf());
        gfx.Ctx()->RSSetState(rasterState_.Get());

        UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
        UINT offset = 0;
        gfx.Ctx()->IASetVertexBuffers(0, 1, vb_.GetAddressOf(), &stride, &offset);
        gfx.Ctx()->IASetIndexBuffer(ib_.Get(), DXGI_FORMAT_R16_UINT, 0);
        gfx.Ctx()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        w.ForEach<MeshRenderer>([&](Entity e, MeshRenderer& mr) {
            auto* t = w.TryGet<Transform>(e);
            if (!t) return;

            // ���[���h�s��
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t->scale.x, t->scale.y, t->scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t->rotation.x),
                DirectX::XMConvertToRadians(t->rotation.y),
                DirectX::XMConvertToRadians(t->rotation.z));
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t->position.x, t->position.y, t->position.z);
            DirectX::XMMATRIX W = S * R * T;

            // VS�萔�o�b�t�@
            VSConstants vsCbuf;
            vsCbuf.WVP = DirectX::XMMatrixTranspose(W * cam.View * cam.Proj);
            vsCbuf.uvTransform = DirectX::XMFLOAT4{ mr.uvOffset.x, mr.uvOffset.y, mr.uvScale.x, mr.uvScale.y };
            gfx.Ctx()->UpdateSubresource(cb_.Get(), 0, nullptr, &vsCbuf, 0, 0);

            // PS�萔�o�b�t�@
            PSConstants psCbuf;
            psCbuf.color = DirectX::XMFLOAT4{ mr.color.x, mr.color.y, mr.color.z, 1.0f };
            psCbuf.useTexture = (mr.texture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
            gfx.Ctx()->UpdateSubresource(psCb_.Get(), 0, nullptr, &psCbuf, 0, 0);

            // �e�N�X�`���ݒ�
            if (mr.texture != TextureManager::INVALID_TEXTURE && texManager_) {
                ID3D11ShaderResourceView* srv = texManager_->GetSRV(mr.texture);
                if (srv) {
                    gfx.Ctx()->PSSetShaderResources(0, 1, &srv);
                }
            }

            gfx.Ctx()->DrawIndexed(indexCount_, 0, 0);
        });
    }
};
