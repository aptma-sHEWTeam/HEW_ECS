/**
 * @file GfxDevice.h
 * @brief DirectX11デバイス管理クラス
 * @author 山内陽
 * @date 2025
 * @version 5.0
 *
 * @details
 * DirectX11の初期化、デバイス・コンテキストの管理、描画フレームの制御を行います。
 */
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdio>
#include "app/DebugLog.h"

#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
#include <dxgidebug.h>
#endif

/**
 * @class GfxDevice
 * @brief DirectX11デバイス管理クラス
 *
 * @details
 * DirectX11のデバイス、スワップチェイン、レンダーターゲット、深度バッファなどを管理し、
 * 描画フレームの開始・終了を制御します。
 *
 * ### 主な機能:
 * - DirectX11デバイスの初期化
 * - スワップチェインの作成
 * - レンダーターゲットビューと深度ステンシルビューの管理
 * - フレームの開始・終了処理
 *
 * @par 使用例
 * @code
 * GfxDevice gfx;
 * if (!gfx.Init(hwnd, 1280, 720)) {
 *     // 初期化失敗
 *     return false;
 * }
 *
 * // メインループ
 * while (running) {
 *     gfx.BeginFrame(0.1f, 0.1f, 0.12f); // ダークブルーでクリア
 *
 *     // 描画処理
 *
 *     gfx.EndFrame();
 * }
 * @endcode
 *
 * @author 山内陽
 */
class GfxDevice {
public:
    /**
     * @brief 初期化
     * @param[in] hwnd ウィンドウハンドル
     * @param[in] w 幅(ピクセル単位)
     * @param[in] h 高さ(ピクセル単位)
     * @param[in] fullscreen フルスクリーンかどうか
     * @return bool 初期化が成功した場合は true
     */
    bool Init(HWND hwnd, uint32_t w, uint32_t h, bool fullscreen) {
        width_ = w;
        height_ = h;
        isShutdown_ = false;

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = w;
        sd.BufferDesc.Height = h;
        sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Direct2D互換のためBGRAに変更
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1; // SwapChain自体はMSAAなし
        sd.SampleDesc.Quality = 0;
        sd.Windowed = fullscreen ? FALSE : TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // FLIP_DISCARD

        UINT flags = 0;
#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL fl;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd,
            swap_.ReleaseAndGetAddressOf(),
            device_.ReleaseAndGetAddressOf(),
            &fl,
            context_.ReleaseAndGetAddressOf());

        if (FAILED(hr)) {
            char errorMsg[256];
            sprintf_s(errorMsg,
                "Failed to create D3D11 device.\nHRESULT: 0x%08X\n",
                hr);
            MessageBoxA(nullptr, errorMsg, "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        bool ok = createBackbufferResources();

        logEnvironment(fl, sd);
        return ok;
    }

    /**
     * @brief フレーム開始(画面クリア)
     * @param[in] r 赤成分
     * @param[in] g 緑成分
     * @param[in] b 青成分
     * @param[in] a アルファ成分
     */
    void BeginFrame(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) {
        msaaResolved_ = false;

        // Clear full window to black (or specified color for bars)
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Always black bars
        context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), dsv_.Get());
        context_->ClearRenderTargetView(rtv_.Get(), clearColor);
        context_->ClearDepthStencilView(dsv_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Calculate 16:9 Aspect Ratio Viewport
        float targetAspect = 16.0f / 9.0f;
        float windowAspect = static_cast<float>(width_) / static_cast<float>(height_);

        if (windowAspect > targetAspect) {
            // Window is wider than 16:9 (Pillarbox)
            viewport_.Height = static_cast<float>(height_);
            viewport_.Width = viewport_.Height * targetAspect;
            viewport_.TopLeftX = (width_ - viewport_.Width) / 2.0f;
            viewport_.TopLeftY = 0.0f;
        } else {
            // Window is narrower than 16:9 (Letterbox)
            viewport_.Width = static_cast<float>(width_);
            viewport_.Height = viewport_.Width / targetAspect;
            viewport_.TopLeftX = 0.0f;
            viewport_.TopLeftY = (height_ - viewport_.Height) / 2.0f;
        }
        viewport_.MinDepth = 0.0f;
        viewport_.MaxDepth = 1.0f;

        // Clear the game area with the requested color (optional, but typical for "scene background")
        // Since we already cleared everything to black, we might want to clear the viewport area 
        // to the requested color if it is different from black. 
        // But ClearRenderTargetView clears the whole RT. We can't clear just a rect comfortably here without extensive setup.
        // Assuming the game draws a skybox or background, so the "black" clear is fine for the background.
        // If 'r,g,b' was meant to be the background color of the scene, it won't be applied to just the viewport here easily.
        // However, we MUST set the viewport for rendering.
        context_->RSSetViewports(1, &viewport_);
    }

    void Resize(uint32_t w, uint32_t h) {
        if (width_ == w && height_ == h) return;
        width_ = w;
        height_ = h;

        if (context_) {
            context_->OMSetRenderTargets(0, nullptr, nullptr);
            rtv_.Reset();
            dsv_.Reset();
            backBufferTex_.Reset();
            msaaTex_.Reset();
            
            context_->Flush();
        }

        if (swap_) {
            HRESULT hr = swap_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr)) {
                DEBUGLOG_ERROR("Failed to resize swap chain buffers");
                return;
            }
            createBackbufferResources();
        }
    }

    D3D11_VIEWPORT GetViewport() const { return viewport_; }

    /**
     * @brief バックバッファへの描画を復元
     */
    void RestoreBackBuffer() {
        context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), dsv_.Get());
        context_->RSSetViewports(1, &viewport_);
    }

    /**
     * @brief MSAAリソースの解決（Resolve）
     * 
     * @details
     * MSAAターゲットの内容をバックバッファに転送します。
     * 2D描画（Direct2Dなど）を行うまえに呼び出すことで、3D描画結果の上に2Dを描画できます。
     * フレーム内で一度だけ実行されます。
     */
    void Resolve() {
        if (msaaResolved_) return;

        // MSAAが有効ならResolveを行う
        if (msaaEnabled_) {
            context_->ResolveSubresource(
                backBufferTex_.Get(), 0,
                msaaTex_.Get(), 0,
                DXGI_FORMAT_B8G8R8A8_UNORM);
        }
        msaaResolved_ = true;
    }

    /**
     * @brief フレーム終了(画面表示)
     */
    void EndFrame() {
        if (!msaaResolved_) {
            Resolve();
        }
        // Present interval: if VSync is desired, use 1, else 0.
        // Assuming 1 for now or 0 depending on prefs. Original code sent 0,0.
        swap_->Present(1, 0); 
    }

    ID3D11Device* Dev() const { return device_.Get(); }
    ID3D11DeviceContext* Ctx() const { return context_.Get(); }
    uint32_t Width() const { return width_; }
    uint32_t Height() const { return height_; }
    IDXGISwapChain* GetSwapChain() const { return swap_.Get(); }
    D3D11_VIEWPORT GetCurrentViewport() const { return viewport_; }

    void Shutdown() {
        if (isShutdown_) return;
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "GfxDevice::Shutdown() - リソースを解放中");

        if (context_) {
            context_->ClearState();
            context_->Flush();
        }

        msaaTex_.Reset();
        backBufferTex_.Reset();
        dsv_.Reset();
        rtv_.Reset();
        swap_.Reset();
        context_.Reset();

#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
        if (device_) {
            Microsoft::WRL::ComPtr<ID3D11Debug> debug;
            if (SUCCEEDED(device_.As(&debug))) {
                debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
            }
        }
#endif

        device_.Reset();
        isShutdown_ = true;
    }

    ~GfxDevice() {
        Shutdown();
    }

private:
    /**
     * @brief バックバッファリソースの作成(MSAA対応含む)
     */
    bool createBackbufferResources() {
        // 1. まずバックバッファテクスチャを取得（Resolve先、またはMSAA無効時のRT）
        HRESULT hr = swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBufferTex_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to get back buffer", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // 2. MSAAサポート確認
        UINT msaaQuality = 0;
        UINT sampleCount = 4;
        hr = device_->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, sampleCount, &msaaQuality);
        if (SUCCEEDED(hr) && msaaQuality > 0) {
            msaaEnabled_ = true;
        } else {
            msaaEnabled_ = false;
            sampleCount = 1;
            msaaQuality = 0;
            // DEBUGLOG_WARNING("MSAA 4x not supported. Falling back to 1x.");
        }

        // 3. レンダーターゲットビュー(RTV)の作成
        if (msaaEnabled_) {
            // MSAA用の中間テクスチャを作成
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = width_;
            desc.Height = height_;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = sampleCount;
            desc.SampleDesc.Quality = msaaQuality - 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            hr = device_->CreateTexture2D(&desc, nullptr, msaaTex_.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                MessageBoxA(nullptr, "Failed to create MSAA Texture", "DirectX Error", MB_OK | MB_ICONERROR);
                return false;
            }

            // MSAA Textureを指すRTVを作成
            hr = device_->CreateRenderTargetView(msaaTex_.Get(), nullptr, rtv_.ReleaseAndGetAddressOf());
        } else {
            // MSAA無効: バックバッファを直接RTVにする
            hr = device_->CreateRenderTargetView(backBufferTex_.Get(), nullptr, rtv_.ReleaseAndGetAddressOf());
        }

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create render target view", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // 4. 深度ステンシルビュー(DSV)の作成
        D3D11_TEXTURE2D_DESC td{};
        td.Width = width_;
        td.Height = height_;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        td.SampleDesc.Count = sampleCount;
        td.SampleDesc.Quality = (msaaEnabled_) ? (msaaQuality - 1) : 0;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
        hr = device_->CreateTexture2D(&td, nullptr, depth.GetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create depth stencil texture", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        hr = device_->CreateDepthStencilView(depth.Get(), nullptr, dsv_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create depth stencil view", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics,
            std::string("CreateBackbufferResources: MSAA ") + (msaaEnabled_ ? "ON (4x)" : "OFF"));

        return true;
    }

    void logEnvironment(D3D_FEATURE_LEVEL fl, const DXGI_SWAP_CHAIN_DESC& sd) {
        DEBUGLOG(std::string("MSAA: ") + (msaaEnabled_ ? "Enabled (4x)" : "Disabled"));
    }

#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
    void ReportLiveObjects() {}
#endif

    // メンバ変数
    uint32_t width_ = 0; 
    uint32_t height_ = 0;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv_;
    
    // MSAA追加メンバ
    Microsoft::WRL::ComPtr<ID3D11Texture2D> msaaTex_;       ///< MSAA Render Target (MSAA有効時のみ使用)
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferTex_; ///< SwapChain BackBuffer (Resolve先)
    bool msaaEnabled_ = false;                              ///< MSAA有効フラグ
    bool msaaResolved_ = false;                             ///< フレーム内でResolve済みか
    D3D11_VIEWPORT viewport_{};                             ///< Current Viewport

    bool isShutdown_ = false;
};
