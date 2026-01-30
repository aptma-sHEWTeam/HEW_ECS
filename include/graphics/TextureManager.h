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
#include "graphics/GfxDevice.h"
#include "app/DebugLog.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <fstream>
#include <cstdio>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

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

        if (!wicFactory_) {
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory_)
            );

            if (FAILED(hr)) {
                std::ostringstream oss;
                oss << "TextureManager::Init() - Failed to create WIC imaging factory (HRESULT=0x"
                    << std::uppercase << std::hex << hr << ")";
                DEBUGLOG_ERROR(oss.str());
                return false;
            }
        }

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
    /**
     * @brief ファイルからテクスチャを読み込み(BMP, PNG, JPGなど)
     * @param[in] filepath 画像ファイルのパス
     * @return TextureHandle テクスチャハンドル(失敗時は INVALID_TEXTURE)
     */
    TextureHandle LoadFromFile(const char* filepath) {
        // パスを正規化（簡易的）
        std::string pathStr = filepath;
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

        // キャッシュチェック
        auto it = textureCache_.find(pathStr);
        if (it != textureCache_.end()) {
            return it->second;
        }

        // WICを使用して画像を読み込む
        if (!wicFactory_) {
            DEBUGLOG_ERROR("TextureManager::LoadFromFile() - WIC factory not initialised");
            return INVALID_TEXTURE;
        }
        
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Loading texture: " + pathStr);

        HRESULT hr = S_OK;
        // ワイド文字列に変換
        wchar_t wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, filepath, -1, wpath, MAX_PATH);

        // デコーダーを作成
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        hr = wicFactory_->CreateDecoderFromFilename(
            wpath,
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder
        );

        if (FAILED(hr)) {
            DEBUGLOG_ERROR(std::string("Failed to load image file: ") + filepath);
#ifdef _DEBUG
            MessageBoxA(nullptr, (std::string("Failed to load image file: ") + filepath).c_str(), "Texture Load Error", MB_OK | MB_ICONERROR);
#endif
            return INVALID_TEXTURE;
        }

        // フレームを取得
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) return INVALID_TEXTURE;

        // RGBA32に変換
        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        hr = wicFactory_->CreateFormatConverter(&converter);
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

        std::vector<uint8_t> pixels;
        UINT maxDim = 16384; // DX11 Max Texture Size

        if (width > maxDim || height > maxDim) {
            float scale = 1.0f;
            if (width > height) {
                scale = static_cast<float>(maxDim) / width;
            } else {
                scale = static_cast<float>(maxDim) / height;
            }
            UINT newWidth = static_cast<UINT>(width * scale);
            UINT newHeight = static_cast<UINT>(height * scale);

            std::string msg = "Texture too large (" + std::to_string(width) + "x" + std::to_string(height) + "). Scaling down to (" + std::to_string(newWidth) + "x" + std::to_string(newHeight) + "): " + pathStr;
            DEBUGLOG_WARNING(msg);

            Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
            hr = wicFactory_->CreateBitmapScaler(&scaler);
            if (FAILED(hr)) return INVALID_TEXTURE;

            hr = scaler->Initialize(
                converter.Get(),
                newWidth,
                newHeight,
                WICBitmapInterpolationModeFant
            );
            if (FAILED(hr)) return INVALID_TEXTURE;

            width = newWidth;
            height = newHeight;
            pixels.resize(width * height * 4);
            hr = scaler->CopyPixels(
                nullptr,
                width * 4,
                static_cast<UINT>(pixels.size()),
                pixels.data()
            );
            
             if (FAILED(hr)) { 
                DEBUGLOG_ERROR("Failed to copy pixels from scaler");
                return INVALID_TEXTURE;
             }
        } else {
            // 通常読み込み
            pixels.resize(width * height * 4);
            hr = converter->CopyPixels(
                nullptr,
                width * 4,
                static_cast<UINT>(pixels.size()),
                pixels.data()
            );
            if (FAILED(hr)) return INVALID_TEXTURE;
        }

        TextureHandle handle = CreateTextureFromMemory(pixels.data(), width, height, 4);
        if (handle != INVALID_TEXTURE) {
            textureCache_[pathStr] = handle;
            // DEBUGLOG_CATEGORYがコメントアウトされていたので復帰
             DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Texture loaded and cached: " + pathStr);
        }
        return handle;
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
    /**
     * @brief メモリからテクスチャを作成
     * @param[in] data ピクセルデータ
     * @param[in] width 幅(ピクセル)
     * @param[in] height 高さ(ピクセル)
     * @param[in] channels チャンネル数(通常4: RGBA)
     * @return TextureHandle テクスチャハンドル(失敗時は INVALID_TEXTURE)
     */
    TextureHandle CreateTextureFromMemory(const uint8_t* data, uint32_t width, uint32_t height, uint32_t channels) {
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 0; // 全ミップレベルを作成
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // GenerateMipsにはRTが必要
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        // InitDataを渡さずに作成
        HRESULT hr = gfx_->Dev()->CreateTexture2D(&texDesc, nullptr, &texture);
        if (FAILED(hr)) {
            // フォールバック: Mipmapなしで再試行
            std::string msg = "Failed to create texture2D (Mipmap enabled). W: " + std::to_string(width) + " H: " + std::to_string(height) + ". Retrying without Mipmaps.";
            DEBUGLOG_WARNING(msg); // ErrorではなくWarningに
            
            texDesc.MipLevels = 1;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // RT外す
            texDesc.MiscFlags = 0; // GenerateMips外す
            
            hr = gfx_->Dev()->CreateTexture2D(&texDesc, nullptr, &texture);
            if (FAILED(hr)) {
                std::string msg2 = "Failed to create texture2D (Fallback).";
                DEBUGLOG_ERROR(msg2);
#ifdef _DEBUG
                MessageBoxA(nullptr, msg2.c_str(), "Texture Error", MB_OK | MB_ICONERROR);
#endif
                return INVALID_TEXTURE;
            }
        }

        // 初期データ転送 (Mip Level 0)
        // UpdateSubresourceを使用
        gfx_->Ctx()->UpdateSubresource(
            texture.Get(),
            0, 
            nullptr, 
            data, 
            width * channels, 
            0
        );

        // シェーダーリソースビューを作成
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = texDesc.MipLevels; // 作成されたMipLevelsを使用 (0なら-1相当だが、Create時は0指定でフルチェーン作成されるので、ここも合わせる必要がある)
        // 注意: Fallbackした場合、MipLevels=1になっている。
        // Original(MipLevels=0)の場合、SRVには-1(全レベル)を指定する。
        // なので、texDesc.MipLevelsが0なら-1、それ以外ならtexDesc.MipLevelsを使う。
        srvDesc.Texture2D.MipLevels = (texDesc.MipLevels == 0) ? -1 : texDesc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = gfx_->Dev()->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("Failed to create SRV");
            return INVALID_TEXTURE;
        }

        // ミップマップ生成 (MiscFlagがある場合のみ)
        if (texDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) {
            gfx_->Ctx()->GenerateMips(srv.Get());
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
     * @brief テクスチャのピクセル寸法を取得
     * @param[in] handle テクスチャハンドル
     * @param[out] width 幅
     * @param[out] height 高さ
     * @return bool 取得に成功したか
     */
    bool GetSize(TextureHandle handle, uint32_t& width, uint32_t& height) const {
        if (handle == INVALID_TEXTURE) return false;
        auto it = textures_.find(handle);
        if (it == textures_.end()) return false;
        width = it->second.width;
        height = it->second.height;
        return true;
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
        textureCache_.clear();
        wicFactory_.Reset();
        defaultWhiteTexture_ = INVALID_TEXTURE;
        gfx_ = nullptr;
        isShutdown_ = true;
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "TextureManager::Shutdown() 完了");
    }

    /**
     * @brief キューブマップを読み込み
     * @param[in] filepaths 6つの画像ファイルパス (順序: +X, -X, +Y, -Y, +Z, -Z)
     * @return TextureHandle テクスチャハンドル
     */
    TextureHandle LoadCubemap(const std::vector<std::string>& filepaths) {
        if (filepaths.size() != 6) {
            DEBUGLOG_ERROR("LoadCubemap: 6つのファイルパスが必要です");
            return INVALID_TEXTURE;
        }

        std::vector<std::vector<uint8_t>> facePixels(6);
        UINT width = 0, height = 0;

        for (int i = 0; i < 6; ++i) {
            UINT w, h;
            if (!LoadImageBytes(filepaths[i].c_str(), facePixels[i], w, h)) {
                DEBUGLOG_ERROR("LoadCubemap: 画像の読み込みに失敗 - " + filepaths[i]);
                return INVALID_TEXTURE;
            }
            if (i == 0) {
                width = w;
                height = h;
            } else {
                if (w != width || h != height) {
                    DEBUGLOG_ERROR("LoadCubemap: 画像サイズが一致しません - " + filepaths[i]);
                    return INVALID_TEXTURE;
                }
            }
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 0; // Mipmap有効化
        texDesc.ArraySize = 6;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

        // InitDataは使わずCreate
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = gfx_->Dev()->CreateTexture2D(&texDesc, nullptr, &texture);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("LoadCubemap: Texture2Dの作成に失敗");
            return INVALID_TEXTURE;
        }

        // 各面のデータを転送
        for (int i = 0; i < 6; ++i) {
            UINT subresource = D3D11CalcSubresource(0, i, texDesc.MipLevels); // Mip 0, Slice i (MipLevelsが0の場合、API的には全レベル数が必要だが、0指定で初期化されてるので...実レベル数を取得する必要あり？)
            // 実際は MipLevels=0で作成した場合、D3Dが完全なミップチェーンを作成する。
            // D3D11_TEXTURE2D_DESCを確認しても実際のレベル数はわからないが、GetDescする必要があるかも？
            // ただし UpdateSubresource の subresource index は  MipSlice + (ArraySlice * MipLevels)
            // ここで MipLevels=0 なので計算がややこしい。
            // 正しくは、作成後のテクスチャからMipLevelsを取得するのが安全。
            
            // 下記で取得
        }
        
        // 正確なMipLevelsを取得
        D3D11_TEXTURE2D_DESC descCreated;
        texture->GetDesc(&descCreated);
        
        for (int i = 0; i < 6; ++i) {
            UINT subresource = D3D11CalcSubresource(0, i, descCreated.MipLevels);
            gfx_->Ctx()->UpdateSubresource(
                texture.Get(),
                subresource,
                nullptr,
                facePixels[i].data(),
                width * 4,
                0
            );
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = -1;
        srvDesc.TextureCube.MostDetailedMip = 0;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = gfx_->Dev()->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("LoadCubemap: SRVの作成に失敗");
            return INVALID_TEXTURE;
        }

        // ミップマップ生成
        gfx_->Ctx()->GenerateMips(srv.Get());

        TextureHandle handle = nextHandle_++;
        TextureData texData;
        texData.texture = texture;
        texData.srv = srv;
        texData.width = width;
        texData.height = height;
        textures_[handle] = texData;

        return handle;
    }

private:
    bool LoadImageBytes(const char* filepath, std::vector<uint8_t>& outPixels, UINT& outWidth, UINT& outHeight) {
        if (!wicFactory_) return false;

        wchar_t wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, filepath, -1, wpath, MAX_PATH);

        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(wicFactory_->CreateDecoderFromFilename(wpath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder))) return false;

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, &frame))) return false;

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        if (FAILED(wicFactory_->CreateFormatConverter(&converter))) return false;

        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) return false;

        if (FAILED(converter->GetSize(&outWidth, &outHeight))) return false;

        outPixels.resize(outWidth * outHeight * 4);
        if (FAILED(converter->CopyPixels(nullptr, outWidth * 4, static_cast<UINT>(outPixels.size()), outPixels.data()))) return false;

        return true;
    }

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
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_; ///< Shared WIC factory instance
    TextureHandle nextHandle_ = 1;                      ///< 次に割り当てるハンドル
    TextureHandle defaultWhiteTexture_ = INVALID_TEXTURE; ///< デフォルト白テクスチャ
    std::unordered_map<TextureHandle, TextureData> textures_; ///< テクスチャマップ
    std::unordered_map<std::string, TextureHandle> textureCache_; ///< パスによるキャッシュ
    bool isShutdown_ = false;                           ///< シャットダウン済みフラグ
};

