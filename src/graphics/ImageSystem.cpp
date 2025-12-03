/**
 * @file ImageSystem.cpp
 * @brief ImageSystem ŽÀ‘•
 */
#include "graphics/ImageSystem.h"
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wincodec.h>

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
    auto it = bitmapCache_.find(filePath);
    if (it != bitmapCache_.end()) { out = it->second; return true; }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = wicFactory_->CreateDecoderFromFilename(filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) { DEBUGLOG_ERROR("ImageSystem: CreateDecoderFromFilename failed"); return false; }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory_->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bmp;
    hr = d2dContext_->CreateBitmapFromWicBitmap(converter.Get(), &props, bmp.GetAddressOf());
    if (FAILED(hr)) return false;

    bitmapCache_[filePath] = bmp;
    out = bmp;
    return true;
}

bool ImageSystem::Draw(const Params &p) {
    if (!d2dContext_) return false;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bmp;
    if (!LoadBitmap(p.filePath, bmp)) return false;

    D2D1_SIZE_F size = bmp->GetSize();
    float dstW = p.width;
    float dstH = p.height;
    float x = p.x;
    float y = p.y;

    if (p.keepAspect && size.width > 0 && size.height > 0) {
        float aspect = size.width / size.height;
        float targetAspect = dstW / dstH;
        if (targetAspect > aspect) {
            dstW = dstH * aspect;
            x += (p.width - dstW) * 0.5f;
        } else {
            dstH = dstW / aspect;
            y += (p.height - dstH) * 0.5f;
        }
    }

    D2D1_RECT_F dest = D2D1::RectF(x, y, x + dstW, y + dstH);
    d2dContext_->DrawBitmap(bmp.Get(), &dest, p.opacity);
    return true;
}
