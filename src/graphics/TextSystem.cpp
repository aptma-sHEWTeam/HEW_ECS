/**
 * @file TextSystem.cpp
 * @brief TextSystemの実装
 * @author 山内陽
 * @date 2025
 */
#include "graphics/TextSystem.h"
#include <cmath>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <vector>
#include <wincodec.h>
#include <comdef.h>

bool TextSystem::Init(GfxDevice &gfx) {
    try {
    if (initialized_) {
        DEBUGLOG_WARNING("TextSystem already initialized");
        return true;
    }
    gfx_ = &gfx;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = gfx.Dev()->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to get IDXGIDevice from D3D11 device");
        return false;
    }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create D2D1 Factory1");
        return false;
    }

    hr = d2dFactory_->CreateDevice(dxgiDevice.Get(), d2dDevice_.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create D2D1 Device");
        return false;
    }
    hr = d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContext_.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create D2D1 DeviceContext");
        return false;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown **>(dwriteFactory_.GetAddressOf()));
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create DWrite Factory");
        return false;
    }

    // WIC Imaging Factory (for image decoding)
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(wicFactory_.GetAddressOf()));
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create WIC Imaging Factory");
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = gfx.GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to get back buffer");
        return false;
    }

    FLOAT dpiX = 96.0f;
    FLOAT dpiY = 96.0f; // 固定DPI (GetDesktopDpiは非推奨)
    // d2dFactory_->GetDesktopDpi(&dpiX, &dpiY);

    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        dpiX,
        dpiY);
    hr = d2dContext_->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bp, targetBitmap_.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create D2D1 Bitmap1 from back buffer");
        return false;
    }
    d2dContext_->SetTarget(targetBitmap_.Get());

    TextFormat defaultFormat;
    if (!CreateTextFormat("default", defaultFormat)) {
        DEBUGLOG_ERROR("Failed to create default text format");
        return false;
    }

    initialized_ = true;
    DEBUGLOG("TextSystem initialized successfully");
    return true;
    } catch (const _com_error& ex) {
        std::wostringstream woss;
        const wchar_t* wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
        woss << L"TextSystem::Init _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
        std::wstring w = woss.str();
        std::string n(w.begin(), w.end());
        DEBUGLOG_ERROR(n);
        return false;
    }
}

bool TextSystem::CreateTextFormat(const std::string &id, const TextFormat &format) {
    if (!dwriteFactory_) {
        DEBUGLOG_ERROR("DWrite Factory not initialized");
        return false;
    }
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    HRESULT hr = dwriteFactory_->CreateTextFormat(
        format.fontFamily.c_str(),
        nullptr,
        format.weight,
        format.style,
        DWRITE_FONT_STRETCH_NORMAL,
        format.fontSize,
        L"ja-jp",
        textFormat.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("Failed to create text format");
        return false;
    }
    textFormat->SetTextAlignment(format.alignment);
    textFormat->SetParagraphAlignment(format.paragraphAlignment);
    textFormats_[id] = textFormat;
    return true;
}

void TextSystem::DrawText(const TextParams &params) {
    if (!d2dContext_) {
        DEBUGLOG_ERROR("DeviceContext not initialized");
        return;
    }

    auto formatIt = textFormats_.find(params.formatId);
    if (formatIt == textFormats_.end()) {
        DEBUGLOG_ERROR("Text format not found");
        return;
    }

    ID2D1SolidColorBrush *brush = GetOrCreateBrush(params.color);
    if (!brush) {
        DEBUGLOG_ERROR("Failed to create brush");
        return;
    }

    D2D1_RECT_F rect = D2D1::RectF(
        params.x,
        params.y,
        params.x + params.width,
        params.y + params.height);

    // 使用するテキストフォーマットを決定（fontSize > 0 の場合、サイズ上書きフォーマットを一時作成）
    IDWriteTextFormat *formatToUse = formatIt->second.Get();
    Microsoft::WRL::ComPtr<IDWriteTextFormat> overrideFormat;
    if (params.fontSize > 0.0f && dwriteFactory_) {
        IDWriteTextFormat *base = formatIt->second.Get();

        // 基本フォーマットのプロパティを取得
        DWRITE_FONT_WEIGHT weight = base->GetFontWeight();
        DWRITE_FONT_STYLE style = base->GetFontStyle();
        DWRITE_FONT_STRETCH stretch = base->GetFontStretch();
        DWRITE_TEXT_ALIGNMENT textAlign = base->GetTextAlignment();
        DWRITE_PARAGRAPH_ALIGNMENT paraAlign = base->GetParagraphAlignment();

        // フォントファミリー名
        UINT32 famLen = base->GetFontFamilyNameLength();
        std::vector<wchar_t> famBuf(famLen + 1);
        if (SUCCEEDED(base->GetFontFamilyName(famBuf.data(), static_cast<UINT32>(famBuf.size())))) {
            // ロケール
            UINT32 locLen = base->GetLocaleNameLength();
            std::vector<wchar_t> locBuf(locLen + 1);
            if (SUCCEEDED(base->GetLocaleName(locBuf.data(), static_cast<UINT32>(locBuf.size())))) {
                HRESULT hrFmt = dwriteFactory_->CreateTextFormat(
                    famBuf.data(),
                    nullptr,
                    weight,
                    style,
                    stretch,
                    params.fontSize,
                    locBuf.data(),
                    overrideFormat.GetAddressOf());
                if (SUCCEEDED(hrFmt) && overrideFormat) {
                    overrideFormat->SetTextAlignment(textAlign);
                    overrideFormat->SetParagraphAlignment(paraAlign);
                    formatToUse = overrideFormat.Get();
                }
            }
        }
    }

    d2dContext_->DrawTextW(
        params.text.c_str(),
        static_cast<UINT32>(params.text.length()),
        formatToUse,
        &rect,
        brush);
}

void TextSystem::BeginDraw() {
    if (d2dContext_) {
        if (!targetBitmap_) {
            RefreshTargetBitmap();
        }
        d2dContext_->BeginDraw();
    }
}

void TextSystem::EndDraw() {
    if (d2dContext_) {
        HRESULT hr = d2dContext_->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            RefreshTargetBitmap();
        } else if (FAILED(hr)) {
            DEBUGLOG_ERROR("EndDraw failed");
        }
    }
}

void TextSystem::Shutdown() {
    if (!initialized_) {
        return;
    }

    textFormats_.clear();
    brushCache_.clear();
    targetBitmap_.Reset();
    d2dContext_.Reset();
    d2dDevice_.Reset();
    dwriteFactory_.Reset();
    wicFactory_.Reset();
    d2dFactory_.Reset();

    initialized_ = false;
    gfx_ = nullptr;
    DEBUGLOG("TextSystem shutdown");
}

ID2D1SolidColorBrush *TextSystem::GetOrCreateBrush(const DirectX::XMFLOAT4 &color) {
    uint32_t hash = ColorToHash(color);

    auto it = brushCache_.find(hash);
    if (it != brushCache_.end()) {
        return it->second.Get();
    }

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = d2dContext_->CreateSolidColorBrush(
        D2D1::ColorF(color.x, color.y, color.z, color.w),
        brush.GetAddressOf());

    if (FAILED(hr)) {
        return nullptr;
    }

    brushCache_[hash] = brush;
    return brush.Get();
}

uint32_t TextSystem::ColorToHash(const DirectX::XMFLOAT4 &color) const {
    uint32_t r = static_cast<uint32_t>(color.x * 255.0f);
    uint32_t g = static_cast<uint32_t>(color.y * 255.0f);
    uint32_t b = static_cast<uint32_t>(color.z * 255.0f);
    uint32_t a = static_cast<uint32_t>(color.w * 255.0f);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void TextSystem::RefreshTargetBitmap() {
    if (!gfx_ || !d2dContext_)
        return;
    d2dContext_->SetTarget(nullptr);
    targetBitmap_.Reset();
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
    HRESULT hr = gfx_->GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("RefreshTargetBitmap: GetBuffer failed");
        return;
    }
    FLOAT dpiX = 96.0f, dpiY = 96.0f;
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);
    hr = d2dContext_->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bp, targetBitmap_.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("RefreshTargetBitmap: CreateBitmapFromDxgiSurface failed");
        return;
    }
    d2dContext_->SetTarget(targetBitmap_.Get());
}

bool TextSystem::LoadBitmapFromFile(const std::wstring &filePath, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &outBitmap) {
    if (!wicFactory_ || !d2dContext_) return false;

    // Cache check
    auto it = bitmapCache_.find(filePath);
    if (it != bitmapCache_.end()) {
        outBitmap = it->second;
        return true;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = wicFactory_->CreateDecoderFromFilename(
        filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) {
        DEBUGLOG_ERROR("WIC: CreateDecoderFromFilename failed");
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory_->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = converter->Initialize(
        frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
        nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    hr = d2dContext_->CreateBitmapFromWicBitmap(converter.Get(), &props, bitmap.GetAddressOf());
    if (FAILED(hr)) return false;

    bitmapCache_[filePath] = bitmap;
    outBitmap = bitmap;
    return true;
}

bool TextSystem::DrawImage(const ImageParams &params) {
    if (!d2dContext_) return false;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    if (!LoadBitmapFromFile(params.filePath, bitmap)) return false;

    D2D1_SIZE_F bmpSize = bitmap->GetSize();
    float dstW = params.width;
    float dstH = params.height;
    float x = params.x;
    float y = params.y;

    if (params.keepAspect && bmpSize.width > 0 && bmpSize.height > 0) {
        float aspect = bmpSize.width / bmpSize.height;
        float targetAspect = dstW / dstH;
        if (targetAspect > aspect) {
            // fit height
            dstW = dstH * aspect;
            x += (params.width - dstW) * 0.5f;
        } else {
            // fit width
            dstH = dstW / aspect;
            y += (params.height - dstH) * 0.5f;
        }
    }

    D2D1_RECT_F destRect = D2D1::RectF(x, y, x + dstW, y + dstH);
    d2dContext_->DrawBitmap(bitmap.Get(), &destRect, params.opacity);
    return true;
}
