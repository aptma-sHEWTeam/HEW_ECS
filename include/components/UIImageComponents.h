/**
 * @file UIImageComponents.h
 * @brief UI画像コンポーネント
 */
#pragma once
#include "components/Component.h"
#include <string>
#include <array>
#include <DirectXMath.h>
#include "graphics/TextureManager.h"

struct UIImage : IComponent {
    std::wstring filePath;           ///< 画像ファイルパス
    float opacity = 1.0f;            ///< 透過(0..1)
    bool keepAspect = true;          ///< アスペクト維持
    bool aspectFill = false;         ///< アスペクト維持(Fill)
    bool aspectAlignLeft = false;    ///< keepAspect時に左揃え
    DirectX::XMFLOAT4 tint{1,1,1,1}; ///< 画像へのカラー乗算

    std::array<float, 4> uvRect = {0, 0, 1, 1};

    // 追加: TextureManagerハンドルでの描画に対応（filePathより優先）
    TextureManager::TextureHandle textureHandle = TextureManager::INVALID_TEXTURE;

    // 追加: フェード等のオーバーレイ画像を上層に描画するためのフラグ
    bool overlay = false;

    UIImage() = default;
    explicit UIImage(const std::wstring &path) : filePath(path) {}
};
