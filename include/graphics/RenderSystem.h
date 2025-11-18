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
#include "graphics/TextureManager.h"
#include "app/DebugLog.h"
#include "app/ServiceLocator.h"
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
        DirectX::XMFLOAT4 color; ///< マテリアルカラー
        float useTexture;        ///< テクスチャ使用フラグ
        float useNormalMap;      ///< ノーマルマップ使用フラグ
        float specularPower;     ///< スペキュラ強度
        float padding;           ///< パディング
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

        // ライト情報の更新
        UpdateLightConstants(w, cam, gfx);

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
        psCb_.Reset();
        psLightCb_.Reset();
        rasterState_.Reset();
        samplerState_.Reset();

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

  private:
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
    };

    // DirectX11リソース
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vsCb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psCb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psLightCb_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;

    // メッシュキャッシュ
    std::unordered_map<int, std::unique_ptr<MeshData>> meshCache_;

    // 状態管理
    bool initialized_ = false;
    Statistics stats_;

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

     struct VSIn {
      float3 pos : POSITION;
                float2 tex : TEXCOORD;
      float3 nrm : NORMAL;
      float3 tan : TANGENT;
       float3 bitan : BITANGENT;
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
           o.pos = mul(float4(i.pos, 1.0f), gWVP);
        o.worldPos = mul(float4(i.pos, 1.0f), gWorld).xyz;
         o.nrm = mul(i.nrm, (float3x3)gWorld);
        o.tan = mul(i.tan, (float3x3)gWorld);
           o.bitan = mul(i.bitan, (float3x3)gWorld);
         o.tex = i.tex * gUVTransform.zw + gUVTransform.xy;
    return o;
       }
        )";

        const char *PS = R"(
            struct DirectionalLight {
     float3 direction;
          float padding;
  float4 color;
     };

         cbuffer PerObject : register(b0) {
    float4 gColor;
    float gUseTexture;
       float gUseNormalMap;
   float gSpecularPower;
    float padding_obj;
            };

       cbuffer PerFrame : register(b1) {
       DirectionalLight gLight;
    float3 gAmbientColor;
    float padding_frame;
      float3 gEyePos;
          float padding_frame2;
   };

   Texture2D gTexture : register(t0);
      Texture2D gNormalMap : register(t1);
 SamplerState gSampler : register(s0);

            struct VSOut {
       float4 pos : SV_POSITION;
       float2 tex : TEXCOORD;
 float3 nrm : NORMAL;
     float3 tan : TANGENT;
      float3 bitan : BITANGENT;
      float3 worldPos : WORLDPOS;
    };

    float4 main(VSOut i) : SV_Target {
      float3 normal = normalize(i.nrm);
     if (gUseNormalMap > 0.5) {
        float3x3 TBN = float3x3(normalize(i.tan), normalize(i.bitan), normalize(i.nrm));
            float3 tangentNormal = gNormalMap.Sample(gSampler, i.tex).xyz * 2.0 - 1.0;
               normal = normalize(mul(tangentNormal, TBN));
   }

     float light_factor = max(0.0f, dot(normal, -gLight.direction));

 float4 final_color = gColor;
         if (gUseTexture > 0.5) {
     final_color *= gTexture.Sample(gSampler, i.tex);
   }

         float3 toEye = normalize(gEyePos - i.worldPos);
     float3 reflection = reflect(gLight.direction, normal);
     float spec_factor = pow(max(0.0f, dot(toEye, reflection)), gSpecularPower);

    float3 diffuse = final_color.rgb * gLight.color.rgb * light_factor;
     float3 ambient = final_color.rgb * gAmbientColor;
           float3 specular = gLight.color.rgb * spec_factor;

          return float4(diffuse + ambient + specular, final_color.a);
            }
  )";

        Microsoft::WRL::ComPtr<ID3DBlob> vsb, psb, err;
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
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
            {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        HRESULT hr = gfx.Dev()->CreateInputLayout(il, 5, vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize(), layout_.GetAddressOf());
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

        // PS定数バッファ
        cbd.ByteWidth = sizeof(PSConstants);
        hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, psCb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[RenderSystem] PS定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // PSライト定数バッファ
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
        // Cube
        if (!CreateCubeMesh(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] キューブメッシュの作成失敗");
            return false;
        }

        // Sphere
        if (!CreateSphereMesh(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 球体メッシュの作成失敗");
            return false;
        }

        // Cylinder
        if (!CreateCylinderMesh(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 円柱メッシュの作成失敗");
            return false;
        }

        // Plane
        if (!CreatePlaneMesh(gfx)) {
            DEBUGLOG_ERROR("[RenderSystem] 平面メッシュの作成失敗");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "[RenderSystem] 基本形状メッシュの作成完了");
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
        gfx.Ctx()->PSSetConstantBuffers(1, 1, psLightCb_.GetAddressOf());
        gfx.Ctx()->PSSetSamplers(0, 1, samplerState_.GetAddressOf());
        gfx.Ctx()->RSSetState(rasterState_.Get());
        gfx.Ctx()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    /**
* @brief ライト定数の更新
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
        w.ForEach<ModelComponent>([&](Entity e, ModelComponent &mc) {
            auto *t = w.TryGet<Transform>(e);
            if (!t)
                return;
            if (!mc.vertexBuffer || !mc.indexBuffer)
                return;

            // ワールド行列の計算
            DirectX::XMMATRIX worldMatrix = CalculateWorldMatrix(*t);

            // 定数バッファの更新
            UpdateVSConstants(gfx, worldMatrix, cam, mc.uvOffset, mc.uvScale);
            UpdatePSConstants(gfx, mc.color, mc.texture, mc.normalTexture, 32.0f);

            // テクスチャの設定
            SetTextures(gfx, texMgr, mc.texture, mc.normalTexture);

            // 描画
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            gfx.Ctx()->IASetVertexBuffers(0, 1, mc.vertexBuffer.GetAddressOf(), &stride, &offset);
            gfx.Ctx()->IASetIndexBuffer(mc.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            gfx.Ctx()->DrawIndexed(mc.indexCount, 0, 0);

            stats_.modelsRendered++;
            stats_.totalDrawCalls++;
        });
    }

    /**
     * @brief MeshRendererの描画
   */
    void RenderMeshRenderers(World &w, GfxDevice &gfx, const Camera &cam, TextureManager &texMgr) {
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
            DirectX::XMMATRIX worldMatrix = CalculateWorldMatrix(t);

            // 定数バッファの更新
            UpdateVSConstants(gfx, worldMatrix, cam, mr.uvOffset, mr.uvScale);
            UpdatePSConstants(gfx, mr.color, mr.texture, TextureManager::INVALID_TEXTURE, 32.0f);

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
     * @brief ワールド行列の計算
     */
    DirectX::XMMATRIX CalculateWorldMatrix(const Transform &t) const {
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
            DirectX::XMConvertToRadians(t.rotation.x),
            DirectX::XMConvertToRadians(t.rotation.y),
            DirectX::XMConvertToRadians(t.rotation.z));
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

        return S * R * T;
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
    void UpdatePSConstants(GfxDevice &gfx, const DirectX::XMFLOAT3 &color, TextureManager::TextureHandle texture, TextureManager::TextureHandle normalTexture, float specularPower) {
        PSConstants psCbuf;
        psCbuf.color = DirectX::XMFLOAT4{color.x, color.y, color.z, 1.0f};
        psCbuf.useTexture = (texture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
        psCbuf.useNormalMap = (normalTexture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
        psCbuf.specularPower = specularPower;

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
