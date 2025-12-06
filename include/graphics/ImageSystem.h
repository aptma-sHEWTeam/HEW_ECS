/**
 * @file ImageSystem.h
 * @brief Direct2D + WIC 画像描画システム
 */
#pragma once
#include "graphics/GfxDevice.h"
#include "app/DebugLog.h"
#include "app/ServiceLocator.h"
#include "graphics/TextureManager.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <wincodec.h> // WIC enums and GUIDs
#include <wrl/client.h>
#include <unordered_map>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")

class ImageSystem {
public:
    struct Params {
        std::wstring filePath;  ///< 画像ファイルパス
        float x = 0.0f;         ///< X座標
        float y = 0.0f;         ///< Y座標
        float width = 100.0f;   ///< 描画幅
        float height = 100.0f;  ///< 描画高さ
        float opacity = 1.0f;   ///< 透明(0..1)
        bool keepAspect = true; ///< アスペクト維持
        // 追加: ソース矩形(ピクセル)指定(スプライトシート用)。-1/-1/-1/-1 で未指定
        float srcX = -1.0f;
        float srcY = -1.0f;
        float srcW = -1.0f;
        float srcH = -1.0f;
    };

    bool Init(GfxDevice &gfx);
    void BeginDraw();
    void EndDraw();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // 既存: ファイルパスで描画(ソース矩形対応)
    bool Draw(const Params &p);

    // 追加: TextureManagerのハンドルで描画(ソース矩形対応)
    bool Draw(TextureManager::TextureHandle handle, float x, float y, float width, float height, float opacity = 1.0f, bool keepAspect = true,
              const D2D1_RECT_F *srcOverride = nullptr) {
        if (!initialized_ || !d2dContext_) return false;
        if (handle == TextureManager::INVALID_TEXTURE) return false;

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> bmp;
        auto it = handleBitmapCache_.find(handle);
        if (it != handleBitmapCache_.end()) {
            bmp = it->second;
        } else {
            if (!CreateBitmapFromHandle(handle, bmp)) {
                return false;
            }
            handleBitmapCache_[handle] = bmp;
        }

        D2D1_RECT_F dst = D2D1::RectF(x, y, x + width, y + height);
        D2D1_SIZE_F srcSize = bmp->GetSize();
        D2D1_RECT_F src = srcOverride ? *srcOverride : D2D1::RectF(0.0f, 0.0f, srcSize.width, srcSize.height);
        if (keepAspect) {
            float sx = width / (src.right - src.left);
            float sy = height / (src.bottom - src.top);
            float s = sx < sy ? sx : sy;
            float dw = (src.right - src.left) * s;
            float dh = (src.bottom - src.top) * s;
            float ox = x + (width - dw) * 0.5f;
            float oy = y + (height - dh) * 0.5f;
            dst = D2D1::RectF(ox, oy, ox + dw, oy + dh);
        }
        d2dContext_->DrawBitmap(bmp.Get(), &dst, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src);
        return true;
    }

private:
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;

    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap1>> bitmapCache_;
    std::unordered_map<TextureManager::TextureHandle, Microsoft::WRL::ComPtr<ID2D1Bitmap1>> handleBitmapCache_;

    bool initialized_ = false;
    GfxDevice *gfx_ = nullptr;

    void RefreshTargetBitmap();
    bool LoadBitmap(const std::wstring &filePath, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &out);

    bool CreateBitmapFromHandle(TextureManager::TextureHandle handle, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &out) {
        auto &texMgr = ServiceLocator::Get<TextureManager>();
        ID3D11ShaderResourceView *srv = texMgr.GetSRV(handle);
        if (!srv) {
            DEBUGLOG_ERROR("ImageSystem::CreateBitmapFromHandle - SRV is null");
            return false;
        }
        Microsoft::WRL::ComPtr<ID3D11Resource> res;
        srv->GetResource(res.GetAddressOf());
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
        if (FAILED(res.As(&tex2d))) {
            DEBUGLOG_ERROR("ImageSystem::CreateBitmapFromHandle - Resource is not Texture2D");
            return false;
        }
        D3D11_TEXTURE2D_DESC desc{};
        tex2d->GetDesc(&desc);
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.BindFlags = 0;
        stagingDesc.MiscFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
        if (FAILED(gfx_->Dev()->CreateTexture2D(&stagingDesc, nullptr, &staging))) {
            DEBUGLOG_ERROR("ImageSystem::CreateBitmapFromHandle - Failed to create staging texture");
            return false;
        }
        gfx_->Ctx()->CopyResource(staging.Get(), tex2d.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(gfx_->Ctx()->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            DEBUGLOG_ERROR("ImageSystem::CreateBitmapFromHandle - Map failed");
            return false;
        }
        if (!wicFactory_) {
            DEBUGLOG_ERROR("ImageSystem - WIC factory not initialized");
            gfx_->Ctx()->Unmap(staging.Get(), 0);
            return false;
        }
        Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
        HRESULT hr = wicFactory_->CreateBitmap(desc.Width, desc.Height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
        if (FAILED(hr)) {
            gfx_->Ctx()->Unmap(staging.Get(), 0);
            return false;
        }
        Microsoft::WRL::ComPtr<IWICBitmapLock> lock;
        WICRect rc{0, 0, static_cast<INT>(desc.Width), static_cast<INT>(desc.Height)};
        hr = wicBitmap->Lock(&rc, WICBitmapLockWrite, &lock);
        if (FAILED(hr)) {
            gfx_->Ctx()->Unmap(staging.Get(), 0);
            return false;
        }
        UINT cbBuffer = 0; BYTE *dstData = nullptr; UINT dstStride = 0;
        lock->GetDataPointer(&cbBuffer, &dstData);
        lock->GetStride(&dstStride);
        const BYTE *srcData = reinterpret_cast<const BYTE *>(mapped.pData);
        UINT copyStride = std::min<UINT>(dstStride, mapped.RowPitch);
        for (UINT y = 0; y < desc.Height; ++y) {
            memcpy(dstData + y * dstStride, srcData + y * mapped.RowPitch, copyStride);
        }
        lock->Release();
        gfx_->Ctx()->Unmap(staging.Get(), 0);

        D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dBmp;
        hr = d2dContext_->CreateBitmapFromWicBitmap(wicBitmap.Get(), &bp, &d2dBmp);
        if (FAILED(hr)) {
            return false;
        }
        out = d2dBmp;
        return true;
    }
};
