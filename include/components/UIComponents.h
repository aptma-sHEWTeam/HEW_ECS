/**
 * @file UIComponents.h
 * @brief UIコンポーネントの定義
 * @author 山内陽
 * @date 2025
 * @version 1.0
 *
 * @details
 * UIボタン、テキストラベル、パネルなどの2D UI要素を
 * ECSアーキテクチャで実装します。
 */
#pragma once
#include "components/Component.h"
#include "components/Transform.h"
#include <DirectXMath.h>
#include <string>
#include <functional>
#include "config/ConfigVar.h"

/**
 * @struct UITransform
 * @brief 2D UI要素の位置とサイズ
 *
 * @details
 * スクリーン座標系での位置とサイズを保持します。
 * アンカーシステムにより、画面解像度の変化に対応できます。
 */
struct UITransform : IComponent {
    DirectX::XMFLOAT2 position{0.0f, 0.0f}; ///< スクリーン座標(ピクセル)
    DirectX::XMFLOAT2 size{100.0f, 50.0f};  ///< サイズ(幅, 高さ)
    DirectX::XMFLOAT2 anchor{0.0f, 0.0f};   ///< アンカー(0-1, 左上=0,0 右下=1,1)
    DirectX::XMFLOAT2 pivot{0.5f, 0.5f};    ///< ピボット(0-1, 中心=0.5,0.5)
    float rotation = 0.0f;                  ///< 回転(度)

    /**
     * @brief アンカーを考慮したスクリーン座標を取得
     * @param[in] screenWidth 画面幅
     * @param[in] screenHeight 画面高さ
   * @return DirectX::XMFLOAT2 実際のスクリーン座標
     */
    DirectX::XMFLOAT2 GetScreenPosition(float screenWidth, float screenHeight) const {
        float anchorX = screenWidth * anchor.x;
        float anchorY = screenHeight * anchor.y;
        float pivotOffsetX = size.x * pivot.x;
        float pivotOffsetY = size.y * pivot.y;
        return DirectX::XMFLOAT2{
            anchorX + position.x - pivotOffsetX,
            anchorY + position.y - pivotOffsetY};
    }

    /**
     * @brief 点がUI要素の範囲内にあるかを判定
     * @param[in] x X座標
     * @param[in] y Y座標
     * @param[in] screenWidth 画面幅
     * @param[in] screenHeight 画面高さ
     * @return bool 範囲内の場合はtrue
     */
    bool Contains(float x, float y, float screenWidth, float screenHeight) const {
        DirectX::XMFLOAT2 screenPos = GetScreenPosition(screenWidth, screenHeight);
        return x >= screenPos.x && x <= screenPos.x + size.x &&
               y >= screenPos.y && y <= screenPos.y + size.y;
    }
};

/**
 * @struct UIText
 * @brief テキストラベルコンポーネント
 */
struct UIText : IComponent {
    std::wstring text = L"Label";                           ///< 表示するテキスト
    DirectX::XMFLOAT4 color{1.0f, 1.0f, 1.0f, 1.0f};        ///< テキストの色(RGBA)
    std::string formatId = "default";                       ///< TextSystemのフォーマットID
    float fontSize = 0.0f;                                  ///< フォントサイズ上書き(>0 で適用)
    DirectX::XMFLOAT4 outlineColor{1.0f, 1.0f, 1.0f, 1.0f}; ///< アウトライン色(RGBA)
    float outlineThickness = 0.0f;                          ///< アウトライン太さ(0で無効)
    std::wstring fillTexturePath;                           ///< 文字塗りつぶし用テクスチャ

    explicit UIText(const std::wstring &txt = L"Label")
        : text(txt) {}
};

/**
 * @struct UIButton
 * @brief ボタンコンポーネント
 *
 * @details
 * クリック可能なボタンUIを実装します。
 * 状態によって色が変化します。
 */
struct UIButton : IComponent {
    enum class State {
        Normal,  ///< 通常状態
        Hovered, ///< マウスオーバー状態
        Pressed, ///< 押下状態
        Disabled ///< 無効状態
    };

    State state = State::Normal; ///< 現在の状態
    bool enabled = true;         ///< 有効/無効

    DirectX::XMFLOAT4 normalColor;
    DirectX::XMFLOAT4 hoverColor;
    DirectX::XMFLOAT4 pressedColor;
    DirectX::XMFLOAT4 disabledColor;

    std::function<void()> onClick; ///< クリック時のコールバック

    // Configs
    inline static ConfigVar<float> cfg_BtnNormalR{"UI.Button.Normal", "BtnNormalR", 0.2f, "ボタン通常時の色 R"};
    inline static ConfigVar<float> cfg_BtnNormalG{"UI.Button.Normal", "BtnNormalG", 0.2f, "ボタン通常時の色 G"};
    inline static ConfigVar<float> cfg_BtnNormalB{"UI.Button.Normal", "BtnNormalB", 0.2f, "ボタン通常時の色 B"};

    inline static ConfigVar<float> cfg_BtnHoverR{"UI.Button.Hover", "BtnHoverR", 0.3f, "ボタンホバー時の色 R"};
    inline static ConfigVar<float> cfg_BtnHoverG{"UI.Button.Hover", "BtnHoverG", 0.3f, "ボタンホバー時の色 G"};
    inline static ConfigVar<float> cfg_BtnHoverB{"UI.Button.Hover", "BtnHoverB", 0.3f, "ボタンホバー時の色 B"};

    inline static ConfigVar<float> cfg_BtnPressR{"UI.Button.Press", "BtnPressR", 0.15f, "ボタン押下時の色 R"};
    inline static ConfigVar<float> cfg_BtnPressG{"UI.Button.Press", "BtnPressG", 0.15f, "ボタン押下時の色 G"};
    inline static ConfigVar<float> cfg_BtnPressB{"UI.Button.Press", "BtnPressB", 0.15f, "ボタン押下時の色 B"};

    UIButton() {
        normalColor = {cfg_BtnNormalR, cfg_BtnNormalG, cfg_BtnNormalB, 1.0f};
        hoverColor = {cfg_BtnHoverR, cfg_BtnHoverG, cfg_BtnHoverB, 1.0f};
        pressedColor = {cfg_BtnPressR, cfg_BtnPressG, cfg_BtnPressB, 1.0f};
        disabledColor = {0.1f, 0.1f, 0.1f, 0.5f};
    }

    /**
     * @brief 現在の状態に応じた色を取得
     * @return DirectX::XMFLOAT4 現在の色
     */
    DirectX::XMFLOAT4 GetCurrentColor() const {
        if (!enabled)
            return disabledColor;
        switch (state) {
            case State::Hovered:
                return hoverColor;
            case State::Pressed:
                return pressedColor;
            default:
                return normalColor;
        }
    }
};

/**
 * @struct UIPanel
 * @brief 単色パネルコンポーネント
 */
struct UIPanel : IComponent {
    DirectX::XMFLOAT4 color{0.1f, 0.1f, 0.1f, 0.8f}; ///< パネルの色(RGBA)
    bool visible = true;                             ///< 表示/非表示
    bool drawBeforeImages = false;

    explicit UIPanel(const DirectX::XMFLOAT4 &col = {0.1f, 0.1f, 0.1f, 0.8f})
        : color(col) {}
};

/**
 * @struct UICanvas
 * @brief UIのルートキャンバス
 *
 * @details
 * すべてのUI要素はこのキャンバスの子要素として扱われます。
 */
struct UICanvas : IComponent {
    bool enabled = true; ///< キャンバス全体の有効/無効
    int sortOrder = 0;   ///< 描画順序(大きいほど手前)
};
