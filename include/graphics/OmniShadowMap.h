#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "app/DebugLog.h"

class OmniShadowMap {
public:
    bool Init(ID3D11Device* device, int size = 1024) {
        size_ = size;
        
        // 1. Texture Cube (R32_FLOAT | BIND_RENDER_TARGET | BIND_SHADER_RESOURCE)
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = size;
        texDesc.Height = size;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 6;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        if (FAILED(device->CreateTexture2D(&texDesc, nullptr, texture_.GetAddressOf()))) {
            DEBUGLOG_ERROR("OmniShadowMap: Failed to create texture");
            return false;
        }

        // 2. Render Target Views (One for each face)
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = 0;
        rtvDesc.Texture2DArray.ArraySize = 1;

        for (int i = 0; i < 6; ++i) {
            rtvDesc.Texture2DArray.FirstArraySlice = i;
            if (FAILED(device->CreateRenderTargetView(texture_.Get(), &rtvDesc, rtvs_[i].GetAddressOf()))) {
                DEBUGLOG_ERROR("OmniShadowMap: Failed to create RTV for face " + std::to_string(i));
                return false;
            }
        }

        // 3. Depth Buffer (Shared for all faces, cleared each time)
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = size;
        depthDesc.Height = size;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;
        if (FAILED(device->CreateTexture2D(&depthDesc, nullptr, depthTex.GetAddressOf()))) return false;
        if (FAILED(device->CreateDepthStencilView(depthTex.Get(), nullptr, dsv_.GetAddressOf()))) return false;

        // 4. Shader Resource View (Cube)
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.TextureCube.MostDetailedMip = 0;

        if (FAILED(device->CreateShaderResourceView(texture_.Get(), &srvDesc, srv_.GetAddressOf()))) {
             DEBUGLOG_ERROR("OmniShadowMap: Failed to create SRV");
             return false;
        }

        return true;
    }

    void BeginFace(ID3D11DeviceContext* context, int faceIndex) {
        if (faceIndex < 0 || faceIndex >= 6) return;
        ID3D11RenderTargetView* rtv = rtvs_[faceIndex].Get();
        ID3D11DepthStencilView* dsv = dsv_.Get();
        
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Max distance
        context->ClearRenderTargetView(rtv, clearColor);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        context->OMSetRenderTargets(1, &rtv, dsv);
        
        D3D11_VIEWPORT vp = {};
        vp.Width = (float)size_;
        vp.Height = (float)size_;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        context->RSSetViewports(1, &vp);
    }

    ID3D11ShaderResourceView* GetSRV() { return srv_.Get(); }

    void Shutdown() {
        texture_.Reset();
        srv_.Reset();
        dsv_.Reset();
        for(auto& rtv : rtvs_) rtv.Reset();
    }

private:
    int size_ = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtvs_[6];
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv_;
};
