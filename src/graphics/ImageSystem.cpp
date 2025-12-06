/**
 * @file ImageSystem.cpp
 * @brief ImageSystem 実装
 */
#include "graphics/ImageSystem.h"
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <unordered_set>
#include <sstream>
#include <comdef.h>
#include <algorithm>

bool ImageSystem::Init(GfxDevice &gfx) {
    if (initialized_) return true;
    gfx_ = &gfx;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = gfx.Dev()->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: IDXGIDevice QI failed"); return false; }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: D2D1 factory failed"); return false; }

    hr = d2dFactory_->CreateDevice(dxgiDevice.Get(), d2dDevice_.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: D2D device failed"); return false; }

    hr = d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContext_.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: D2D context failed"); return false; }

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicFactory_.GetAddressOf()));
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: WIC factory failed"); return false; }

    Microsoft::WRL::ComPtr<IDXGISurface> backBuffer;
    hr = gfx.GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: GetBuffer failed"); return false; }

    FLOAT dpiX = 96.0f, dpiY = 96.0f;
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);
    hr = d2dContext_->CreateBitmapFromDxgiSurface(backBuffer.Get(), &bp, targetBitmap_.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: CreateBitmapFromDxgiSurface failed"); return false; }

    d2dContext_->SetTarget(targetBitmap_.Get());

    initialized_ = true;
    return true;
}

void ImageSystem::BeginDraw() {
    if (d2dContext_) {
        if (!targetBitmap_) RefreshTargetBitmap();
        d2dContext_->BeginDraw();
    }
}

void ImageSystem::EndDraw() {
    if (!d2dContext_) return;
    HRESULT hr = d2dContext_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        RefreshTargetBitmap();
    } else if (FAILED(hr)) {
        DEBUGLOG_ERROR("ImageSystem: EndDraw failed");
    }
}

void ImageSystem::Shutdown() {
    if (!initialized_) return;
    bitmapCache_.clear();
    targetBitmap_.Reset();
    d2dContext_.Reset();
    d2dDevice_.Reset();
    wicFactory_.Reset();
    d2dFactory_.Reset();
    initialized_ = false;
    gfx_ = nullptr;
}

void ImageSystem::RefreshTargetBitmap() {
    if (!gfx_ || !d2dContext_) return;
    d2dContext_->SetTarget(nullptr);
    targetBitmap_.Reset();
    Microsoft::WRL::ComPtr<IDXGISurface> backBuffer;
    HRESULT hr = gfx_->GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: Refresh GetBuffer failed"); return; }
    FLOAT dpiX = 96.0f, dpiY = 96.0f;
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);
    hr = d2dContext_->CreateBitmapFromDxgiSurface(backBuffer.Get(), &bp, targetBitmap_.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: Refresh CreateBitmapFromDxgiSurface failed"); return; }
    d2dContext_->SetTarget(targetBitmap_.Get());
}

bool ImageSystem::LoadBitmap(const std::wstring &filePath, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &out) {
    if (!wicFactory_ || !d2dContext_) return false;
    if (filePath.empty()) {
        DEBUGLOG_WARNING("ImageSystem: empty filePath requested");
        return false;
    }
    auto it = bitmapCache_.find(filePath);
    if (it != bitmapCache_.end()) { out = it->second; return true; }

    auto logHr = [&](const char* msg, HRESULT hrVal) {
        std::wstringstream wss;
        wss << L"[ImageSystem] " << msg << L" hr=0x" << std::hex << hrVal << L" path=" << filePath;
        std::wstring w = wss.str();
        std::string n(w.begin(), w.end());
        DEBUGLOG_ERROR(n);
    };

    try {
        constexpr UINT MAX_DIMENSION = 16384; // D2D既定の最大テクスチャサイズ
        static std::unordered_set<std::wstring> scaledWarned;
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        HRESULT hr = wicFactory_->CreateDecoderFromFilename(filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
        if (FAILED(hr)) { logHr("CreateDecoderFromFilename failed", hr); return false; }

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(hr)) { logHr("GetFrame failed", hr); return false; }

        // サイズ取得＆必要ならダウンサンプル
        UINT srcW = 0, srcH = 0;
        hr = frame->GetSize(&srcW, &srcH);
        if (FAILED(hr)) { logHr("GetSize failed", hr); return false; }

        Microsoft::WRL::ComPtr<IWICBitmapSource> source = frame;
        if (srcW > MAX_DIMENSION || srcH > MAX_DIMENSION) {
            double scale = static_cast<double>(MAX_DIMENSION) / static_cast<double>(std::max(srcW, srcH));
            UINT dstW = std::max<UINT>(1, static_cast<UINT>(srcW * scale));
            UINT dstH = std::max<UINT>(1, static_cast<UINT>(srcH * scale));
            Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
            hr = wicFactory_->CreateBitmapScaler(scaler.GetAddressOf());
            if (FAILED(hr)) { logHr("CreateBitmapScaler failed", hr); return false; }
            hr = scaler->Initialize(frame.Get(), dstW, dstH, WICBitmapInterpolationModeFant);
            if (FAILED(hr)) { logHr("Scaler Initialize failed", hr); return false; }
            source = scaler;
            if (scaledWarned.insert(filePath).second) {
                std::wstringstream wss;
                wss << L"[ImageSystem] Downscale " << srcW << L"x" << srcH << L" -> " << dstW << L"x" << dstH << L" for path=" << filePath;
                std::wstring w = wss.str();
                std::string n(w.begin(), w.end());
                DEBUGLOG_WARNING(n);
            }
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        hr = wicFactory_->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(hr)) { logHr("CreateFormatConverter failed", hr); return false; }

        hr = converter->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) { logHr("FormatConverter Initialize failed", hr); return false; }

        D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> bmp;
        hr = d2dContext_->CreateBitmapFromWicBitmap(converter.Get(), &props, bmp.GetAddressOf());
        if (FAILED(hr)) {
            logHr("CreateBitmapFromWicBitmap failed", hr);
            // フォールバック: デフォルト白テクスチャからD2Dビットマップを生成
            auto &texMgr = ServiceLocator::Get<TextureManager>();
            auto handle = texMgr.GetDefaultWhite();
            if (handle != TextureManager::INVALID_TEXTURE && CreateBitmapFromHandle(handle, bmp)) {
                std::wstring wmsg = L"[ImageSystem] Fallback to default white texture for path=" + filePath;
                std::string n(wmsg.begin(), wmsg.end());
                DEBUGLOG_WARNING(n);
                bitmapCache_[filePath] = bmp;
                out = bmp;
                return true;
            }
            return false;
        }

        bitmapCache_[filePath] = bmp;
        out = bmp;
        return true;
    } catch (const _com_error& ex) {
        std::wstringstream wss;
        const wchar_t* wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
        wss << L"[ImageSystem] LoadBitmap _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg << L" path=" << filePath;
        std::wstring w = wss.str();
        std::string n(w.begin(), w.end());
        DEBUGLOG_ERROR(n);
        return false;
    }
}

bool ImageSystem::Draw(const Params &p) {
    if (!d2dContext_) return false;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bmp;
    if (!LoadBitmap(p.filePath, bmp)) {
        static std::unordered_set<std::wstring> warned;
        if (warned.insert(p.filePath).second) {
            std::wstring wmsg = L"[ImageSystem] Draw skipped (LoadBitmap failed) path=" + p.filePath;
            std::string n(wmsg.begin(), wmsg.end());
            DEBUGLOG_WARNING(n);
        }
        return false;
    }

    D2D1_SIZE_F size = bmp->GetSize();
    float dstW = p.width;
    float dstH = p.height;
    float x = p.x;
    float y = p.y;

    // ソース矩形(ピクセル)の決定。srcX/Y/W/H が 0..1 の場合は正規化とみなして変換
    D2D1_RECT_F src = D2D1::RectF(0.0f, 0.0f, size.width, size.height);
    if (p.srcW > 0.0f && p.srcH > 0.0f) {
        float sx = p.srcX;
        float sy = p.srcY;
        float sw = p.srcW;
        float sh = p.srcH;
        // 0..1 の正規化値とみなしてピクセルに変換
        if (sx >= 0.0f && sx <= 1.0f && sy >= 0.0f && sy <= 1.0f && sw > 0.0f && sw <= 1.0f && sh > 0.0f && sh <= 1.0f) {
            sx *= size.width;
            sy *= size.height;
            sw *= size.width;
            sh *= size.height;
        }
        src = D2D1::RectF(sx, sy, sx + sw, sy + sh);
    }

    if (p.keepAspect && (src.right - src.left) > 0 && (src.bottom - src.top) > 0) {
        float sx = dstW / (src.right - src.left);
        float sy = dstH / (src.bottom - src.top);
        float s = sx < sy ? sx : sy;
        dstW = (src.right - src.left) * s;
        dstH = (src.bottom - src.top) * s;
        x += (p.width - dstW) * 0.5f;
        y += (p.height - dstH) * 0.5f;
    }

    D2D1_RECT_F dest = D2D1::RectF(x, y, x + dstW, y + dstH);
    d2dContext_->DrawBitmap(bmp.Get(), &dest, p.opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src);
    return true;
}
