/**
 * @file ImageSystem.h
 * @brief Direct2D + WIC 画像描画システム
 */
#pragma once
#include "graphics/GfxDevice.h"
#include "app/DebugLog.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
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
        float opacity = 1.0f;   ///< 透過(0..1)
        bool keepAspect = true; ///< アスペクト比維持
    };

    bool Init(GfxDevice &gfx);
    void BeginDraw();
    void EndDraw();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    bool Draw(const Params &p);

private:
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;

    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap1>> bitmapCache_;

    bool initialized_ = false;
    GfxDevice *gfx_ = nullptr;

    void RefreshTargetBitmap();
    bool LoadBitmap(const std::wstring &filePath, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &out);
};
