/**
 * @file TextSystem.h
 * @brief DirectWriteを使用した文字描画システム
 * @author 山内陽
 * @date 2025
 * @version 1.0
 *
 * @details
 * Direct2DとDirectWriteを使用してテキストを描画するシステムです。
 * 3DシーンとシームレスにCasadeするため、同じスワップチェインを使用します。
 */
#pragma once
#include "graphics/GfxDevice.h"
#include "app/DebugLog.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

/**
 * @class TextSystem
 * @brief テキスト描画システム
 *
 * @details
 * DirectWriteとDirect2Dを使用して2Dテキストを描画します。
 * 3DシーンとシームレスにCasadeするため、同じスワップチェインを使用します。
 *
 * @par 使用例
 * @code
 * TextSystem textSystem;
 * textSystem.Init(gfx);
 *
 * TextSystem::TextFormat titleFormat;
 * titleFormat.fontSize = 48.0f;
 * titleFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
 * textSystem.CreateTextFormat("title", titleFormat);
 *
 * textSystem.BeginDraw();
 *
 * TextSystem::TextParams params;
 * params.text = L"Hello World";
 * params.x = 100.0f;
 * params.y = 50.0f;
 * params.formatId = "title";
 * textSystem.DrawText(params);
 *
 * textSystem.EndDraw();
 * @endcode
 */
class TextSystem {
  public:
    /**
  * @struct TextFormat
     * @brief テキストフォーマット設定
     */
    struct TextFormat {
        std::wstring fontFamily = L"Mamelon-5-Hi-Regular.otf";                           ///< フォントファミリー
        float fontSize = 24.0f;                                                          ///< フォントサイズ
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;                           ///< フォントの太さ
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;                              ///< フォントスタイル
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING;                 ///< 水平方向の配置
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR; ///< 垂直方向の配置
    };

    /**
     * @struct TextParams
* @brief テキスト描画パラメータ
     */
    struct TextParams {
        std::wstring text;                                      ///< 描画するテキスト
        float x = 0.0f;                                         ///< X座標
        float y = 0.0f;                                         ///< Y座標
        float width = 100.0f;                                   ///< 描画領域の幅
        float height = 50.0f;                                   ///< 描画領域の高さ
        float fontSize = 0.0f;                                  ///< フォントサイズの上書き(>0 の場合に適用)
        DirectX::XMFLOAT4 color{1.0f, 1.0f, 1.0f, 1.0f};        ///< テキストの色(RGBA)
        DirectX::XMFLOAT4 outlineColor{1.0f, 1.0f, 1.0f, 1.0f}; ///< アウトライン色(RGBA)
        float outlineThickness = 0.0f;                          ///< アウトライン太さ(0で無効)
        std::wstring fillTexturePath;                           ///< 文字塗りつぶし用テクスチャ
        std::string formatId = "default";                       ///< 使用するフォーマットID
    };

    // 画像描画用パラメータ
    struct ImageParams {
        float x = 0.0f;         ///< X座標
        float y = 0.0f;         ///< Y座標
        float width = 100.0f;   ///< 描画領域の幅
        float height = 100.0f;  ///< 描画領域の高さ
        float opacity = 1.0f;   ///< 透過(0..1)
        bool keepAspect = true; ///< アスペクト比維持
        std::wstring filePath;  ///< 画像ファイルパス
    };

    TextSystem() = default;
    ~TextSystem() {
        Shutdown();
    }

    TextSystem(const TextSystem &) = delete;
    TextSystem &operator=(const TextSystem &) = delete;

    /**
     * @brief 初期化
     * @param[in] gfx グラフィックスデバイス
     * @return bool 初期化が成功した場合はtrue
     */
    bool Init(GfxDevice &gfx);

    /**
     * @brief テキストフォーマットを作成
     * @param[in] id フォーマットID
   * @param[in] format フォーマット設定
     * @return bool 作成が成功した場合はtrue
     */
    bool CreateTextFormat(const std::string &id, const TextFormat &format);

    /**
     * @brief テキストを描画
     * @param[in] params 描画パラメータ
     *
     * @details
     * BeginDraw()とEndDraw()の間で呼び出す必要があります。
     */
    void DrawText(const TextParams &params);

    /**
     * @brief 矩形を塗りつぶし描画
     * @param[in] x 左上X座標
     * @param[in] y 左上Y座標
     * @param[in] width 幅
     * @param[in] height 高さ
     * @param[in] color 塗りつぶし色(RGBA)
     *
     * @details
     * BeginDraw()とEndDraw()の間で呼び出す必要があります。
     */
    void FillRect(float x, float y, float width, float height, const DirectX::XMFLOAT4 &color);

    // 画像描画API
    /**
     * @brief 画像を描画
     * @param[in] params 描画パラメータ
     *
     * @details
     * 透過、サイズ制御、オプションのアスペクト比維持をサポートします。
     */
    bool DrawImage(const ImageParams &params);

    /**
  * @brief 描画開始
     *
     * @details
     * すべてのテキスト描画の前に呼び出します。
     */
    void BeginDraw();

    /**
     * @brief 描画終了
     *
     * @details
     * すべてのテキスト描画の後に呼び出します。
     */
    void EndDraw();

    /**
     * @brief シャットダウン
     */
    void Shutdown();

    /**
     * @brief 初期化状態を確認
     * @return bool 初期化済みの場合はtrue
     */
    bool IsInitialized() const {
        return initialized_;
    }

  private:
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;

    std::unordered_map<std::string, Microsoft::WRL::ComPtr<IDWriteTextFormat>> textFormats_;
    std::unordered_map<uint32_t, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> brushCache_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap1>> bitmapCache_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1BitmapBrush>> bitmapBrushCache_;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> customFontCollection_;
    Microsoft::WRL::ComPtr<IDWriteFontCollectionLoader> customFontCollectionLoader_;
    std::vector<std::wstring> customFontFiles_;

    bool initialized_ = false;
    GfxDevice *gfx_ = nullptr;

    ID2D1SolidColorBrush *GetOrCreateBrush(const DirectX::XMFLOAT4 &color);
    ID2D1BitmapBrush *GetOrCreateBitmapBrush(const std::wstring &filePath);
    uint32_t ColorToHash(const DirectX::XMFLOAT4 &color) const;
    void RefreshTargetBitmap();
    bool InitCustomFontCollection();
    bool TryResolveCustomFontFamily(const std::wstring &requestedFamily, std::wstring &outFamily) const;

    // 画像読み込み（キャッシュ利用）
    bool LoadBitmapFromFile(const std::wstring &filePath, Microsoft::WRL::ComPtr<ID2D1Bitmap1> &outBitmap);
};
