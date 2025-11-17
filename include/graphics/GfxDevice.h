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
     * @return bool 初期化が成功した場合は true
     *
     * @details
     * DirectX11デバイス、スワップチェイン、レンダーターゲット、
     * 深度バッファを作成します。
     * デバッグビルドではデバッグレイヤーが有効になります。
     */
    bool Init(HWND hwnd, uint32_t w, uint32_t h) {
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
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // FLIP_DISCARDに変更（推奨モデル）

        UINT flags = 0;
#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Direct2D1.1との互換用
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
            // エラーの詳細をログ出力
            char errorMsg[256];
            sprintf_s(errorMsg,
                "Failed to create D3D11 device.\nHRESULT: 0x%08X\n"
                "Please check:\n"
                "- DirectX 11 is installed\n"
                "- Graphics drivers are up to date",
                hr);
            MessageBoxA(nullptr, errorMsg, "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        bool ok = createBackbufferResources();

        // 追加: アダプタ/機能レベル/フォーマット/SwapEffect/VSYNC情報をログ
        logEnvironment(fl, sd);
        return ok;
    }

    /**
     * @brief フレーム開始(画面クリア)
     * @param[in] r 赤成分(デフォルト: 0.1f)
     * @param[in] g 緑成分(デフォルト: 0.1f)
     * @param[in] b 青成分(デフォルト: 0.12f)
     * @param[in] a アルファ成分(デフォルト: 1.0f)
     *
     * @details
     * レンダーターゲットと深度バッファをクリアし、ビューポートを設定します。
     * すべての描画処理の前に呼び出してください。
     */
    void BeginFrame(float r = 0.1f, float g = 0.1f, float b = 0.12f, float a = 1.0f) {
        float c[4] = { r, g, b, a };
        context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), dsv_.Get());
        context_->ClearRenderTargetView(rtv_.Get(), c);
        context_->ClearDepthStencilView(dsv_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<FLOAT>(width_);
        vp.Height = static_cast<FLOAT>(height_);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        context_->RSSetViewports(1, &vp);
    }

    /**
     * @brief フレーム終了(画面表示)
     *
     * @details
     * バックバッファをフロントバッファに切り替え、画面に表示します。
     * すべての描画処理の後に呼び出してください。
     * 垂直同期(VSync)が有効です。
     */
    void EndFrame() {
        swap_->Present(1, 0);
    }

    /**
     * @brief デバイスアクセス
     * @return ID3D11Device* デバイスポインタ
     *
     * @details
     * リソース(テクスチャ、バッファなど)を作成する際に使用します。
     */
    ID3D11Device* Dev() const { return device_.Get(); }

    /**
     * @brief デバイスコンテキストアクセス
     * @return ID3D11DeviceContext* デバイスコンテキストのポインタ
     *
     * @details
     * 描画コマンドの発行やリソースの設定に使用します。
     */
    ID3D11DeviceContext* Ctx() const { return context_.Get(); }

    /**
     * @brief 幅を取得
     * @return uint32_t 幅(ピクセル単位)
     */
    uint32_t Width() const { return width_; }

    /**
     * @brief 高さを取得
     * @return uint32_t 高さ(ピクセル単位)
     */
    uint32_t Height() const { return height_; }

    /**
     * @brief スワップチェインへのアクセス
     * @return IDXGISwapChain* スワップチェインのポインタ
     *
     * @details
     * TextSystemなど、スワップチェインへの直接アクセスが必要な場合に使用します。
     */
    IDXGISwapChain* GetSwapChain() const { return swap_.Get(); }

    /**
     * @brief リソースの明示的解放
     *
     * @details
     * DirectX11リソースを明示的に解放します。
     * デストラクタからも呼ばれますが、順序制御のため明示的に呼び出すことを推奨します。
     */
    void Shutdown() {
        if (isShutdown_) return; // 冪等性
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "GfxDevice::Shutdown() - リソースを解放中");

        // P2: コンテキストの状態をクリアしてFlush
        if (context_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "ID3D11DeviceContext::ClearState() を呼び出し");
            context_->ClearState();

            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "ID3D11DeviceContext::Flush() を呼び出し");
            context_->Flush();
        }

        // リソース解放カウンタ
        int releasedCount = 0;

        // Live Objects用に参照カウントを収集
        long dsvRefForReport = -1;
        long rtvRefForReport = -1;
        long swapRefForReport = -1;
        long ctxRefForReport = -1;
        long devRefForReport = -1;

        if (dsv_) {
            ULONG refCount = dsv_.Get()->AddRef() - 1;
            dsv_.Get()->Release();
            dsvRefForReport = static_cast<long>(refCount);
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "深度ステンシルビューを解放 (ULONG RefCount: " + std::to_string(refCount) + ")");
            dsv_.Reset();
            releasedCount++;
        }

        if (rtv_) {
            ULONG refCount = rtv_.Get()->AddRef() - 1;
            rtv_.Get()->Release();
            rtvRefForReport = static_cast<long>(refCount);
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "レンダーターゲットビューを解放 (ULONG RefCount: " + std::to_string(refCount) + ")");
            rtv_.Reset();
            releasedCount++;
        }

        if (swap_) {
            ULONG refCount = swap_.Get()->AddRef() - 1;
            swap_.Get()->Release();
            swapRefForReport = static_cast<long>(refCount);
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "スワップチェインを解放 (ULONG RefCount: " + std::to_string(refCount) + ")");
            swap_.Reset();
            releasedCount++;
        }

        if (context_) {
            ULONG refCount = context_.Get()->AddRef() - 1;
            context_.Get()->Release();
            ctxRefForReport = static_cast<long>(refCount);
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "デバイスコンテキストを解放 (ULONG RefCount: " + std::to_string(refCount) + ")");
            context_.Reset();
            releasedCount++;
        }

#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
        // デバッグビルド: VS出力を使わず、アプリのログに参照カウント要約を出力
        if (device_) {
            Microsoft::WRL::ComPtr<ID3D11Debug> debug;
            if (SUCCEEDED(device_.As(&debug))) {
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "========================================");
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "D3D11デバッグレイヤー: Live Objectsレポート(アプリログ)開始");
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "========================================");

                // Visual Studio出力は呼ばない
                // 参照カウント測定前に debug を明示的に解放して自己参照を除去
                debug.Reset();

                // デバイスの参照カウントをチェック（測定専用）
                ULONG deviceRefCount = device_.Get()->AddRef() - 1;
                device_.Get()->Release();
                devRefForReport = static_cast<long>(deviceRefCount);

                // ログに要約を出力
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "--- Live Objects要約 (参照カウント) ---");
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DSV RefCount: " + std::to_string(dsvRefForReport));
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "RTV RefCount: " + std::to_string(rtvRefForReport));
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "SwapChain RefCount: " + std::to_string(swapRefForReport));
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DeviceContext RefCount: " + std::to_string(ctxRefForReport));
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Device RefCount: " + std::to_string(devRefForReport));
                DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "========================================");

                if (deviceRefCount > 1) {
                    DEBUGLOG_WARNING("デバイスの参照カウントが 1 より大きい: " + std::to_string(deviceRefCount) + " (リーク可能性)");
                } else {
                    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "デバイスの参照カウント: " + std::to_string(deviceRefCount) + " (正常)");
                }
            } else {
                DEBUGLOG_WARNING("D3D11デバッグレイヤーが利用できません (デバッグフラグで作成されていない可能性)");
            }
        }
#endif

        if (device_) {
            ULONG refCount = device_.Get()->AddRef() - 1;
            device_.Get()->Release();
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "デバイスを解放 (ULONG RefCount: " + std::to_string(refCount) + ")");
            device_.Reset();
            releasedCount++;
        }

        isShutdown_ = true;
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "GfxDevice::Shutdown() 完了 (解放リソース数: " + std::to_string(releasedCount) + ")");
    }

    /**
     * @brief デストラクタでリソースを明示的に解放
     *
     * @details
     * ComPtrは自動で解放されますが、念のため明示的にリセットします。
     */
    ~GfxDevice() {
        DEBUGLOG("GfxDevice::~GfxDevice() - デストラクタ呼び出し");
        Shutdown();
    }

private:
    /**
     * @brief バックバッファリソースの作成
     * @return bool 作成が成功した場合は true
     *
     * @details
     * スワップチェインからバックバッファを取得し、
     * レンダーターゲットビューと深度ステンシルビューを作成します。
     */
    bool createBackbufferResources() {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> back;
        HRESULT hr = swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)back.GetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to get back buffer", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        hr = device_->CreateRenderTargetView(back.Get(), nullptr, rtv_.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to create render target view", "DirectX Error", MB_OK | MB_ICONERROR);
            return false;
        }

        D3D11_TEXTURE2D_DESC td{};
        td.Width = width_;
        td.Height = height_;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        td.SampleDesc.Count = 1;
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

        return true;
    }

    /**
     * @brief 環境メトリクスのログ出力
     * @param fl 機能レベル
     * @param sd スワップチェインの設定
     *
     * @details
     * 初期化時に取得したアダプタ名、機能レベル、スワップ効果、バックバッファフォーマット、
     * VSync設定などの情報をログに出力します。
     */
    void logEnvironment(D3D_FEATURE_LEVEL fl, const DXGI_SWAP_CHAIN_DESC& sd) {
        // アダプタ名取得
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        if (SUCCEEDED(device_.As(&dxgiDevice))) {
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            if (SUCCEEDED(dxgiDevice->GetAdapter(adapter.GetAddressOf()))) {
                DXGI_ADAPTER_DESC desc{};
                if (SUCCEEDED(adapter->GetDesc(&desc))) {
                    char name[256];
                    size_t outSize = 0;
                    wcstombs_s(&outSize, name, desc.Description, 255);
                    DEBUGLOG(std::string("アダプタ: ") + name);
                }
            }
        }

        // Feature Level
        const char* flText = "Unknown";
        switch (fl) {
            case D3D_FEATURE_LEVEL_11_1: flText = "11.1"; break;
            case D3D_FEATURE_LEVEL_11_0: flText = "11.0"; break;
            case D3D_FEATURE_LEVEL_10_1: flText = "10.1"; break;
            case D3D_FEATURE_LEVEL_10_0: flText = "10.0"; break;
            default: break;
        }
        DEBUGLOG(std::string("機能レベル: ") + flText);

        // スワップチェイン情報
        const char* swapEffectText = "Unknown";
        switch (sd.SwapEffect) {
            case DXGI_SWAP_EFFECT_DISCARD: swapEffectText = "DISCARD (レガシー)"; break;
            case DXGI_SWAP_EFFECT_SEQUENTIAL: swapEffectText = "SEQUENTIAL (レガシー)"; break;
            case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL: swapEffectText = "FLIP_SEQUENTIAL"; break;
            case DXGI_SWAP_EFFECT_FLIP_DISCARD: swapEffectText = "FLIP_DISCARD (推奨)"; break;
            default: break;
        }
        DEBUGLOG(std::string("スワップ効果: ") + swapEffectText);
        DEBUGLOG(std::string("バックバッファフォーマット: BGRA8_UNORM (Direct2D互換)"));
        DEBUGLOG(std::string("垂直同期: ON (Present(1)) - ディスプレイのリフレッシュレートに同期"));
    }

#if defined(ENABLE_GFX_DEBUG_LAYER) && ENABLE_GFX_DEBUG_LAYER
    /**
     * @brief 旧仕様の互換ダミー
     */
    void ReportLiveObjects() {}
#endif

    // メンバ変数
    uint32_t width_ = 0;  ///< 画面幅
    uint32_t height_ = 0; ///< 画面高さ
    Microsoft::WRL::ComPtr<ID3D11Device> device_;           ///< D3D11デバイス
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;   ///< D3D11デバイスコンテキスト
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_;           ///< スワップチェイン
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;    ///< レンダーターゲットビュー
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv_;    ///< 深度ステンシルビュー
    bool isShutdown_ = false; ///< シャットダウン済みフラグ
};
