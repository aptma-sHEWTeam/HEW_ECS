/**
 * @file TextSystem.cpp
 * @brief TextSystemの実装
 * @author 山内陽
 * @date 2025
 */
#include "graphics/TextSystem.h"
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cwctype>
#include <new>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <vector>
#include <wincodec.h>
#include <comdef.h>

namespace {
class FontFileEnumerator final : public IDWriteFontFileEnumerator {
  public:
    FontFileEnumerator(IDWriteFactory *factory, const std::vector<std::wstring> &files)
        : refCount_(1), factory_(factory), files_(files) {
        if (factory_) {
            factory_->AddRef();
        }
    }

    ~FontFileEnumerator() {
        if (factory_) {
            factory_->Release();
            factory_ = nullptr;
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override {
        if (!ppvObject) {
            return E_POINTER;
        }
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontFileEnumerator)) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++refCount_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --refCount_;
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE MoveNext(BOOL *hasCurrentFile) override {
        if (!hasCurrentFile) {
            return E_POINTER;
        }
        if (index_ >= files_.size()) {
            *hasCurrentFile = FALSE;
            currentFile_.Reset();
            return S_OK;
        }

        HRESULT hr = factory_->CreateFontFileReference(files_[index_].c_str(), nullptr, currentFile_.GetAddressOf());
        if (SUCCEEDED(hr)) {
            *hasCurrentFile = TRUE;
            ++index_;
        } else {
            *hasCurrentFile = FALSE;
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile **fontFile) override {
        if (!fontFile) {
            return E_POINTER;
        }
        if (!currentFile_) {
            return E_FAIL;
        }
        *fontFile = currentFile_.Get();
        (*fontFile)->AddRef();
        return S_OK;
    }

  private:
    std::atomic<ULONG> refCount_;
    IDWriteFactory *factory_ = nullptr;
    std::vector<std::wstring> files_;
    size_t index_ = 0;
    Microsoft::WRL::ComPtr<IDWriteFontFile> currentFile_;
};

class FontCollectionLoader final : public IDWriteFontCollectionLoader {
  public:
    explicit FontCollectionLoader(const std::vector<std::wstring> &files)
        : refCount_(1), files_(files) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override {
        if (!ppvObject) {
            return E_POINTER;
        }
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontCollectionLoader)) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++refCount_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --refCount_;
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
        IDWriteFactory *factory,
        const void *collectionKey,
        UINT32 collectionKeySize,
        IDWriteFontFileEnumerator **fontFileEnumerator) override {
        (void) collectionKey;
        (void) collectionKeySize;
        if (!factory || !fontFileEnumerator) {
            return E_INVALIDARG;
        }
        auto *enumerator = new (std::nothrow) FontFileEnumerator(factory, files_);
        if (!enumerator) {
            return E_OUTOFMEMORY;
        }
        *fontFileEnumerator = enumerator;
        return S_OK;
    }

  private:
    std::atomic<ULONG> refCount_;
    std::vector<std::wstring> files_;
};

std::wstring ToLowerCopy(const std::wstring &value) {
    std::wstring out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return out;
}

bool EndsWithIgnoreCase(const std::wstring &value, const std::wstring &suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    std::wstring tail = value.substr(value.size() - suffix.size());
    return ToLowerCopy(tail) == ToLowerCopy(suffix);
}

std::wstring StripStyleSuffix(std::wstring name) {
    const std::wstring regularSuffix = L"-Regular";
    const std::wstring normalSuffix = L"-Normal";
    if (EndsWithIgnoreCase(name, regularSuffix)) {
        name.erase(name.size() - regularSuffix.size());
    }
    if (EndsWithIgnoreCase(name, normalSuffix)) {
        name.erase(name.size() - normalSuffix.size());
    }
    return name;
}

std::wstring ExtractBaseName(const std::wstring &path) {
    std::wstring base = path;
    size_t pos = base.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        base = base.substr(pos + 1);
    }
    size_t dot = base.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        base = base.substr(0, dot);
    }
    return StripStyleSuffix(base);
}

bool ContainsIgnoreCase(const std::wstring &value, const std::wstring &needle) {
    if (needle.empty()) {
        return false;
    }
    std::wstring haystackLower = ToLowerCopy(value);
    std::wstring needleLower = ToLowerCopy(needle);
    return haystackLower.find(needleLower) != std::wstring::npos;
}
} // namespace

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

        InitCustomFontCollection();

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
    } catch (const _com_error &ex) {
        std::wostringstream woss;
        const wchar_t *wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
        woss << L"TextSystem::Init _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
        //std::wstring w = woss.str();
        //std::string n(w.begin(), w.end());
        //DEBUGLOG_ERROR(n);
        return false;
    }
}

bool TextSystem::CreateTextFormat(const std::string &id, const TextFormat &format) {
    if (!dwriteFactory_) {
        DEBUGLOG_ERROR("DWrite Factory not initialized");
        return false;
    }
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    std::wstring resolvedFamily = format.fontFamily;
    IDWriteFontCollection *fontCollection = nullptr;
    if (TryResolveCustomFontFamily(format.fontFamily, resolvedFamily)) {
        fontCollection = customFontCollection_.Get();
    }
    HRESULT hr = dwriteFactory_->CreateTextFormat(
        resolvedFamily.c_str(),
        fontCollection,
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

bool TextSystem::InitCustomFontCollection() {
    if (!dwriteFactory_) {
        return false;
    }
    customFontFiles_.clear();
    customFontFiles_.push_back(L"./Assets/Fonts/Mamelon-5-Hi-Regular.otf");
    customFontFiles_.push_back(L"./Assets/Fonts/Kinkakuji-Normal.ttf");

    auto *loader = new (std::nothrow) FontCollectionLoader(customFontFiles_);
    if (!loader) {
        return false;
    }
    customFontCollectionLoader_.Attach(loader);

    HRESULT hr = dwriteFactory_->RegisterFontCollectionLoader(customFontCollectionLoader_.Get());
    if (FAILED(hr)) {
        customFontCollectionLoader_.Reset();
        return false;
    }

    hr = dwriteFactory_->CreateCustomFontCollection(
        customFontCollectionLoader_.Get(),
        nullptr,
        0,
        customFontCollection_.GetAddressOf());
    if (FAILED(hr)) {
        dwriteFactory_->UnregisterFontCollectionLoader(customFontCollectionLoader_.Get());
        customFontCollectionLoader_.Reset();
        customFontCollection_.Reset();
        return false;
    }
    return true;
}

bool TextSystem::TryResolveCustomFontFamily(const std::wstring &requestedFamily, std::wstring &outFamily) const {
    if (!customFontCollection_ || requestedFamily.empty()) {
        return false;
    }

    UINT32 index = 0;
    BOOL exists = FALSE;
    if (SUCCEEDED(customFontCollection_->FindFamilyName(requestedFamily.c_str(), &index, &exists)) && exists) {
        outFamily = requestedFamily;
        return true;
    }

    std::wstring key = ExtractBaseName(requestedFamily);
    if (key.empty()) {
        return false;
    }

    UINT32 familyCount = customFontCollection_->GetFontFamilyCount();
    for (UINT32 i = 0; i < familyCount; ++i) {
        Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
        if (FAILED(customFontCollection_->GetFontFamily(i, family.GetAddressOf()))) {
            continue;
        }
        Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> names;
        if (FAILED(family->GetFamilyNames(names.GetAddressOf()))) {
            continue;
        }

        UINT32 nameIndex = 0;
        BOOL nameExists = FALSE;
        if (SUCCEEDED(names->FindLocaleName(L"en-us", &nameIndex, &nameExists)) && !nameExists) {
            nameIndex = 0;
        }

        UINT32 length = 0;
        if (FAILED(names->GetStringLength(nameIndex, &length))) {
            continue;
        }
        std::wstring familyName(length + 1, L'\0');
        if (FAILED(names->GetString(nameIndex, familyName.data(), length + 1))) {
            continue;
        }
        familyName.resize(length);
        if (ContainsIgnoreCase(familyName, key)) {
            outFamily = familyName;
            return true;
        }
    }
    return false;
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

    auto drawTextWithBrush = [&](ID2D1Brush *brush, float offsetX, float offsetY) {
        if (!brush) {
            DEBUGLOG_ERROR("Failed to create brush");
            return;
        }
        D2D1_RECT_F rect = D2D1::RectF(
            params.x + offsetX,
            params.y + offsetY,
            params.x + params.width + offsetX,
            params.y + params.height + offsetY);
        d2dContext_->DrawTextW(
            params.text.c_str(),
            static_cast<UINT32>(params.text.length()),
            formatToUse,
            &rect,
            brush);
    };

    auto drawTextWithColor = [&](const DirectX::XMFLOAT4 &color, float offsetX, float offsetY) {
        ID2D1SolidColorBrush *brush = GetOrCreateBrush(color);
        if (!brush) {
            DEBUGLOG_ERROR("Failed to create brush");
            return;
        }
        drawTextWithBrush(brush, offsetX, offsetY);
    };

    if (params.outlineThickness > 0.0f && params.outlineColor.w > 0.0f) {
        const float t = params.outlineThickness;
        const float offsets[][2] = {
            {-t, 0.0f},
            {t, 0.0f},
            {0.0f, -t},
            {0.0f, t},
            {-t, -t},
            {-t, t},
            {t, -t},
            {t, t},
        };
        for (const auto &offset : offsets) {
            drawTextWithColor(params.outlineColor, offset[0], offset[1]);
        }
    }

    ID2D1Brush *fillBrush = nullptr;
    if (!params.fillTexturePath.empty()) {
        if (auto *bitmapBrush = GetOrCreateBitmapBrush(params.fillTexturePath)) {
            bitmapBrush->SetTransform(D2D1::Matrix3x2F::Translation(params.x, params.y));
            fillBrush = bitmapBrush;
        }
    }

    if (fillBrush) {
        drawTextWithBrush(fillBrush, 0.0f, 0.0f);
    } else {
        drawTextWithColor(params.color, 0.0f, 0.0f);
    }
}

void TextSystem::FillRect(float x, float y, float width, float height, const DirectX::XMFLOAT4 &color) {
    if (!d2dContext_) {
        DEBUGLOG_ERROR("DeviceContext not initialized");
        return;
    }

    ID2D1SolidColorBrush *brush = GetOrCreateBrush(color);
    if (!brush) {
        DEBUGLOG_ERROR("Failed to create brush");
        return;
    }

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    d2dContext_->FillRectangle(&rect, brush);
}

void TextSystem::BeginDraw() {
    if (gfx_) gfx_->Resolve();

    if (d2dContext_) {
        if (!targetBitmap_) {
            RefreshTargetBitmap();
        }
        d2dContext_->BeginDraw();

        // Calculate and apply scaling if logical size is set
        baseTransform_ = D2D1::Matrix3x2F::Identity();
        if (gfx_ && logicalWidth_ > 0.0f && logicalHeight_ > 0.0f) {
            D3D11_VIEWPORT vp = gfx_->GetCurrentViewport();
            float scaleX = vp.Width / logicalWidth_;
            float scaleY = vp.Height / logicalHeight_;
            
            baseTransform_ = D2D1::Matrix3x2F::Scale(scaleX, scaleY) * 
                             D2D1::Matrix3x2F::Translation(vp.TopLeftX, vp.TopLeftY);
        }
        d2dContext_->SetTransform(baseTransform_);
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
    bitmapBrushCache_.clear();
    targetBitmap_.Reset();
    d2dContext_.Reset();
    d2dDevice_.Reset();
    if (dwriteFactory_ && customFontCollectionLoader_) {
        dwriteFactory_->UnregisterFontCollectionLoader(customFontCollectionLoader_.Get());
    }
    customFontCollection_.Reset();
    customFontCollectionLoader_.Reset();
    customFontFiles_.clear();
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

ID2D1BitmapBrush *TextSystem::GetOrCreateBitmapBrush(const std::wstring &filePath) {
    if (!d2dContext_ || filePath.empty()) {
        return nullptr;
    }

    auto it = bitmapBrushCache_.find(filePath);
    if (it != bitmapBrushCache_.end()) {
        return it->second.Get();
    }

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    if (!LoadBitmapFromFile(filePath, bitmap)) {
        return nullptr;
    }

    D2D1_BITMAP_BRUSH_PROPERTIES brushProps = D2D1::BitmapBrushProperties(
        D2D1_EXTEND_MODE_CLAMP,
        D2D1_EXTEND_MODE_CLAMP,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    D2D1_BRUSH_PROPERTIES baseProps = D2D1::BrushProperties(1.0f, D2D1::Matrix3x2F::Identity());

    Microsoft::WRL::ComPtr<ID2D1BitmapBrush> brush;
    HRESULT hr = d2dContext_->CreateBitmapBrush(bitmap.Get(), brushProps, baseProps, brush.GetAddressOf());
    if (FAILED(hr)) {
        return nullptr;
    }

    bitmapBrushCache_[filePath] = brush;
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
    if (!wicFactory_ || !d2dContext_)
        return false;

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
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory_->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = converter->Initialize(
        frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
        nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
        return false;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    hr = d2dContext_->CreateBitmapFromWicBitmap(converter.Get(), &props, bitmap.GetAddressOf());
    if (FAILED(hr))
        return false;

    bitmapCache_[filePath] = bitmap;
    outBitmap = bitmap;
    return true;
}

bool TextSystem::DrawImage(const ImageParams &params) {
    if (!d2dContext_)
        return false;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
    if (!LoadBitmapFromFile(params.filePath, bitmap))
        return false;

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
