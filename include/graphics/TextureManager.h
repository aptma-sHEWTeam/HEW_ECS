/**
 * @file TextureManager.h
 * @brief テクスチャ管理システム
 * @author 山内陽
 * @date 2025
 * @version 5.0
 *
 * @details
 * 画像ファイルの読み込み、テクスチャの作成・管理を行うシステムです。
 * WIC (Windows Imaging Component) を使用して様々な画像フォーマットに対応しています。
 */
#pragma once

#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <wincodec.h>
#include "graphics/GfxDevice.h"
#include "app/DebugLog.h"

/**
 * @class TextureManager
 * @brief テクスチャ管理システム
 *
 * @details
 * テクスチャの読み込み、作成、管理を一元的に行うクラスです。
 * ハンドルベースの管理により、安全かつ効率的にテクスチャを扱えます。
 *
 * ### 対応フォーマット:
 * - BMP (ビットマップ)
 * - PNG (Portable Network Graphics)
 * - JPG/JPEG (Joint Photographic Experts Group)
 * - その他WIC がサポートする形式
 *
 * ### 使用例
 * @code
 * TextureManager texManager;
 * texManager.Init(gfx);
 *
 * // ファイルから読み込み
 * auto handle = texManager.LoadFromFile("assets/brick.png");
 *
 * // メッシュレンダラーに設定
 * auto* renderer = world.TryGet<MeshRenderer>(entity);
 * if (renderer) {
 *     renderer->texture = handle;
 * }
 *
 * // テクスチャの解放
 * texManager.Release(handle);
 * @endcode
 *
 * @note すべてのテクスチャは RGBA32 フォーマットに変換されます
 *
 * @author 山内陽
 */
class TextureManager {
public:
    /**
     * @typedef TextureHandle
     * @brief テクスチャを識別するハンドル
     */
    using TextureHandle = uint32_t;

    /**
     * @var INVALID_TEXTURE
     * @brief 無効なテクスチャを表す定数値
     */
    static constexpr TextureHandle INVALID_TEXTURE = 0;

    /**
     * @brief 初期化
     * @param[in] gfx グラフィックスデバイス
     * @return bool 初期化が成功した場合は true
     *
     * @details
     * テクスチャマネージャーを初期化し、デフォルトの白テクスチャを作成します。
     */
    bool Init(GfxDevice& gfx) {
        gfx_ = &gfx;
        isShutdown_ = false;

        // デフォルトの白テクスチャを作成
        uint32_t whitePixel = 0xFFFFFFFF;
        defaultWhiteTexture_ = CreateTextureFromMemory(
            reinterpret_cast<const uint8_t*>(&whitePixel),
            1, 1, 4
        );

        return defaultWhiteTexture_ != INVALID_TEXTURE;
    }

    /**
     * @brief ファイルからテクスチャを読み込み(BMP, PNG, JPGなど)
     * @param[in] filepath 画像ファイルのパス
     * @return TextureHandle テクスチャハンドル(失敗時は INVALID_TEXTURE)
     *
     * @details
     * Windows Imaging Component (WIC) を使用して画像を読み込み、
     * DirectX11 テクスチャに変換します。
     *
     * @par 使用例
     * @code
     * auto texture = texManager.LoadFromFile("assets/player.png");
     * if (texture != TextureManager::INVALID_TEXTURE) {
     *     // テクスチャの使用
     * }
     * @endcode
     */
    TextureHandle LoadFromFile(const char* filepath) {
        // WICを使用して画像を読み込む
        Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        );

        if (FAILED(hr)) {
            char msg[512];
            sprintf_s(msg, "Failed to create WIC factory for: %s", filepath);
            MessageBoxA(nullptr, msg, "Texture Load Error", MB_OK | MB_ICONERROR);
            return INVALID_TEXTURE;
        }

        // ワイド文字列に変換
        wchar_t wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, filepath, -1, wpath, MAX_PATH);

        // デコーダーを作成
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        hr = wicFactory->CreateDecoderFromFilename(
            wpath,
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder
        );

        if (FAILED(hr)) {
            char msg[512];
            sprintf_s(msg, "Failed to load image file: %s", filepath);
            MessageBoxA(nullptr, msg, "Texture Load Error", MB_OK | MB_ICONERROR);
            return INVALID_TEXTURE;
        }

        // フレームを取得
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) return INVALID_TEXTURE;

        // RGBA32に変換
        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        hr = wicFactory->CreateFormatConverter(&converter);
        if (FAILED(hr)) return INVALID_TEXTURE;

        hr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom
        );
        if (FAILED(hr)) return INVALID_TEXTURE;

        // サイズを取得
        UINT width, height;
        hr = converter->GetSize(&width, &height);
        if (FAILED(hr)) return INVALID_TEXTURE;

        // ピクセルデータを取得
        std::vector<uint8_t> pixels(width * height * 4);
        hr = converter->CopyPixels(
            nullptr,
            width * 4,
            static_cast<UINT>(pixels.size()),
            pixels.data()
        );
        if (FAILED(hr)) return INVALID_TEXTURE;

        return CreateTextureFromMemory(pixels.data(), width, height, 4);
    }

    /**
     * @brief メモリからテクスチャを作成
     * @param[in] data ピクセルデータ
     * @param[in] width 幅(ピクセル)
     * @param[in] height 高さ(ピクセル)
     * @param[in] channels チャンネル数(通常4: RGBA)
     * @return TextureHandle テクスチャハンドル(失敗時は INVALID_TEXTURE)
     *
     * @details
     * メモリ上のピクセルデータから DirectX11 テクスチャを作成します。
     * プロシージャルテクスチャの生成などに使用できます。
     *
     * @par 使用例
     * @code
     * // 2x2のチェッカーボードパターンを作成
     * uint8_t pixels[] = {
     *     255, 0, 0, 255,    0, 255, 0, 255,
     *     0, 255, 0, 255,    255, 0, 0, 255
     * };
     * auto texture = texManager.CreateTextureFromMemory(pixels, 2, 2, 4);
     * @endcode
     */
    TextureHandle CreateTextureFromMemory(const uint8_t* data, uint32_t width, uint32_t height, uint32_t channels) {
        // 事前条件チェック（クラッシュ防止）
        if (!gfx_ || !gfx_->Dev()) {
            DEBUGLOG_ERROR("TextureManager not initialized: gfx_ is null");
            return INVALID_TEXTURE;
        }
        if (!data) {
            DEBUGLOG_ERROR("CreateTextureFromMemory: data is null");
            return INVALID_TEXTURE;
        }
        if (width == 0 || height == 0) {
            DEBUGLOG_ERROR("CreateTextureFromMemory: width/height must be > 0");
            return INVALID_TEXTURE;
        }
        if (channels != 4) {
            DEBUGLOG_WARNING("CreateTextureFromMemory: channels != 4 with RGBA8 format; forcing 4");
        }

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = data;
        initData.SysMemPitch = width * 4; // RGBA8 固定

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = gfx_->Dev()->CreateTexture2D(&texDesc, &initData, &texture);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("Failed to create texture2D (HRESULT=0x%08X)", hr);
            MessageBoxA(nullptr, "Failed to create texture2D", "Texture Error", MB_OK | MB_ICONERROR);
            return INVALID_TEXTURE;
        }

        // シェーダーリソースビューを作成
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = gfx_->Dev()->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("Failed to create SRV (HRESULT=0x%08X)");
            MessageBoxA(nullptr, "Failed to create SRV", "Texture Error", MB_OK | MB_ICONERROR);
            return INVALID_TEXTURE;
        }

        // テクスチャを登録
        TextureHandle handle = nextHandle_++;
        TextureData texData;
        texData.texture = texture;
        texData.srv = srv;
        texData.width = width;
        texData.height = height;
        textures_[handle] = texData;

        return handle;
    }

    /**
     * @brief テクスチャの取得
     * @param[in] handle テクスチャハンドル
     * @return ID3D11ShaderResourceView* シェーダーリソースビュー(失敗時は nullptr)
     *
     * @details
     * ハンドルから ShaderResourceView を取得します。
     * これをシェーダーにバインドすることでテクスチャを使用できます。
     *
     * @par 使用例
     * @code
     * ID3D11ShaderResourceView* srv = texManager.GetSRV(textureHandle);
     * if (srv) {
     *     deviceContext->PSSetShaderResources(0, 1, &srv);
     * }
     * @endcode
     */
    ID3D11ShaderResourceView* GetSRV(TextureHandle handle) const {
        if (handle == INVALID_TEXTURE) return nullptr;
        auto it = textures_.find(handle);
        if (it == textures_.end()) return nullptr;
        return it->second.srv.Get();
    }

    /**
     * @brief デフォルトテクスチャ(白色)を取得
     * @return TextureHandle 白色テクスチャのハンドル
     *
     * @details
     * システムが自動的に作成する1x1の白色テクスチャです。
     * テクスチャが指定されていない場合のフォールバックに使用できます。
     */
    TextureHandle GetDefaultWhite() const { return defaultWhiteTexture_; }

    /**
     * @brief テクスチャの解放
     * @param[in] handle テクスチャハンドル
     *
     * @details
     * 指定されたテクスチャをメモリから解放します。
     * 解放後、そのハンドルは無効になります。
     *
     * @par 使用例
     * @code
     * texManager.Release(textureHandle);
     * @endcode
     */
    void Release(TextureHandle handle) {
        if (handle == INVALID_TEXTURE || handle == defaultWhiteTexture_) {
            return;
        }
        textures_.erase(handle);
    }

    /**
     * @brief デストラクタ
     *
     * @details
     * 管理しているすべてのテクスチャを自動的に解放します。
     */
    ~TextureManager() {
        DEBUGLOG("TextureManager::~TextureManager() - デストラクタ呼び出し");
        if (!isShutdown_) { DEBUGLOG_WARNING("TextureManager::Shutdown()が明示的に呼ばれていません。デストラクタで自動クリーンアップします。"); }
        Shutdown();
    }

    /**
     * @brief リソースの明示的解放
     */
    void Shutdown() {
        if (isShutdown_) return; // 冪等性
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "TextureManager::Shutdown() - " + std::to_string(textures_.size()) + " 個のテクスチャを解放中");

        // 各テクスチャの詳細をログ
        int textureCount = 0;
        int srvCount = 0;
        for (const auto& pair : textures_) {
            if (pair.second.texture) textureCount++;
            if (pair.second.srv) srvCount++;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "テクスチャ2D: " + std::to_string(textureCount) + " 個");
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "シェーダーリソースビュー: " + std::to_string(srvCount) + " 個");

        textures_.clear();
        isShutdown_ = true;
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "TextureManager::Shutdown() 完了");
    }

private:
    /**
     * @struct TextureData
     * @brief テクスチャの内部データ
     */
    struct TextureData {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        uint32_t width;
        uint32_t height;
    };

    GfxDevice* gfx_ = nullptr;                          ///< グラフィックスデバイスへのポインタ
    TextureHandle nextHandle_ = 1;                      ///< 次に割り当てるハンドル
    TextureHandle defaultWhiteTexture_ = INVALID_TEXTURE; ///< デフォルト白テクスチャ
    std::unordered_map<TextureHandle, TextureData> textures_; ///< テクスチャマップ
    bool isShutdown_ = false;                           ///< シャットダウン済みフラグ
};
