/**
 * @file RenderSystem.h
 * @brief 3Dレンダリングシステム
 * @author 山内陽
 * @date 2025
 * @version 7.0
 *
 * @details
 * DirectX11を使用した3Dレンダリングシステムです。
 * ModelComponentとMeshRendererの両方をサポートします。
 */
#pragma once
#include "graphics/GfxDevice.h"
#include "graphics/Camera.h"
#include "ecs/World.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/ModelComponent.h"
#include "components/Light.h"
#include "components/TransformHierarchy.h"
#include "graphics/TextureManager.h"
#include "app/DebugLog.h"
#include "app/ServiceLocator.h"
#include "systems/RenderingSystem.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#pragma comment(lib, "d3dcompiler.lib")

/**
 * @struct RenderSystem
 * @brief 3Dレンダリングシステム
 *
 * @details
 * DirectX11を使用してModelComponentとMeshRendererを描画します。
 *
 * ### 主な機能:
 * - Blinn-Phongライティングモデル
 * - ノーマルマッピング対応
 * - テクスチャサポート
 * - 基本形状(Cube, Sphere, Cylinder, Plane)の描画
 *
 * @par 使用例
 * @code
 * RenderSystem renderer;
 * if (!renderer.Init()) {
 *  return false;
 * }
 *
 * // メインループ
 * renderer.Render(world, camera);
 *
 * // シャットダウン
 * renderer.Shutdown();
 * @endcode
 */
struct RenderSystem {
    /**
     * @struct VSConstants
     * @brief 頂点シェーダー用定数バッファ
     */
    struct VSConstants {
        DirectX::XMMATRIX World;       ///< ワールド行列
        DirectX::XMMATRIX WVP;         ///< ワールド・ビュー・プロジェクション行列
        DirectX::XMFLOAT4 uvTransform; ///< UVオフセットとスケール
    };

    /**
     * @struct PSConstants
     * @brief ピクセルシェーダー用オブジェクト定数バッファ
     */
    struct PSConstants {
        DirectX::XMFLOAT4 color;         ///< マテリアルカラー
        float useTexture;                ///< テクスチャ使用フラグ
        float useNormalMap;              ///< ノーマルマップ使用フラグ
        float useLighting;               ///< ライティング有効フラグ
        float specularAttenuation;       ///< スペキュラー減衰(0以下で無効)
        float specularEccentricity;      ///< 偏心度(-1〜1目安)
        DirectX::XMFLOAT3 specularColor; ///< ハイライト色
        float paddingA;                  ///< パディング
        DirectX::XMFLOAT3 reflectionColor; ///< 反射カラー
        float reflectance;               ///< 反射率(F0相当)
        float paddingEnd[3] = {0.0f, 0.0f, 0.0f}; ///< 16バイト境界合わせ
        DirectX::XMFLOAT4 chromaticParams; ///< x:強度 y:オフセット z:半径倍率
        DirectX::XMFLOAT4 chromaticScreen; ///< x:幅 y:高さ
        DirectX::XMFLOAT4 stylizeParams0; ///< x:ビネット y:ノイズ z:フォグ w:中心保護半径
        DirectX::XMFLOAT4 stylizeParams1; ///< x:色温度 y:速度線 z:方向x w:方向y
        DirectX::XMFLOAT4 stylizeParams2; ///< x:フラッシュ y:リップル z:時間 w:予約
    };

    /**
     * @struct PSLightConstants
     * @brief ピクセルシェーダー用ライト定数バッファ
     */
    struct PSLightConstants {
        DirectionalLight light;                           ///< ディレクショナルライト
        DirectX::XMFLOAT3 ambientColor{0.2f, 0.2f, 0.2f}; ///< アンビエントカラー
        float padding2;                                   ///< パディング
        DirectX::XMFLOAT3 eyePos;                         ///< カメラ位置
        float padding3;                                   ///< パディング
    };

    /**
     * @struct Statistics
     * @brief レンダリング統計情報
     */
    struct Statistics {
        size_t modelsRendered = 0; ///< 描画されたModelComponentの数
        size_t meshesRendered = 0; ///< 描画されたMeshRendererの数
        size_t totalDrawCalls = 0; ///< 総描画コール数

        void Reset() {
            modelsRendered = 0;
            meshesRendered = 0;
            totalDrawCalls = 0;
        }
    };

    /**
   * @brief デストラクタ
     */
    ~RenderSystem() {
        Shutdown();
    }

    /**
     * @brief コピー禁止
     */
    RenderSystem(const RenderSystem &) = delete;
    RenderSystem &operator=(const RenderSystem &) = delete;

    /**
     * @brief ムーブ許可
     */
    RenderSystem(RenderSystem &&) noexcept = default;
    RenderSystem &operator=(RenderSystem &&) noexcept = default;

    /**
     * @brief デフォルトコンストラクタ
     */
    RenderSystem() = default;

    /**
     * @brief 初期化
     * @return bool 初期化が成功した場合は true
  *
     * @details
     * シェーダーのコンパイル、パイプラインステートの作成、
     * 基本形状メッシュの生成を行います。
     */
    bool Init() {
        if (initialized_) {
            DEBUGLOG_WARNING("[RenderSystem] 既に初期化されています");
            return true;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] 初期化開始");

        auto &gfx = ServiceLocator::Get<GfxDevice>();

        if (!RenderingSystem::GetInstance().Initialize(gfx.Dev())) {
            DEBUGLOG_ERROR("[RenderSystem] RenderingSystem の初期化に失敗");
            return false;
        }

        if (!CompileShaders(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] シェーダーのコンパイルに失敗");
            return false;
        }

        if (!CreateInputLayout(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 入力レイアウトの作成に失敗");
            return false;
        }

        if (!CreateConstantBuffers(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 定数バッファの作成に失敗");
            return false;
        }

        if (!CreateStates(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] ステートの作成に失敗");
            return false;
        }

        if (!CreatePrimitiveMeshes(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 基本形状メッシュの作成に失敗");
            return false;
        }

        if (!CreateDummyShadowMap(gfx)) {
             DEBUGLOG_WARNING("[RenderSystem] ダミーシャドウマップの作成に失敗");
        }

        initialized_ = true;
        stats_.Reset();

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] 初期化完了");
        return true;
    }

    /**
     * @brief レンダリング
     * @param[in] w ワールド
     * @param[in] cam カメラ
     *
     * @details
     * すべてのModelComponentとMeshRendererを描画します。
     */
    void Render(World &w, const Camera &cam) {
        if (!initialized_) {
            DEBUGLOG_WARNING("[RenderSystem] 初期化されていません");
            return;
        }

        auto &gfx = ServiceLocator::Get<GfxDevice>();
        auto &texMgr = ServiceLocator::Get<TextureManager>();

        stats_.Reset();

        // パイプラインステートの設定
        SetupPipeline(gfx);

        // ライト情報の更新（統合レンダリングシステムに委譲）
        RenderingSystem::GetInstance().UpdateLights(w, cam.position);
        RenderingSystem::GetInstance().BindLightBuffer(gfx.Ctx(), 1);
        RenderingSystem::GetInstance().BindMaterialBuffer(gfx.Ctx(), 2);

        ID3D11ShaderResourceView* shadowSRV = shadowMapSRV_;
        if (!shadowSRV || !IsValidSRV(shadowSRV)) {
            shadowSRV = dummyShadowSRV_.Get();
        }
        if (shadowSRV) {
            gfx.Ctx()->PSSetShaderResources(2, 1, &shadowSRV);
        }

        // ModelComponentの描画
        RenderModelComponents(w, gfx, cam, texMgr);

        // MeshRendererの描画
        RenderMeshRenderers(w, gfx, cam, texMgr);
    }

    /**
     * @brief シャットダウン
     *
 * @details
     * すべてのリソースを解放します。
     */
    void Shutdown() {
        if (!initialized_)
            return;

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] シャットダウン開始");

        RenderingSystem::GetInstance().Shutdown();

        // 統計情報のログ出力
        if (stats_.totalDrawCalls > 0) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics,
                              "RenderSystem統計: Models=" + std::to_string(stats_.modelsRendered) +
                                  ", Meshes=" + std::to_string(stats_.meshesRendered) +
                                  ", DrawCalls=" + std::to_string(stats_.totalDrawCalls));
        }

        // リソース解放
        vs_.Reset();
        ps_.Reset();
        layout_.Reset();
        vsCb_.Reset();
        skinningCb_.Reset();
        psCb_.Reset();
        psLightCb_.Reset();
        rasterState_.Reset();
        samplerState_.Reset();
        samplerStateClamp_.Reset();
        shadowMapSRV_ = nullptr;
        dummyShadowTex_.Reset();
        dummyShadowSRV_.Reset();
        ClearScreenEffects();

        meshCache_.clear();

        initialized_ = false;

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] シャットダウン完了");
    }

    /**
     * @brief 統計情報の取得
     * @return const Statistics& 統計情報への参照
     */
    const Statistics &GetStatistics() const {
        return stats_;
    }

    /**
  * @brief 初期化状態の確認
     * @return bool 初期化済みの場合は true
  */
    bool IsInitialized() const {
        return initialized_;
    }

    /**
     * @brief ShadowRenderSystemなど、他のシステムからメッシュを描画するためのヘルパー
     */
    void DrawMesh(MeshType type) {
        auto it = meshCache_.find(static_cast<int>(type));
        if (it == meshCache_.end()) return;

        auto& mesh = it->second;
        auto& gfx = ServiceLocator::Get<GfxDevice>();
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        gfx.Ctx()->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(), &stride, &offset);
        gfx.Ctx()->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        gfx.Ctx()->DrawIndexed(mesh->indexCount, 0, 0);
    }

    void DrawModel(const ModelComponent& model) {
        // ... (See previous notes)
    }

    void SetShadowMap(ID3D11ShaderResourceView* srv) {
        shadowMapSRV_ = srv;
        if (srv && !IsValidSRV(srv)) {
            DEBUGLOG_WARNING("[RenderSystem] SetShadowMap: Invalid SRV detected, resetting to nullptr");
            shadowMapSRV_ = nullptr;
        }
    }

    void TriggerChromaticAberration(float intensity, float duration, float sampleOffset, float radialScale) {
        chromaticSampleOffset_ = std::max(0.0f, sampleOffset);
        chromaticRadialScale_ = std::max(0.0f, radialScale);
        intensity = std::max(0.0f, intensity);
        duration = std::max(0.0f, duration);
        if (intensity <= 0.0f || duration <= 0.0f) {
            ClearChromaticAberration();
            return;
        }
        chromaticStartIntensity_ = intensity;
        chromaticIntensityCurrent_ = intensity;
        chromaticDuration_ = duration;
        chromaticElapsed_ = 0.0f;
        chromaticActive_ = true;
    }

    void SetScenicLayer(float vignetteIntensity,
                        float noiseIntensity,
                        float fogIntensity,
                        float centerSafeRadius,
                        float colorTemperatureShift) {
        vignetteIntensity_ = std::clamp(vignetteIntensity, 0.0f, 1.0f);
        noiseIntensity_ = std::clamp(noiseIntensity, 0.0f, 1.0f);
        fogIntensity_ = std::clamp(fogIntensity, 0.0f, 1.0f);
        centerSafeRadius_ = std::clamp(centerSafeRadius, 0.0f, 0.49f);
        colorTemperatureShift_ = std::clamp(colorTemperatureShift, -1.0f, 1.0f);
    }

    void SetScenicMotion(float speedLineIntensity, float dirX, float dirY) {
        speedLineIntensity_ = std::clamp(speedLineIntensity, 0.0f, 1.0f);
        const float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len <= 1e-6f) {
            speedLineDirection_ = {0.0f, 1.0f};
            return;
        }
        speedLineDirection_ = {dirX / len, dirY / len};
    }

    void TriggerMicroFlash(float intensity, float duration) {
        intensity = std::max(0.0f, intensity);
        duration = std::max(0.0f, duration);
        if (intensity <= 0.0f || duration <= 0.0f) {
            return;
        }
        flashStartIntensity_ = intensity;
        flashIntensityCurrent_ = intensity;
        flashDuration_ = duration;
        flashElapsed_ = 0.0f;
        flashActive_ = true;
    }

    void TriggerEmissionRipple(float intensity, float duration) {
        intensity = std::max(0.0f, intensity);
        duration = std::max(0.0f, duration);
        if (intensity <= 0.0f || duration <= 0.0f) {
            return;
        }
        rippleStartIntensity_ = intensity;
        rippleIntensityCurrent_ = intensity;
        rippleDuration_ = duration;
        rippleElapsed_ = 0.0f;
        rippleActive_ = true;
    }

    void UpdateScreenEffects(float dt) {
        const float safeDt = std::max(0.0f, dt);
        screenEffectTime_ += safeDt;

        if (chromaticActive_) {
            chromaticElapsed_ += safeDt;
            if (chromaticElapsed_ >= chromaticDuration_) {
                ClearChromaticAberration();
            } else {
                float t = chromaticElapsed_ / std::max(0.001f, chromaticDuration_);
                float fade = 1.0f - t;
                chromaticIntensityCurrent_ = chromaticStartIntensity_ * fade * fade;
            }
        }

        if (flashActive_) {
            flashElapsed_ += safeDt;
            if (flashElapsed_ >= flashDuration_) {
                flashActive_ = false;
                flashIntensityCurrent_ = 0.0f;
            } else {
                const float t = flashElapsed_ / std::max(0.001f, flashDuration_);
                const float fade = 1.0f - t;
                flashIntensityCurrent_ = flashStartIntensity_ * fade;
            }
        }

        if (rippleActive_) {
            rippleElapsed_ += safeDt;
            if (rippleElapsed_ >= rippleDuration_) {
                rippleActive_ = false;
                rippleIntensityCurrent_ = 0.0f;
            } else {
                const float t = rippleElapsed_ / std::max(0.001f, rippleDuration_);
                const float fade = 1.0f - t;
                rippleIntensityCurrent_ = rippleStartIntensity_ * fade * fade;
            }
        }
    }

    void ClearScreenEffects() {
        ClearChromaticAberration();
        flashActive_ = false;
        rippleActive_ = false;
        flashStartIntensity_ = 0.0f;
        flashIntensityCurrent_ = 0.0f;
        flashDuration_ = 0.0f;
        flashElapsed_ = 0.0f;
        rippleStartIntensity_ = 0.0f;
        rippleIntensityCurrent_ = 0.0f;
        rippleDuration_ = 0.0f;
        rippleElapsed_ = 0.0f;
        vignetteIntensity_ = 0.0f;
        noiseIntensity_ = 0.0f;
        fogIntensity_ = 0.0f;
        centerSafeRadius_ = 0.20f;
        colorTemperatureShift_ = 0.0f;
        speedLineIntensity_ = 0.0f;
        speedLineDirection_ = {0.0f, 1.0f};
        screenEffectTime_ = 0.0f;
    }

  private:
    void ClearChromaticAberration() {
        chromaticActive_ = false;
        chromaticStartIntensity_ = 0.0f;
        chromaticIntensityCurrent_ = 0.0f;
        chromaticDuration_ = 0.0f;
        chromaticElapsed_ = 0.0f;
    }

    /**
     * @brief SRVが有効かチェック（AddRefでCOM参照カウントを確認）
     */
    bool IsValidSRV(ID3D11ShaderResourceView* srv) const {
        if (!srv) return false;
        ULONG refCount = srv->AddRef();
        if (refCount > 1) {
            srv->Release();
            return true;
        }
        return false;
    }

    /**
     * @brief ダミーシャドウマップの作成（フォールバック用）
     */
    bool CreateDummyShadowMap(GfxDevice &gfx) {
        float white = 1.0f; // Max distance
        D3D11_SUBRESOURCE_DATA data[6];
        for(int i=0; i<6; ++i) {
            data[i].pSysMem = &white;
            data[i].SysMemPitch = sizeof(float);
            data[i].SysMemSlicePitch = 0;
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = 1;
        texDesc.Height = 1;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 6;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT hr = gfx.Dev()->CreateTexture2D(&texDesc, data, dummyShadowTex_.GetAddressOf());
        if (FAILED(hr)) {
             DEBUGLOG_ERROR("Failed to create dummy shadow texture");
             return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.TextureCube.MostDetailedMip = 0;

        hr = gfx.Dev()->CreateShaderResourceView(dummyShadowTex_.Get(), &srvDesc, dummyShadowSRV_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("Failed to create dummy shadow SRV");
            return false;
        }
        return true;
    }

    ID3D11ShaderResourceView* shadowMapSRV_ = nullptr; // 追加
    Microsoft::WRL::ComPtr<ID3D11Texture2D> dummyShadowTex_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyShadowSRV_;

    /**
     * @struct MeshData
     * @brief メッシュデータ
     */
    struct MeshData {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;
    };


    /**
     * @struct Vertex
     * @brief 頂点データ
     */
    struct Vertex {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 tex;
        DirectX::XMFLOAT3 nrm;
        DirectX::XMFLOAT3 tan;
        DirectX::XMFLOAT3 bitan;
        uint32_t boneIndices[4];
        float boneWeights[4];
    };

    struct SkinningConstants {
        DirectX::XMFLOAT4X4 boneTransforms[128];
        DirectX::XMFLOAT4 skinDebug; // x:weightSum<=0 flag
    };

    // DirectX11リソース
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vsCb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> skinningCb_;
    DirectX::XMFLOAT4 skinDebugCPU_{0, 0, 0, 0}; // デバッグ用
    Microsoft::WRL::ComPtr<ID3D11Buffer> psCb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psLightCb_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerStateClamp_;

    // メッシュキャッシュ
    std::unordered_map<int, std::unique_ptr<MeshData>> meshCache_;

    // 状態管理
    bool initialized_ = false;
    Statistics stats_;
    bool chromaticActive_ = false;
    float chromaticStartIntensity_ = 0.0f;
    float chromaticIntensityCurrent_ = 0.0f;
    float chromaticDuration_ = 0.0f;
    float chromaticElapsed_ = 0.0f;
    float chromaticSampleOffset_ = 0.0f;
    float chromaticRadialScale_ = 0.0f;
    bool flashActive_ = false;
    float flashStartIntensity_ = 0.0f;
    float flashIntensityCurrent_ = 0.0f;
    float flashDuration_ = 0.0f;
    float flashElapsed_ = 0.0f;
    bool rippleActive_ = false;
    float rippleStartIntensity_ = 0.0f;
    float rippleIntensityCurrent_ = 0.0f;
    float rippleDuration_ = 0.0f;
    float rippleElapsed_ = 0.0f;
    float vignetteIntensity_ = 0.0f;
    float noiseIntensity_ = 0.0f;
    float fogIntensity_ = 0.0f;
    float centerSafeRadius_ = 0.20f;
    float colorTemperatureShift_ = 0.0f;
    float speedLineIntensity_ = 0.0f;
    DirectX::XMFLOAT2 speedLineDirection_ = {0.0f, 1.0f};
    float screenEffectTime_ = 0.0f;

    /**
     * @brief シェーダーのコンパイル
     */
    bool CompileShaders(GfxDevice &gfx) {
        const char *VS = R"(
            cbuffer PerObject : register(b0) {
                float4x4 gWorld;
                float4x4 gWVP;
                float4 gUVTransform;
            };

            cbuffer Skinning : register(b1) {
                row_major float4x4 gBoneTransforms[128]; // CPU側はrow-majorで送る
                float4 gSkinDebug; // x:weightSum<=0 flag
            };

            struct VSIn {
                float3 pos : POSITION;
                float2 tex : TEXCOORD;
                float3 nrm : NORMAL;
                float3 tan : TANGENT;
                float3 bitan : BITANGENT;
                uint4 boneIndices : BLENDINDICES;
                float4 boneWeights : BLENDWEIGHT;
            };

            struct VSOut {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD;
                float3 nrm : NORMAL;
                float3 tan : TANGENT;
                float3 bitan : BITANGENT;
                float3 worldPos : WORLDPOS;
            };

            VSOut main(VSIn i) {
                VSOut o;
                float3 nrmL = i.nrm;
                float3 tanL = i.tan;
                float3 bitanL = i.bitan;
                // local position before skinning (used by skinning code)
                float3 posL = i.pos;

                // スキニング計算
                float weights[4] = {i.boneWeights.x, i.boneWeights.y, i.boneWeights.z, i.boneWeights.w};
                uint indices[4] = {i.boneIndices.x, i.boneIndices.y, i.boneIndices.z, i.boneIndices.w};

                // ウェイトの合計を確認（0ならスキニングしない）
                float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
                // gSkinDebug は定数バッファの値であり、シェーダー内で書き換え不可のため、代わりにローカル変数で保持
                float dbgZeroWeight = (weightSum <= 0.001f) ? 1.0f : 0.0f;

                if (weightSum > 0.001f) {
                    float3 p = 0.0f;
                    float3 n = 0.0f;
                    float3 t = 0.0f;
                    float3 b = 0.0f;

                    [unroll]
                    for(int j=0; j<4; ++j) {
                        float w = weights[j];
                        if (w > 0.0f) {
                            // row_major行列 × 行ベクトル
                            p += mul(float4(posL, 1.0f), gBoneTransforms[indices[j]]).xyz * w;
                            n += mul(nrmL, (float3x3)gBoneTransforms[indices[j]]) * w;
                            t += mul(tanL, (float3x3)gBoneTransforms[indices[j]]) * w;
                            b += mul(bitanL, (float3x3)gBoneTransforms[indices[j]]) * w;
                        }
                    }
                    posL = p;
                    nrmL = normalize(n);
                    tanL = normalize(t);
                    bitanL = normalize(b);
                }

                o.pos = mul(float4(posL, 1.0f), gWVP);
                o.worldPos = mul(float4(posL, 1.0f), gWorld).xyz;
                o.nrm = mul(nrmL, (float3x3)gWorld);
                o.tan = mul(tanL, (float3x3)gWorld);
                o.bitan = mul(bitanL, (float3x3)gWorld);
                o.tex = i.tex * gUVTransform.zw + gUVTransform.xy;
                return o;
            }
        )";

        const char *PS = R"(
            struct PointLightGPU {
                float3 position; float range;
                float3 color; float intensity;
                float constantAtt; float linearAtt; float quadraticAtt; float enabled;
            };

            cbuffer PerObject : register(b0) {
                float4 gColor;
                float gUseTexture;
                float gUseNormalMap;
                float gUseLighting;
                float gSpecularAttenuation;
                float gSpecularEccentricity;
                float3 gSpecularColor;
                float padding_obj;
                float3 gReflectionColor;
                float gReflectance;
                float3 gPaddingEnd;
                float4 gChromaticParams;
                float4 gChromaticScreen;
                float4 gStylizeParams0;
                float4 gStylizeParams1;
                float4 gStylizeParams2;
            };

            cbuffer PerFrameLighting : register(b1) {
                float3 gAmbientColor; float gAmbientIntensity;
                float3 gEyePos; int gActivePointLights;
                float3 gDirLightDir; float gDirLightEnabled;
                float3 gDirLightColor; float gDirLightIntensity;
                int gShadowLightIndex; float3 gPaddingLight;
                PointLightGPU gPointLights[64];
            };

            cbuffer MaterialBuffer : register(b2) {
                float4 gDiffuseColor;
                float3 gEmissiveColor; float gEmissiveIntensity;
            };

            Texture2D gTexture : register(t0);
            Texture2D gNormalMap : register(t1);
            TextureCube gShadowMap : register(t2);
            SamplerState gSampler : register(s0);

            struct VSOut {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD;
                float3 nrm : NORMAL;
                float3 tan : TANGENT;
                float3 bitan : BITANGENT;
                float3 worldPos : WORLDPOS;
            };

            float CalcShadow(float3 worldPos, float3 lightPos, float lightRange) {
                float3 toPixel = worldPos - lightPos;
                float distToPixel = length(toPixel);
                float3 dir = toPixel / distToPixel;

                // Sample shadow map (R32_FLOAT stores normDist = dist / range)
                float storedNormDist = gShadowMap.Sample(gSampler, dir).r;
                float storedDist = storedNormDist * lightRange;

                // Bias to prevent acne
                float bias = 0.05;
                if (distToPixel - bias > storedDist) {
                    return 0.0; // Shadowed
                }
                return 1.0; // Lit
            }

            float3 ApplyPointLight(PointLightGPU L,
                                   float3 worldPos,
                                   float3 normal,
                                   float3 viewDir,
                                   float3 baseColor,
                                   float3 tangent,
                                   float3 bitangent,
                                   float specAtten,
                                   float specEcc,
                                   float3 specColor,
                                   float3 reflectionColor,
                                   float reflectance,
                                   float shadowFactor)
            {
                if (L.enabled < 0.5)
                    return 0.0.xxx;
                float3 toLight = L.position - worldPos;
                float dist = length(toLight);
                if (dist > L.range)
                    return 0.0.xxx;
                float3 lightDir = toLight / max(dist, 1e-4);
                float NdotL = max(0.0, dot(normal, lightDir));
                float att = 1.0 / max(L.constantAtt + L.linearAtt * dist + L.quadraticAtt * dist * dist, 1e-4);

                // Shadow implementation
                float vis = shadowFactor;

                float3 diffuse = baseColor * L.color * (NdotL * L.intensity * att * vis);
                if (specAtten <= 0.0001 || reflectance <= 0.0001)
                    return diffuse;
                // Blinn-Phong specular
                float3 halfDir = normalize(lightDir + viewDir);
                float ndoth = max(0.0, dot(normal, halfDir));
                // 簡易偏心補正: T/B 方向の成分で重み付け
                float3 t = normalize(tangent);
                float3 b = normalize(bitangent);
                float2 hTB = float2(dot(t, halfDir), dot(b, halfDir));
                float eccWeight = saturate(1.0 + specEcc * (hTB.x * hTB.x - hTB.y * hTB.y));
                float spec = pow(ndoth, 32.0) * eccWeight; // 基本の鋭さは固定
                // Fresnel Schlick
                float3 F0 = specColor * reflectance;
                float fresnel = pow(1.0 - max(0.0, dot(viewDir, halfDir)), 5.0);
                float3 F = F0 + (1.0.xxx - F0) * fresnel;
                float3 specular = F * (spec * L.color * L.intensity * att * specAtten * vis) * reflectionColor;
                return diffuse + specular;
            }

            float3 ApplyDirectional(float3 dir,
                                    float3 color,
                                    float intensity,
                                    float3 normal,
                                    float3 viewDir,
                                    float3 baseColor,
                                    float3 tangent,
                                    float3 bitangent,
                                    float specAtten,
                                    float specEcc,
                                    float3 specColor,
                                    float3 reflectionColor,
                                    float reflectance)
            {
                float NdotL = max(0.0, dot(normal, -normalize(dir)));
                float3 diffuse = baseColor * color * (NdotL * intensity);
                if (specAtten <= 0.0001 || reflectance <= 0.0001)
                    return diffuse;
                float3 halfDir = normalize(-normalize(dir) + viewDir);
                float ndoth = max(0.0, dot(normal, halfDir));
                float3 t = normalize(tangent);
                float3 b = normalize(bitangent);
                float2 hTB = float2(dot(t, halfDir), dot(b, halfDir));
                float eccWeight = saturate(1.0 + specEcc * (hTB.x * hTB.x - hTB.y * hTB.y));
                float spec = pow(ndoth, 32.0) * eccWeight;
                float3 F0 = specColor * reflectance;
                float fresnel = pow(1.0 - max(0.0, dot(viewDir, halfDir)), 5.0);
                float3 F = F0 + (1.0.xxx - F0) * fresnel;
                float3 specular = F * (spec * intensity * specAtten) * reflectionColor;
                return diffuse + specular;
            }

            float3 ApplyChromaticToFinalColor(float3 color,
                                              float2 fragPos,
                                              float2 screenSize,
                                              float intensity,
                                              float sampleOffset,
                                              float radialScale) {
                if (intensity <= 0.0001 || sampleOffset <= 0.00001 || screenSize.x <= 1.0 || screenSize.y <= 1.0) {
                    return color;
                }
                float2 screenUV = fragPos / screenSize;
                float2 centerDir = screenUV - float2(0.5, 0.5);
                float centerLen = length(centerDir);
                float strength = saturate(intensity * (1.0 + centerLen * radialScale));
                strength *= saturate(centerLen * 2.0);
                float fringe = strength * sampleOffset * 40.0;
                color.r = saturate(color.r + fringe);
                color.b = saturate(color.b - fringe);
                return color;
            }

            float ComputePeripheryMask(float2 uv, float centerSafeRadius) {
                float2 dist = abs(uv - float2(0.5, 0.5));
                float edge = max(dist.x, dist.y);
                float safe = saturate(centerSafeRadius);
                float denom = max(1e-4, 0.5 - safe);
                return saturate((edge - safe) / denom);
            }

            float Hash12(float2 p) {
                float h = dot(p, float2(127.1, 311.7));
                return frac(sin(h) * 43758.5453);
            }

            float3 ApplyStylizedLayer(float3 color, float2 fragPos, float2 screenSize) {
                if (screenSize.x <= 1.0 || screenSize.y <= 1.0) {
                    return color;
                }

                float2 uv = fragPos / screenSize;
                float edgeMask = ComputePeripheryMask(uv, gStylizeParams0.w);

                float vignette = saturate(gStylizeParams0.x) * edgeMask;
                color *= (1.0 - vignette * 0.35);

                float colorTemp = clamp(gStylizeParams1.x, -1.0, 1.0) * (0.35 + edgeMask * 0.65);
                float3 tempTint = float3(1.0 + colorTemp * 0.14,
                                         1.0 + colorTemp * 0.04,
                                         1.0 - colorTemp * 0.14);
                color *= saturate(tempTint);

                float noiseSeed = Hash12(uv * screenSize * 0.5 + gStylizeParams2.z * float2(41.0, 17.0));
                float signedNoise = (noiseSeed - 0.5) * 2.0;
                color += signedNoise * saturate(gStylizeParams0.y) * edgeMask * 0.03;

                float2 dir = float2(gStylizeParams1.z, gStylizeParams1.w);
                float dirLen = length(dir);
                if (dirLen > 1e-4) {
                    dir /= dirLen;
                } else {
                    dir = float2(0.0, 1.0);
                }
                float2 perp = float2(-dir.y, dir.x);
                float phase = dot(uv - float2(0.5, 0.5), perp) * 120.0 + gStylizeParams2.z * 35.0;
                float streak = smoothstep(0.86, 1.0, abs(sin(phase)));
                float speedLines = saturate(gStylizeParams1.y) * edgeMask * streak;
                color += speedLines * 0.08;

                float flash = saturate(gStylizeParams2.x);
                color += flash * (0.10 + edgeMask * 0.10);

                float ring = smoothstep(0.10, 0.0, abs(length(uv - float2(0.5, 0.5)) - 0.32));
                float ripple = saturate(gStylizeParams2.y) * edgeMask * ring;
                color += ripple * 0.12;

                return saturate(color);
            }

            float4 main(VSOut i) : SV_Target {
                float3 normal = normalize(i.nrm);
                if (gUseNormalMap > 0.5) {
                    float3x3 TBN = float3x3(normalize(i.tan), normalize(i.bitan), normalize(i.nrm));
                    float3 tangentNormal = gNormalMap.Sample(gSampler, i.tex).xyz * 2.0 - 1.0;
                    normal = normalize(mul(tangentNormal, TBN));
                }

                float4 base = gColor;
                if (gUseTexture > 0.5) {
                    float2 uv = i.tex;
                    float4 texel = gTexture.Sample(gSampler, uv);
                    if (gChromaticParams.x > 0.0001 && gChromaticScreen.x > 1.0 && gChromaticScreen.y > 1.0) {
                        float2 screenUV = i.pos.xy / gChromaticScreen.xy;
                        float2 centerDir = screenUV - float2(0.5, 0.5);
                        float centerLen = length(centerDir);
                        float2 dir = (centerLen > 1e-5) ? (centerDir / centerLen) : float2(0.0, 0.0);
                        float2 offset = dir * gChromaticParams.y * gChromaticParams.x * (1.0 + centerLen * gChromaticParams.z);
                        float2 uvR = saturate(uv + offset);
                        float2 uvB = saturate(uv - offset);
                        texel.r = gTexture.Sample(gSampler, uvR).r;
                        texel.b = gTexture.Sample(gSampler, uvB).b;
                    }
                    base *= texel;
                }

                float3 viewDir = normalize(gEyePos - i.worldPos);

                // アンリットならライト計算スキップ
                if (gUseLighting < 0.5) {
                    float3 unlitColor = base.rgb + gEmissiveColor * gEmissiveIntensity;
                    unlitColor = ApplyChromaticToFinalColor(unlitColor,
                                                            i.pos.xy,
                                                            gChromaticScreen.xy,
                                                            gChromaticParams.x,
                                                            gChromaticParams.y,
                                                            gChromaticParams.z);
                    unlitColor = ApplyStylizedLayer(unlitColor, i.pos.xy, gChromaticScreen.xy);
                    return float4(unlitColor, base.a);
                }

                float3 colorAccum = base.rgb * gAmbientColor * gAmbientIntensity;

                if (gDirLightEnabled > 0.5)
                {
                    colorAccum += ApplyDirectional(gDirLightDir, gDirLightColor, gDirLightIntensity, normal, viewDir, base.rgb, i.tan, i.bitan, gSpecularAttenuation, gSpecularEccentricity, gSpecularColor, gReflectionColor, gReflectance);
                }

                [unroll]
                for (int idx = 0; idx < gActivePointLights; ++idx) {
                    float shadow = 1.0f;
                    if (idx == gShadowLightIndex) {
                        shadow = CalcShadow(i.worldPos, gPointLights[idx].position, gPointLights[idx].range);
                    }
                    // Prevent total blackness if shadow is 0 (which happens if texture is unbound/black)
                    // This is a temporary safeguard. Real shadow maps might have 0.
                    // But usually fully shadowed is 0.
                    // If we want to debug, we can clamp shadow.
                    // For now, trust the logic, but if I disable shadow index in C++, this code path won't run.
                    colorAccum += ApplyPointLight(gPointLights[idx], i.worldPos, normal, viewDir, base.rgb, i.tan, i.bitan, gSpecularAttenuation, gSpecularEccentricity, gSpecularColor, gReflectionColor, gReflectance, shadow);
                }

                // emissive additive
                colorAccum += gEmissiveColor * gEmissiveIntensity;
                colorAccum = ApplyChromaticToFinalColor(colorAccum,
                                                        i.pos.xy,
                                                        gChromaticScreen.xy,
                                                        gChromaticParams.x,
                                                        gChromaticParams.y,
                                                        gChromaticParams.z);
                colorAccum = ApplyStylizedLayer(colorAccum, i.pos.xy, gChromaticScreen.xy);

                return float4(colorAccum, base.a);
            }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> vsb, psb, err;
        UINT compileFlags = D3D_COMPILE_STANDARD_FILE_INCLUDE ? D3DCOMPILE_ENABLE_STRICTNESS : D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(ENABLE_SHADER_DEBUG) && ENABLE_SHADER_DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        // 頂点シェーダーのコンパイル
        HRESULT hr = D3DCompile(VS, strlen(VS), nullptr, nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, vsb.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                std::string errorMsg(static_cast<const char *>(err->GetBufferPointer()), err->GetBufferSize());
                DEBUGLOG_ERROR("[RenderSystem] 頂点シェーダーのコンパイル失敗: " + errorMsg);
            } else {
                DEBUGLOG_ERROR("[RenderSystem] 頂点シェーダーのコンパイル失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            }
            return false;
        }

        // ピクセルシェーダーのコンパイル
        err.Reset();
        hr = D3DCompile(PS, strlen(PS), nullptr, nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, psb.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                std::string errorMsg(static_cast<const char *>(err->GetBufferPointer()), err->GetBufferSize());
                DEBUGLOG_ERROR("[RenderSystem] ピクセルシェーダーのコンパイル失敗: " + errorMsg);
            } else {
                DEBUGLOG_ERROR("[RenderSystem] ピクセルシェーダーのコンパイル失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            }
            return false;
        }

        // 頂点シェーダーの作成
        hr = gfx.Dev()->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, vs_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] 頂点シェーダーの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // ピクセルシェーダーの作成
        hr = gfx.Dev()->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, ps_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] ピクセルシェーダーの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // 入力レイアウトの作成のためにvsb_を保存
        vsBlob_ = vsb;

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] シェーダーのコンパイル完了");
        return true;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob_; // 入力レイアウト作成用に保持

    /**
     * @brief 入力レイアウトの作成
     */
    bool CreateInputLayout(GfxDevice &gfx) {
        D3D11_INPUT_ELEMENT_DESC il[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        HRESULT hr = gfx.Dev()->CreateInputLayout(il, 7, vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize(), layout_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] 入力レイアウトの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] 入力レイアウトの作成完了");
        return true;
    }

    /**
     * @brief 定数バッファの作成
     */
    bool CreateConstantBuffers(GfxDevice &gfx) {
        D3D11_BUFFER_DESC cbd{};
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = 0;
        cbd.MiscFlags = 0;
        cbd.StructureByteStride = 0;

        // VS定数バッファ
        cbd.ByteWidth = sizeof(VSConstants);
        HRESULT hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, vsCb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] VS定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // Skinning 定数バッファ
        cbd.ByteWidth = sizeof(SkinningConstants);
        hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, skinningCb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] Skinning定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // PS定数バッファ（PerObject）
        cbd.ByteWidth = sizeof(PSConstants);
        hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, psCb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] PS定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // 旧PSライト定数バッファ（未使用だが互換のため作成）
        cbd.ByteWidth = sizeof(PSLightConstants);
        hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, psLightCb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] PSライト定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] 定数バッファの作成完了");
        return true;
    }

    /**
     * @brief ステートの作成
     */
    bool CreateStates(GfxDevice &gfx) {
        // サンプラーステート
        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.MaxAnisotropy = 16;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT hr = gfx.Dev()->CreateSamplerState(&sampDesc, &samplerState_);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] サンプラーステートの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr = gfx.Dev()->CreateSamplerState(&sampDesc, samplerStateClamp_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] クランプサンプラーステートの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // ラスタライザーステート
        D3D11_RASTERIZER_DESC rsd{};
        rsd.FillMode = D3D11_FILL_SOLID;
        rsd.CullMode = D3D11_CULL_BACK;
        rsd.FrontCounterClockwise = FALSE;
        rsd.DepthClipEnable = TRUE;
        rsd.ScissorEnable = FALSE;
        rsd.MultisampleEnable = FALSE;
        rsd.AntialiasedLineEnable = FALSE;

        hr = gfx.Dev()->CreateRasterizerState(&rsd, rasterState_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] ラスタライザーステートの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] ステートの作成完了");
        return true;
    }

    /**
     * @brief 基本形状メッシュの作成
     */
    bool CreatePrimitiveMeshes(GfxDevice &gfx) {
        if (!CreateCubeMesh(gfx)) { DEBUGLOG_ERROR("[RenderSystem] キューブメッシュの作成失敗"); return false; }
        if (!CreateSphereMesh(gfx)) { DEBUGLOG_ERROR("[RenderSystem] 球体メッシュの作成失敗"); return false; }
        if (!CreateCylinderMesh(gfx)) { DEBUGLOG_ERROR("[RenderSystem] 円柱メッシュの作成失敗"); return false; }
        if (!CreatePlaneMesh(gfx)) { DEBUGLOG_ERROR("[RenderSystem] 平面メッシュの作成に失敗"); return false; }
        if (!CreateRightIsoTriPrismMesh(gfx)) { DEBUGLOG_ERROR("[RenderSystem] 直角二等辺三角柱メッシュの作成に失敗"); return false; }
        return true;
    }

    /**
     * @brief キューブメッシュの作成
     */
    bool CreateCubeMesh(GfxDevice &gfx) {
        const float size = 0.5f;

        // 頂点データ（各面に4頂点）
        Vertex vertices[] = {
            // Front face (Z+)
            {{-size, -size, size}, {0, 1}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
            {{size, -size, size}, {1, 1}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
            {{size, size, size}, {1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
            {{-size, size, size}, {0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}},

            // Back face (Z-)
            {{size, -size, -size}, {0, 1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
            {{-size, -size, -size}, {1, 1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
            {{-size, size, -size}, {1, 0}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
            {{size, size, -size}, {0, 0}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},

            // Left face (X-)
            {{-size, -size, -size}, {0, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
            {{-size, -size, size}, {1, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
            {{-size, size, size}, {1, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
            {{-size, size, -size}, {0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},

            // Right face (X+)
            {{size, -size, size}, {0, 1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
            {{size, -size, -size}, {1, 1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
            {{size, size, -size}, {1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
            {{size, size, size}, {0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}},

            // Top face (Y+)
            {{-size, size, size}, {0, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
            {{size, size, size}, {1, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
            {{size, size, -size}, {1, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
            {{-size, size, -size}, {0, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}},

            // Bottom face (Y-)
            {{-size, -size, -size}, {0, 1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{size, -size, -size}, {1, 1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{size, -size, size}, {1, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{-size, -size, size}, {0, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
        };

        // インデックスデータ（各面に2三角形 = 6インデックス）
        uint16_t indices[] = {
            0, 1, 2, 0, 2, 3,       // Front
            4, 5, 6, 4, 6, 7,       // Back
            8, 9, 10, 8, 10, 11,    // Left
            12, 13, 14, 12, 14, 15, // Right
            16, 17, 18, 16, 18, 19, // Top
            20, 21, 22, 20, 22, 23  // Bottom
        };

        return CreateMeshBuffers(gfx, vertices, sizeof(vertices) / sizeof(Vertex), indices, sizeof(indices) / sizeof(uint16_t), static_cast<int>(MeshType::Cube));
    }

    /**
   * @brief 球体メッシュの作成
     */
    bool CreateSphereMesh(GfxDevice &gfx) {
        const int segments = 32;
        const int rings = 16;
        const float radius = 0.5f;

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        // 頂点生成
        for (int ring = 0; ring <= rings; ++ring) {
            float phi = DirectX::XM_PI * ring / rings;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            for (int seg = 0; seg <= segments; ++seg) {
                float theta = 2.0f * DirectX::XM_PI * seg / segments;
                float sinTheta = sinf(theta);
                float cosTheta = cosf(theta);

                Vertex v;
                v.pos.x = radius * sinPhi * cosTheta;
                v.pos.y = radius * cosPhi;
                v.pos.z = radius * sinPhi * sinTheta;

                v.nrm.x = sinPhi * cosTheta;
                v.nrm.y = cosPhi;
                v.nrm.z = sinPhi * sinTheta;

                v.tex.x = static_cast<float>(seg) / segments;
                v.tex.y = static_cast<float>(ring) / rings;

                v.tan.x = -sinTheta;
                v.tan.y = 0;
                v.tan.z = cosTheta;

                v.bitan.x = cosPhi * cosTheta;
                v.bitan.y = -sinPhi;
                v.bitan.z = cosPhi * sinTheta;

                vertices.push_back(v);
            }
        }

        // インデックス生成
        for (int ring = 0; ring < rings; ++ring) {
            for (int seg = 0; seg < segments; ++seg) {
                int a = ring * (segments + 1) + seg;
                int b = a + 1;
                int c = a + segments + 1;
                int d = c + 1;

                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(b);

                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }

        return CreateMeshBuffers(gfx, vertices.data(), vertices.size(), indices.data(), indices.size(), static_cast<int>(MeshType::Sphere));
    }

    /**
     * @brief 円柱メッシュの作成
     */
    bool CreateCylinderMesh(GfxDevice &gfx) {
        const int segments = 32;
        const float radius = 0.5f;
        const float height = 1.0f;

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        // 側面の頂点
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * DirectX::XM_PI * i / segments;
            float cosTheta = cosf(theta);
            float sinTheta = sinf(theta);

            // トップの頂点
            Vertex vTop;
            vTop.pos = {radius * cosTheta, height / 2, radius * sinTheta};
            vTop.tex = {static_cast<float>(i) / segments, 0.0f};
            vTop.nrm = {cosTheta, 0, sinTheta};
            vTop.tan = {-sinTheta, 0, cosTheta};
            vTop.bitan = {0, 1, 0};
            vertices.push_back(vTop);

            // ボトムの頂点
            Vertex vBottom;
            vBottom.pos = {radius * cosTheta, -height / 2, radius * sinTheta};
            vBottom.tex = {static_cast<float>(i) / segments, 1.0f};
            vBottom.nrm = {cosTheta, 0, sinTheta};
            vBottom.tan = {-sinTheta, 0, cosTheta};
            vBottom.bitan = {0, 1, 0};
            vertices.push_back(vBottom);
        }

        // 側面のインデックス
        for (int i = 0; i < segments; ++i) {
            int top1 = i * 2;
            int bot1 = i * 2 + 1;
            int top2 = (i + 1) * 2;
            int bot2 = (i + 1) * 2 + 1;

            indices.push_back(top1);
            indices.push_back(bot1);
            indices.push_back(top2);

            indices.push_back(top2);
            indices.push_back(bot1);
            indices.push_back(bot2);
        }

        // キャップの追加は省略（実装を簡略化）

        return CreateMeshBuffers(gfx, vertices.data(), vertices.size(), indices.data(), indices.size(), static_cast<int>(MeshType::Cylinder));
    }

    /**
     * @brief 平面メッシュの作成
     */
    bool CreatePlaneMesh(GfxDevice &gfx) {
        const float size = 0.5f;

        Vertex vertices[] = {
            {{-size, 0, -size}, {0, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{size, 0, -size}, {1, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{size, 0, size}, {1, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{-size, 0, size}, {0, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}},
        };

        uint16_t indices[] = {0, 1, 2, 0, 2, 3};

        return CreateMeshBuffers(gfx, vertices, 4, indices, 6, static_cast<int>(MeshType::Plane));
    }

    /**
     * @brief 直角二等辺三角柱メッシュの作成
     */
    bool CreateRightIsoTriPrismMesh(GfxDevice &gfx) {
        const float s = 0.5f;
        std::vector<Vertex> v;
        std::vector<uint16_t> idx;
        DirectX::XMFLOAT3 A{-s,-s,-s};
        DirectX::XMFLOAT3 B{ s,-s,-s};
        DirectX::XMFLOAT3 C{-s,-s, s};
        DirectX::XMFLOAT3 A2{-s,s,-s};
        DirectX::XMFLOAT3 B2{ s,s,-s};
        DirectX::XMFLOAT3 C2{-s,s, s};
        auto make = [&](DirectX::XMFLOAT3 p, DirectX::XMFLOAT2 uv, DirectX::XMFLOAT3 n){ v.push_back({p,uv,n,{1,0,0},{0,1,0}}); };
        make(A,{0,1},{0,-1,0}); make(B,{1,1},{0,-1,0}); make(C,{0,0},{0,-1,0});
        make(A2,{0,1},{0,1,0}); make(C2,{0,0},{0,1,0}); make(B2,{1,1},{0,1,0});
        DirectX::XMFLOAT3 nBack{0,0,-1};
        make(A,{0,1},nBack); make(A2,{0,0},nBack); make(B2,{1,0},nBack); make(B,{1,1},nBack);
        DirectX::XMFLOAT3 nFront{0,0,1};
        make(B,{1,1},nFront); make(B2,{1,0},nFront); make(C2,{0,0},nFront); make(C,{0,1},nFront);
        DirectX::XMFLOAT3 nLeft{-1,0,0};
        make(C,{1,1},nLeft); make(C2,{1,0},nLeft); make(A2,{0,0},nLeft); make(A,{0,1},nLeft);
        auto calcNormal = [](DirectX::XMFLOAT3 p0, DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2){ using namespace DirectX; XMVECTOR v0=XMLoadFloat3(&p0), v1=XMLoadFloat3(&p1), v2=XMLoadFloat3(&p2); XMVECTOR n = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(v1,v0), XMVectorSubtract(v2,v0))); DirectX::XMFLOAT3 r; XMStoreFloat3(&r,n); return r; };
        DirectX::XMFLOAT3 nDiag = calcNormal(B,A,A2);
        make(B,{1,1},nDiag); make(A,{0,1},nDiag); make(A2,{0,0},nDiag); make(B2,{1,0},nDiag);
        DirectX::XMFLOAT3 nDiag2 = calcNormal(C,B,B2);
        make(C,{0,1},nDiag2); make(B,{1,1},nDiag2); make(B2,{1,0},nDiag2); make(C2,{0,0},nDiag2);
        idx.insert(idx.end(),{0,1,2,3,4,5});
        idx.insert(idx.end(),{6,7,8,6,8,9});
        idx.insert(idx.end(),{10,11,12,10,12,13});
        idx.insert(idx.end(),{14,15,16,14,16,17});
        idx.insert(idx.end(),{18,19,20,18,20,21});
        idx.insert(idx.end(),{22,23,24,22,24,25});
        return CreateMeshBuffers(gfx, v.data(), v.size(), idx.data(), idx.size(), static_cast<int>(MeshType::RightIsoTriPrism));
    }

    /**
     * @brief メッシュバッファの作成
     */
    bool CreateMeshBuffers(GfxDevice &gfx, const Vertex *vertices, size_t vertexCount, const uint16_t *indices, size_t indexCount, int meshTypeKey) {
        auto meshData = std::make_unique<MeshData>();

        // 頂点バッファの作成
        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.ByteWidth = static_cast<UINT>(vertexCount * sizeof(Vertex));
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = 0;
        vbd.MiscFlags = 0;
        vbd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData{};
        vData.pSysMem = vertices;

        HRESULT hr = gfx.Dev()->CreateBuffer(&vbd, &vData, meshData->vertexBuffer.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] 頂点バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // インデックスバッファの作成
        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = static_cast<UINT>(indexCount * sizeof(uint16_t));
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        ibd.MiscFlags = 0;
        ibd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA iData{};
        iData.pSysMem = indices;

        hr = gfx.Dev()->CreateBuffer(&ibd, &iData, meshData->indexBuffer.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] インデックスバッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        meshData->indexCount = static_cast<UINT>(indexCount);
        meshCache_[meshTypeKey] = std::move(meshData);

        return true;
    }

    /**
   * @brief パイプラインの設定
     */
    void SetupPipeline(GfxDevice &gfx) {
        gfx.Ctx()->IASetInputLayout(layout_.Get());
        gfx.Ctx()->VSSetShader(vs_.Get(), nullptr, 0);
        gfx.Ctx()->PSSetShader(ps_.Get(), nullptr, 0);
        gfx.Ctx()->VSSetConstantBuffers(0, 1, vsCb_.GetAddressOf());
        gfx.Ctx()->PSSetConstantBuffers(0, 1, psCb_.GetAddressOf());
        // b1/b2 は統合RenderingSystem側でバインドする
        gfx.Ctx()->PSSetSamplers(0, 1, samplerState_.GetAddressOf());
        gfx.Ctx()->RSSetState(rasterState_.Get());
        gfx.Ctx()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void SetClampSampler(bool enabled, GfxDevice &gfx) {
        ID3D11SamplerState *samplers[1] = {nullptr};
        if (enabled) {
            samplers[0] = samplerStateClamp_.Get();
        } else {
            samplers[0] = samplerState_.Get();
        }
        gfx.Ctx()->PSSetSamplers(0, 1, samplers);
    }

    /**
* @brief ライト定数の更新（非使用・互換維持）
     */
    void UpdateLightConstants(World &w, const Camera &cam, GfxDevice &gfx) {
        PSLightConstants lightCbuf;
        lightCbuf.eyePos = cam.position;

        w.ForEach<DirectionalLight>([&](Entity e, DirectionalLight &l) {
            lightCbuf.light = l;
        });

        gfx.Ctx()->UpdateSubresource(psLightCb_.Get(), 0, nullptr, &lightCbuf, 0, 0);
    }

    /**
     * @brief ModelComponentの描画
     */
    void RenderModelComponents(World &w, GfxDevice &gfx, const Camera &cam, TextureManager &texMgr) {
        std::unordered_map<uint32_t, DirectX::XMMATRIX> worldCache;
        w.ForEach<ModelComponent>([&](Entity e, ModelComponent &mc) {
            auto *t = w.TryGet<Transform>(e);
            if (!t)
                return;
            if (!mc.vertexBuffer || !mc.indexBuffer)
                return;

            // ワールド行列の計算
            DirectX::XMMATRIX worldMatrix = CalculateWorldMatrix(w, e, *t, worldCache);

            bool useClampSampler = false;
            if (mc.useLighting < 0.5f && mc.normalTexture == TextureManager::INVALID_TEXTURE) {
                if (auto *hier = w.TryGet<TransformHierarchy>(e)) {
                    useClampSampler = (hier->GetParent() == Entity{});
                }
            }
            SetClampSampler(useClampSampler, gfx);

                // 定数バッファの更新
            UpdateVSConstants(gfx, worldMatrix, cam, mc.uvOffset, mc.uvScale);
            UpdatePSConstants(gfx,
                              mc.color,
                              mc.texture,
                              mc.normalTexture,
                              mc.useLighting,
                              mc.specularAttenuation,
                              mc.specularColor,
                              mc.reflectance,
                              mc.reflectionColor,
                              mc.specularEccentricity);

            // スキニング定数バッファの更新とバインド
            if (mc.isSkinned) {
                SkinningConstants skCbuf;
                // ボーン変換行列をコピー (最大128個まで) - 転置しない（row_major前提）
                size_t boneCount = std::min(mc.skeleton.boneTransforms.size(), size_t(128));
                for (size_t i = 0; i < boneCount; ++i) {
                    skCbuf.boneTransforms[i] = mc.skeleton.boneTransforms[i];
                }
                skCbuf.skinDebug = skinDebugCPU_;
                // 足りない分は単位行列で埋める（念のため）
                for (size_t i = boneCount; i < 128; ++i) {
                    DirectX::XMStoreFloat4x4(&skCbuf.boneTransforms[i], DirectX::XMMatrixIdentity());
                }

                gfx.Ctx()->UpdateSubresource(skinningCb_.Get(), 0, nullptr, &skCbuf, 0, 0);
                gfx.Ctx()->VSSetConstantBuffers(1, 1, skinningCb_.GetAddressOf());

#ifdef _DEBUG
                static int s_skinningLogCount = 0;
                if (s_skinningLogCount < 3) {
                    DEBUGLOG("Skinning debug: boneCount=" + std::to_string(boneCount) +
                             ", skeletonSize=" + std::to_string(mc.skeleton.bones.size()) +
                             ", bone0 m41,m42,m43=" +
                             std::to_string(mc.skeleton.boneTransforms.empty() ? 0.0f : mc.skeleton.boneTransforms[0]._41) + "," +
                             std::to_string(mc.skeleton.boneTransforms.empty() ? 0.0f : mc.skeleton.boneTransforms[0]._42) + "," +
                             std::to_string(mc.skeleton.boneTransforms.empty() ? 0.0f : mc.skeleton.boneTransforms[0]._43) +
                             ", dbgZeroWeight=" + std::to_string(skinDebugCPU_.x));
                    s_skinningLogCount++;
                }
#endif
            } else {
                // スキニングなしの場合は単位行列などをセットするか、何もしない
                // シェーダー側でウェイト0なら計算されないので、バインドしなくてもゴミ値で計算されるだけだが、念のため単位行列をセットしておくと安全
                // ここではパフォーマンス優先でバインドしない（ウェイト0で保護されている前提）
                // ただし、もし前の描画でバインドされていたら？ -> シェーダーが同じならスロットは残るかもしれないが、
                // 今回のシェーダーはウェイトの合計を見て分岐しているので大丈夫なはず。
            }

            // エミッシブ/マテリアルの設定
            RenderingSystem::GetInstance().SetMaterialForEntity(gfx.Ctx(), e, w);

            // テクスチャの設定
            SetTextures(gfx, texMgr, mc.texture, mc.normalTexture);

            // 描画
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            gfx.Ctx()->IASetVertexBuffers(0, 1, mc.vertexBuffer.GetAddressOf(), &stride, &offset);
            gfx.Ctx()->IASetIndexBuffer(mc.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            gfx.Ctx()->DrawIndexed(mc.indexCount, 0, 0);

            if (useClampSampler) {
                SetClampSampler(false, gfx);
            }

            stats_.modelsRendered++;
            stats_.totalDrawCalls++;
        });
    }

    /**
     * @brief MeshRendererの描画
   */
    void RenderMeshRenderers(World &w, GfxDevice &gfx, const Camera &cam, TextureManager &texMgr) {
        std::unordered_map<uint32_t, DirectX::XMMATRIX> worldCache;
        w.ForEach<Transform, MeshRenderer>([&](Entity e, Transform &t, MeshRenderer &mr) {
            // メッシュデータの取得
            auto it = meshCache_.find(static_cast<int>(mr.meshType));
            if (it == meshCache_.end() || !it->second) {
                DEBUGLOG_WARNING("[RenderSystem] MeshType not found: " + std::to_string(static_cast<int>(mr.meshType)));
                return;
            }

            auto *meshData = it->second.get();
            if (!meshData->vertexBuffer || !meshData->indexBuffer)
                return;

            // ワールド行列の計算
            DirectX::XMMATRIX worldMatrix = CalculateWorldMatrix(w, e, t, worldCache);

            // 定数バッファの更新
            UpdateVSConstants(gfx, worldMatrix, cam, mr.uvOffset, mr.uvScale);
            UpdatePSConstants(gfx,
                              mr.color,
                              mr.texture,
                              TextureManager::INVALID_TEXTURE,
                              mr.useLighting,
                              mr.specularAttenuation,
                              mr.specularColor,
                              mr.reflectance,
                              mr.reflectionColor,
                              mr.specularEccentricity);

            // エミッシブ/マテリアルの設定
            RenderingSystem::GetInstance().SetMaterialForEntity(gfx.Ctx(), e, w);

            // テクスチャの設定
            SetTextures(gfx, texMgr, mr.texture, TextureManager::INVALID_TEXTURE);

            // 描画
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            gfx.Ctx()->IASetVertexBuffers(0, 1, meshData->vertexBuffer.GetAddressOf(), &stride, &offset);
            gfx.Ctx()->IASetIndexBuffer(meshData->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            gfx.Ctx()->DrawIndexed(meshData->indexCount, 0, 0);

            stats_.meshesRendered++;
            stats_.totalDrawCalls++;
        });
    }

    /**
     * @brief ローカル行列の計算
     */
    DirectX::XMMATRIX BuildLocalMatrix(const Transform &t) const {
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
            DirectX::XMConvertToRadians(t.rotation.x),
            DirectX::XMConvertToRadians(t.rotation.y),
            DirectX::XMConvertToRadians(t.rotation.z));
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

        return S * R * T;
    }

    /**
     * @brief 親子階層を考慮したワールド行列の計算（メモ化付き）
     */
    DirectX::XMMATRIX CalculateWorldMatrix(
        const World &w,
        Entity e,
        const Transform &t,
        std::unordered_map<uint32_t, DirectX::XMMATRIX> &cache) const {
        auto it = cache.find(e.id);
        if (it != cache.end()) {
            return it->second;
        }

        DirectX::XMMATRIX world = BuildLocalMatrix(t);
        auto *hierarchy = w.TryGet<TransformHierarchy>(e);
        if (hierarchy && hierarchy->HasParent()) {
            auto parentOpt = hierarchy->GetParent();
            if (parentOpt && w.IsAlive(*parentOpt)) {
                if (auto *parentTransform = w.TryGet<Transform>(*parentOpt)) {
                    DirectX::XMMATRIX parentWorld = CalculateWorldMatrix(w, *parentOpt, *parentTransform, cache);
                    world = DirectX::XMMatrixMultiply(world, parentWorld);
                }
            }
        }

        cache[e.id] = world;
        return world;
    }

    /**
     * @brief VS定数バッファの更新
     */
    void UpdateVSConstants(GfxDevice &gfx, const DirectX::XMMATRIX &worldMatrix, const Camera &cam, const DirectX::XMFLOAT2 &uvOffset, const DirectX::XMFLOAT2 &uvScale) {
        VSConstants vsCbuf;
        vsCbuf.World = DirectX::XMMatrixTranspose(worldMatrix);
        vsCbuf.WVP = DirectX::XMMatrixTranspose(worldMatrix * cam.View * cam.Proj);
        vsCbuf.uvTransform = DirectX::XMFLOAT4{uvOffset.x, uvOffset.y, uvScale.x, uvScale.y};

        gfx.Ctx()->UpdateSubresource(vsCb_.Get(), 0, nullptr, &vsCbuf, 0, 0);
    }

    /**
     * @brief PS定数バッファの更新
     */
    void UpdatePSConstants(GfxDevice &gfx,
                           const DirectX::XMFLOAT3 &color,
                           TextureManager::TextureHandle texture,
                           TextureManager::TextureHandle normalTexture,
                           float useLighting,
                           float specularAttenuation,
                           const DirectX::XMFLOAT3 &specularColor,
                           float reflectance,
                           const DirectX::XMFLOAT3 &reflectionColor,
                           float specularEccentricity) {
        PSConstants psCbuf;
        psCbuf.color = DirectX::XMFLOAT4{color.x, color.y, color.z, 1.0f};
        psCbuf.useTexture = (texture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
        psCbuf.useNormalMap = (normalTexture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
        psCbuf.useLighting = useLighting;
        psCbuf.specularAttenuation = specularAttenuation;
        psCbuf.specularEccentricity = specularEccentricity;
        psCbuf.specularColor = specularColor;
        psCbuf.reflectionColor = reflectionColor;
        psCbuf.reflectance = reflectance;
        psCbuf.paddingEnd[0] = psCbuf.paddingEnd[1] = psCbuf.paddingEnd[2] = 0.0f;
        psCbuf.chromaticParams = DirectX::XMFLOAT4{chromaticIntensityCurrent_, chromaticSampleOffset_, chromaticRadialScale_, 0.0f};
        D3D11_VIEWPORT viewport = gfx.GetCurrentViewport();
        psCbuf.chromaticScreen = DirectX::XMFLOAT4{std::max(1.0f, viewport.Width), std::max(1.0f, viewport.Height), 0.0f, 0.0f};
        psCbuf.stylizeParams0 = DirectX::XMFLOAT4{vignetteIntensity_, noiseIntensity_, fogIntensity_, centerSafeRadius_};
        psCbuf.stylizeParams1 = DirectX::XMFLOAT4{colorTemperatureShift_, speedLineIntensity_, speedLineDirection_.x, speedLineDirection_.y};
        psCbuf.stylizeParams2 = DirectX::XMFLOAT4{flashIntensityCurrent_, rippleIntensityCurrent_, screenEffectTime_, 0.0f};

        gfx.Ctx()->UpdateSubresource(psCb_.Get(), 0, nullptr, &psCbuf, 0, 0);
    }

    /**
     * @brief テクスチャの設定
     */
    void SetTextures(GfxDevice &gfx, TextureManager &texMgr, TextureManager::TextureHandle texture, TextureManager::TextureHandle normalTexture) {
        ID3D11ShaderResourceView *srvs[2] = {nullptr, nullptr};

        if (texture != TextureManager::INVALID_TEXTURE) {
            srvs[0] = texMgr.GetSRV(texture);
        }

        if (normalTexture != TextureManager::INVALID_TEXTURE) {
            srvs[1] = texMgr.GetSRV(normalTexture);
        }

        gfx.Ctx()->PSSetShaderResources(0, 2, srvs);
    }
};
