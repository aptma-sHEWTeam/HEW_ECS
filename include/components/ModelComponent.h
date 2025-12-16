#pragma once
#include <DirectXMath.h>
#include "graphics/TextureManager.h"
#include <wrl/client.h>
#include <d3d11.h>

/**
 * @file ModelComponent.h
 * @brief 3Dモデルのメッシュデータを保持するコンポーネントの定義
 * @author 山内陽
 * @date 2025
 * @version 6.0
 * 
 * @details
 * このファイルは、Assimpによってロードされた3Dモデルの個々のメッシュの
 * 頂点データ、インデックスデータ、およびマテリアル情報を保持する
 * ModelComponentを定義します。
 */

struct ModelComponent {
    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    // インデックスバッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    // インデックス数
    UINT indexCount = 0;
    // テクスチャハンドル (現時点では単一テクスチャを想定)
    TextureManager::TextureHandle texture = TextureManager::INVALID_TEXTURE;
    TextureManager::TextureHandle normalTexture = TextureManager::INVALID_TEXTURE;
    // 基本色 (テクスチャがない場合、または色調補正用)
    DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
    // スペキュラー設定 (デフォルトは無効になるよう0)
    float specularAttenuation = 0.0f;                  // 0以下でスペキュラー無効
    DirectX::XMFLOAT3 specularColor{ 1.0f, 1.0f, 1.0f }; // ハイライト色
    float reflectance = 0.0f;                          // Fresnel F0 相当(0-1)
    DirectX::XMFLOAT3 reflectionColor{ 1.0f, 1.0f, 1.0f }; // 反射カラー乗算
    float specularEccentricity = 0.0f;                  // -1〜1 目安。0で等方。
    float useLighting = 1.0f;                           // 0でアンリット
    // UVオフセットとスケール (将来的に必要に応じて拡張)
    DirectX::XMFLOAT2 uvOffset{ 0.0f, 0.0f };
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };

    // スキニングアニメーション用データ
    struct Bone {
        std::string name;
        DirectX::XMFLOAT4X4 offsetMatrix; // メッシュ空間からボーン空間への変換
        int parentIndex = -1;
    };

    struct Skeleton {
        std::vector<Bone> bones;
        std::vector<DirectX::XMFLOAT4X4> boneTransforms; // 現在のボーン変換行列 (アニメーション適用後)
    };

    Skeleton skeleton;
    bool isSkinned = false; // スキニングが有効かどうか
    
    // アニメーションデータ
    struct Keyframe {
        float time;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 rotation; // Quaternion
        DirectX::XMFLOAT3 scale;
    };

    struct BoneAnimation {
        std::string boneName;
        int boneIndex;
        std::vector<Keyframe> keyframes;
        bool hasPositionKeys = false;
        bool hasRotationKeys = false;
        bool hasScaleKeys = false;
    };

    struct AnimationClip {
        std::string name;
        float duration;
        float ticksPerSecond;
        std::vector<BoneAnimation> boneAnimations;
    };
    
    // このモデルが持つアニメーションのリスト (別ファイルからロードする場合もある)
    std::vector<AnimationClip> animations;

    DirectX::XMFLOAT4X4 globalInverse = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};
