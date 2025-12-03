/**
 * @file UIImageComponents.h
 * @brief UI画像コンポーネント
 */
#pragma once
#include "components/Component.h"
#include <string>
#include <DirectXMath.h>

struct UIImage : IComponent {
    std::wstring filePath;           ///< 画像ファイルパス
    float opacity = 1.0f;            ///< 透過(0..1)
    bool keepAspect = true;          ///< アスペクト比維持
    DirectX::XMFLOAT4 tint{1,1,1,1}; ///< 将来のティント用(未使用)

    UIImage() = default;
    explicit UIImage(const std::wstring &path) : filePath(path) {}
};
