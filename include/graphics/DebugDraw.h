/**
 * @file DebugDraw.h
 * @brief デバッグ用の線描画システム
 * @author 山内陽
 * @date 2025
 * @version 6.0
 */
#pragma once
#include "graphics/GfxDevice.h"
#include "graphics/Camera.h"
#include "app/DebugLog.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

/**
 * @class DebugDraw
 * @brief デバッグ用の線描画システム
 *
 * @details
 * 開発中にグリッド、座標軸、任意の線を描画するためのクラスです。
 * ワールド空間でのデバッグ情報の可視化に使用します。
 *
 * ### 主な機能:
 * - 高パフォーマンスな線描画
 * - 動的頂点バッファの効率的な管理
 * - 描画統計の自動収集
 * - エラーハンドリングの強化
 *
 * ### 主な用途:
 * - グリッド表示(基準となる平面)
 * - 座標軸表示(X, Y, Z軸)
 * - 当たり判定の可視化
 * - 移動経路の表示
 *
 * @par 使用例
 * @code
 * DebugDraw debugDraw;
 * if (!debugDraw.Init(gfx)) {
 *     // 初期化失敗
 * return false;
 * }
 *
 * // グリッドと軸を描画
 * debugDraw.DrawGrid(20.0f, 20);
 * debugDraw.DrawAxes(5.0f);
 *
 * // カスタム線を描画
 * debugDraw.AddLine(
 *     DirectX::XMFLOAT3{0, 0, 0},
 *     DirectX::XMFLOAT3{5, 5, 5},
 *   DirectX::XMFLOAT3{1, 1, 0}  // 黄色
 * );
 *
 * // 描画実行
 * debugDraw.Render(gfx, camera);
 *
 * // フレーム終了時にクリア
 * debugDraw.Clear();
 * @endcode
 *
 * @note デバッグビルド(_DEBUG定義時)のみ使用を推奨
 * @note C++14準拠
 *
 * @author 山内陽
 */
class DebugDraw {
  public:
    /**
     * @struct Line
     * @brief 線分の定義(開始点、終了点、色)
     */
    struct Line {
        DirectX::XMFLOAT3 start; ///< 線の開始点
        DirectX::XMFLOAT3 end;   ///< 線の終了点
        DirectX::XMFLOAT3 color; ///< 線の色(RGB: 0.0～1.0)
    };

    /**
     * @struct Statistics
     * @brief 描画統計情報
     */
    struct Statistics {
        size_t linesDrawn = 0;      ///< 描画された線の数
        size_t linesDropped = 0;    ///< 容量不足で破棄された線の数
        size_t totalLinesAdded = 0; ///< 追加された線の総数
        size_t peakLineCount = 0;   ///< ピーク時の線の数

        void Reset() {
            linesDrawn = 0;
            linesDropped = 0;
            totalLinesAdded = 0;
            peakLineCount = 0;
        }
    };

    /**
     * @brief デフォルトコンストラクタ
     */
    DebugDraw() = default;

    /**
     * @brief コピー禁止
     */
    DebugDraw(const DebugDraw &) = delete;
    DebugDraw &operator=(const DebugDraw &) = delete;

    /**
     * @brief ムーブ許可
*/
    DebugDraw(DebugDraw &&other) noexcept
        : vs_(std::move(other.vs_)), ps_(std::move(other.ps_)), layout_(std::move(other.layout_)), cb_(std::move(other.cb_)), vb_(std::move(other.vb_)), lines_(std::move(other.lines_)), maxLines_(other.maxLines_), isShutdown_(other.isShutdown_), initialized_(other.initialized_), stats_(other.stats_) {
        other.initialized_ = false;
        other.isShutdown_ = true;
    }

    DebugDraw &operator=(DebugDraw &&other) noexcept {
        if (this != &other) {
            Shutdown();

            vs_ = std::move(other.vs_);
            ps_ = std::move(other.ps_);
            layout_ = std::move(other.layout_);
            cb_ = std::move(other.cb_);
            vb_ = std::move(other.vb_);
            lines_ = std::move(other.lines_);
            maxLines_ = other.maxLines_;
            isShutdown_ = other.isShutdown_;
            initialized_ = other.initialized_;
            stats_ = other.stats_;

            other.initialized_ = false;
            other.isShutdown_ = true;
        }
        return *this;
    }

    /**
   * @brief 初期化
     * @param[in] gfx グラフィックスデバイス
     * @param[in] maxLines 最大線数(デフォルト: 10000)
     * @return bool 初期化が成功した場合は true
     *
* @details
     * シェーダーのコンパイル、パイプラインステートの作成、
     * 動的頂点バッファの作成を行います。
     */
    bool Init(GfxDevice &gfx, size_t maxLines = 10000) {
        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw::Init() 開始 (最大線数: " + std::to_string(maxLines) + ")");

        // 既に初期化済みの再初期化に対応
        if (initialized_) {
            DEBUGLOG_WARNING("DebugDraw::Init() - 既に初期化されています。再初期化します。");
            Shutdown();
        }

        maxLines_ = maxLines;
        lines_.reserve(maxLines_);

        // シェーダーのコンパイル
        if (!CompileShaders(gfx)) {
            DEBUGLOG_ERROR("[DebugDraw] シェーダーのコンパイルに失敗しました");
            return false;
        }

        // パイプラインステートの作成
        if (!CreatePipelineState(gfx)) {
            DEBUGLOG_ERROR("[DebugDraw] パイプラインステートの作成に失敗しました");
            return false;
        }

        // 定数バッファの作成
        if (!CreateConstantBuffer(gfx)) {
            DEBUGLOG_ERROR("[DebugDraw] 定数バッファの作成に失敗しました");
            return false;
        }

        // 動的頂点バッファの作成
        if (!CreateVertexBuffer(gfx)) {
            DEBUGLOG_ERROR("[DebugDraw] 頂点バッファの作成に失敗しました");
            return false;
        }

        initialized_ = true;
        isShutdown_ = false;
        stats_.Reset();

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw::Init() 正常に完了");
        return true;
    }

    /**
     * @brief 初期化状態を確認
     * @return bool 初期化済みの場合は true
     */
    bool IsInitialized() const {
        return initialized_;
    }

    /**
     * @brief 線を追加
     * @param[in] start 線の開始点
     * @param[in] end 線の終了点
     * @param[in] color 線の色(RGB: 0.0～1.0)
     *
     * @details
     * 描画する線をリストに追加します。
     * 実際の描画はRender()呼び出し時に行われます。
     *
     * @par 使用例
     * @code
     * // 赤い線を描画
     * debugDraw.AddLine(
     *     DirectX::XMFLOAT3{0, 0, 0},
     *     DirectX::XMFLOAT3{10, 0, 0},
     *     DirectX::XMFLOAT3{1, 0, 0}
     * );
     * @endcode
     */
    void AddLine(const DirectX::XMFLOAT3 &start, const DirectX::XMFLOAT3 &end, const DirectX::XMFLOAT3 &color) {
        if (!initialized_) {
            DEBUGLOG_WARNING("[DebugDraw] 初期化されていません。AddLine()を無視します。");
            return;
        }

        if (lines_.size() >= maxLines_) {
            stats_.linesDropped++;

            // 最初の警告のみログ出力
            static bool warningShown = false;
            if (!warningShown) {
                DEBUGLOG_WARNING("[DebugDraw] 線の上限に到達 (" + std::to_string(maxLines_) + ")。これ以降の線は破棄されます。");
                warningShown = true;
            }
            return;
        }

        lines_.push_back({start, end, color});
        stats_.totalLinesAdded++;
        stats_.peakLineCount = (std::max)(stats_.peakLineCount, lines_.size());
    }

    /**
     * @brief ボックスを描画
     * @param[in] center ボックスの中心
   * @param[in] halfExtents ボックスの半分のサイズ
     * @param[in] color ボックスの色
     *
     * @details
     * ワイヤーフレームのボックスを描画します。
     * 当たり判定の可視化に便利です。
     */
    void DrawBox(const DirectX::XMFLOAT3 &center, const DirectX::XMFLOAT3 &halfExtents, const DirectX::XMFLOAT3 &color) {
        float minX = center.x - halfExtents.x;
        float maxX = center.x + halfExtents.x;
        float minY = center.y - halfExtents.y;
        float maxY = center.y + halfExtents.y;
        float minZ = center.z - halfExtents.z;
        float maxZ = center.z + halfExtents.z;

        // Front face
        AddLine({minX, minY, minZ}, {maxX, minY, minZ}, color);
        AddLine({maxX, minY, minZ}, {maxX, maxY, minZ}, color);
        AddLine({maxX, maxY, minZ}, {minX, maxY, minZ}, color);
        AddLine({minX, maxY, minZ}, {minX, minY, minZ}, color);

        // Back face
        AddLine({minX, minY, maxZ}, {maxX, minY, maxZ}, color);
        AddLine({maxX, minY, maxZ}, {maxX, maxY, maxZ}, color);
        AddLine({maxX, maxY, maxZ}, {minX, maxY, maxZ}, color);
        AddLine({minX, maxY, maxZ}, {minX, minY, maxZ}, color);

        // Connections
        AddLine({minX, minY, minZ}, {minX, minY, maxZ}, color);
        AddLine({maxX, minY, minZ}, {maxX, minY, maxZ}, color);
        AddLine({maxX, maxY, minZ}, {maxX, maxY, maxZ}, color);
        AddLine({minX, maxY, minZ}, {minX, maxY, maxZ}, color);
    }

    /**
     * @brief 球を描画
     * @param[in] center 球の中心
 * @param[in] radius 球の半径
* @param[in] color 球の色
     * @param[in] segments 分割数(デフォルト: 16)
     *
     * @details
   * ワイヤーフレームの球を描画します。
   */
    void DrawSphere(const DirectX::XMFLOAT3 &center, float radius, const DirectX::XMFLOAT3 &color, int segments = 16) {
        const float angleStep = DirectX::XM_2PI / segments;

        // XY平面の円
        for (int i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);
            DirectX::XMFLOAT3 p1{
                center.x + radius * cosf(angle1),
                center.y + radius * sinf(angle1),
                center.z};
            DirectX::XMFLOAT3 p2{
                center.x + radius * cosf(angle2),
                center.y + radius * sinf(angle2),
                center.z};
            AddLine(p1, p2, color);
        }

        // XZ平面の円
        for (int i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);
            DirectX::XMFLOAT3 p1{
                center.x + radius * cosf(angle1),
                center.y,
                center.z + radius * sinf(angle1)};
            DirectX::XMFLOAT3 p2{
                center.x + radius * cosf(angle2),
                center.y,
                center.z + radius * sinf(angle2)};
            AddLine(p1, p2, color);
        }

        // YZ平面の円
        for (int i = 0; i < segments; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);
            DirectX::XMFLOAT3 p1{
                center.x,
                center.y + radius * cosf(angle1),
                center.z + radius * sinf(angle1)};
            DirectX::XMFLOAT3 p2{
                center.x,
                center.y + radius * cosf(angle2),
                center.z + radius * sinf(angle2)};
            AddLine(p1, p2, color);
        }
    }

    /**
     * @brief グリッドを描画
     * @param[in] size グリッドのサイズ(全体の幅と奥行き)
     * @param[in] divisions 分割数(何本の線を引くか)
     * @param[in] color グリッドの色(デフォルト: 灰色)
     * @param[in] yOffset Y位置のオフセット(デフォルト: -0.01f、座標軸との重なりを防ぐ)
     *
     * @details
     * X-Z平面にグリッドを描画します。
     * Y=yOffsetの平面に水平なグリッドが表示されます。
     *
* @par 使用例
     * @code
     * // 20x20のグリッドを20本の線で描画
  * debugDraw.DrawGrid(20.0f, 20);
     * @endcode
     */
    void DrawGrid(float size = 10.0f, int divisions = 10, const DirectX::XMFLOAT3 &color = {0.5f, 0.5f, 0.5f}, float yOffset = -0.01f) {
        if (divisions <= 0) {
            DEBUGLOG_WARNING("[DebugDraw] DrawGrid: divisionsは正の値である必要があります");
            return;
        }

        float step = size / divisions;
        float halfSize = size * 0.5f;

        // X-Z平面のグリッド（yOffsetを適用）
        for (int i = 0; i <= divisions; ++i) {
            float pos = -halfSize + i * step;

            // Z軸に平行な線(X方向に並ぶ)
            AddLine(
                DirectX::XMFLOAT3{-halfSize, yOffset, pos},
                DirectX::XMFLOAT3{halfSize, yOffset, pos},
                color);

            // X軸に平行な線(Z方向に並ぶ)
            AddLine(
                DirectX::XMFLOAT3{pos, yOffset, -halfSize},
                DirectX::XMFLOAT3{pos, yOffset, halfSize},
                color);
        }
    }

    /**
   * @brief 座標軸を描画
     * @param[in] length 軸の長さ
   *
     * @details
   * X軸(赤)、Y軸(緑)、Z軸(青)を原点から描画します。
     * 3D空間の方向を確認するのに便利です。
     *
     * @par 使用例
     * @code
     * // 5単位の長さの座標軸を描画
     * debugDraw.DrawAxes(5.0f);
     * @endcode
*/
    void DrawAxes(float length = 500.0f) {
        if (length <= 0.0f) {
            DEBUGLOG_WARNING("[DebugDraw] DrawAxes: lengthは正の値である必要があります");
            return;
        }

        // X軸(明るい赤)
        AddLine(
            DirectX::XMFLOAT3{0, 0, 0},
            DirectX::XMFLOAT3{length, 0, 0},
            DirectX::XMFLOAT3{1, 0.2f, 0.2f});

        // Y軸(明るい緑)
        AddLine(
            DirectX::XMFLOAT3{0, 0, 0},
            DirectX::XMFLOAT3{0, length, 0},
            DirectX::XMFLOAT3{0.2f, 1, 0.2f});

        // Z軸(明るい青)
        AddLine(
            DirectX::XMFLOAT3{0, 0, 0},
            DirectX::XMFLOAT3{0, 0, length},
            DirectX::XMFLOAT3{0.3f, 0.3f, 1});
    }

    /**
     * @brief すべての線を描画
   * @param[in] gfx グラフィックスデバイス
   * @param[in] cam カメラ
     *
  * @details
     * AddLine()やDrawGrid()などで追加されたすべての線を描画します。
     * カメラのView・Projection行列を使用してワールド空間から画面空間に変換します。
     */
    void Render(GfxDevice &gfx, const Camera &cam) {
        if (!initialized_) {
            DEBUGLOG_WARNING("[DebugDraw] 初期化されていません。Render()を無視します。");
            return;
        }

        if (lines_.empty()) {
            return;
        }

        // 頂点データを構築
        std::vector<Vertex> vertices;
        vertices.reserve(lines_.size() * 2);

        for (const auto &line : lines_) {
            vertices.push_back({line.start, line.color});
            vertices.push_back({line.end, line.color});
        }

        // 頂点バッファを更新
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = gfx.Ctx()->Map(vb_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] 頂点バッファのマップ失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return;
        }

        memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        gfx.Ctx()->Unmap(vb_.Get(), 0);

        // パイプライン設定
        gfx.Ctx()->IASetInputLayout(layout_.Get());
        gfx.Ctx()->VSSetShader(vs_.Get(), nullptr, 0);
        gfx.Ctx()->PSSetShader(ps_.Get(), nullptr, 0);
        gfx.Ctx()->VSSetConstantBuffers(0, 1, cb_.GetAddressOf());

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        gfx.Ctx()->IASetVertexBuffers(0, 1, vb_.GetAddressOf(), &stride, &offset);
        gfx.Ctx()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        // 定数バッファ更新(ワールド行列は単位行列)
        DirectX::XMMATRIX VP = DirectX::XMMatrixTranspose(cam.View * cam.Proj);
        gfx.Ctx()->UpdateSubresource(cb_.Get(), 0, nullptr, &VP, 0, 0);

        // 描画
        gfx.Ctx()->Draw(static_cast<UINT>(vertices.size()), 0);

        stats_.linesDrawn = lines_.size();
    }

    /**
     * @brief フレーム終了時にクリア
     *
 * @details
     * 蓄積された線データをクリアします。
     * 毎フレーム呼び出す必要があります。
  *
     * @par 使用例
     * @code
     * while (running) {
     *     // 線を追加
     *     debugDraw.DrawGrid(20.0f, 20);
     *
     *   // 描画
     *     debugDraw.Render(gfx, camera);
     *
     *   // フレーム終了時にクリア
     *     debugDraw.Clear();
     * }
     * @endcode
   */
    void Clear() {
        lines_.clear();
    }

    /**
     * @brief 統計情報を取得
     * @return const Statistics& 統計情報への参照
   */
    const Statistics &GetStatistics() const {
        return stats_;
    }

    /**
     * @brief 統計情報をリセット
     */
    void ResetStatistics() {
        stats_.Reset();
    }

    /**
     * @brief 現在の線の数を取得
     * @return size_t 現在の線の数
  */
    size_t GetLineCount() const {
        return lines_.size();
    }

    /**
     * @brief 最大線数を取得
     * @return size_t 最大線数
     */
    size_t GetMaxLines() const {
        return maxLines_;
    }

    /**
     * @brief デストラクタ
     */
    ~DebugDraw() {
        if (!isShutdown_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw::~DebugDraw() - デストラクタ呼び出し");
            DEBUGLOG_WARNING("[DebugDraw] Shutdown()が明示的に呼ばれていません。デストラクタで自動クリーンアップします。");
        }
        Shutdown();
    }

    /**
     * @brief リソースの明示的解放
     */
    void Shutdown() {
        if (isShutdown_)
            return; // 冪等性

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw::Shutdown() - リソースを解放中");

        // 統計情報をログ出力
        if (stats_.totalLinesAdded > 0) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics,
                              "DebugDraw統計: 総追加線数=" + std::to_string(stats_.totalLinesAdded) +
                                  ", ピーク線数=" + std::to_string(stats_.peakLineCount) +
                                  ", 破棄線数=" + std::to_string(stats_.linesDropped));
        }

        int releasedCount = 0;

        if (vs_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "頂点シェーダーを解放 (DebugDraw)");
            vs_.Reset();
            releasedCount++;
        }

        if (ps_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "ピクセルシェーダーを解放 (DebugDraw)");
            ps_.Reset();
            releasedCount++;
        }

        if (layout_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "入力レイアウトを解放 (DebugDraw)");
            layout_.Reset();
            releasedCount++;
        }

        if (cb_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "定数バッファを解放 (DebugDraw)");
            cb_.Reset();
            releasedCount++;
        }

        if (vb_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "頂点バッファを解放 (DebugDraw, 最大線数: " + std::to_string(maxLines_) + ")");
            vb_.Reset();
            releasedCount++;
        }

        lines_.clear();
        lines_.shrink_to_fit();

        isShutdown_ = true;
        initialized_ = false;

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw::Shutdown() 完了 (解放リソース数: " + std::to_string(releasedCount) + ")");
    }

  private:
    /**
   * @struct Vertex
 * @brief 頂点データ(位置と色)
     */
    struct Vertex {
        DirectX::XMFLOAT3 pos; ///< 位置
        DirectX::XMFLOAT3 col; ///< 色
    };

    /**
     * @brief シェーダーのコンパイル
     */
    bool CompileShaders(GfxDevice &gfx) {
        const char *VS = R"(
            cbuffer CB : register(b0) { float4x4 gVP; };
struct VSIn { float3 pos : POSITION; float3 col : COLOR; };
            struct VSOut { float4 pos : SV_POSITION; float3 col : COLOR; };
            VSOut main(VSIn i){
  VSOut o;
     o.pos = mul(float4(i.pos, 1), gVP);
          o.col = i.col;
        return o;
        }
        )";

        const char *PS = R"(
          struct VSOut { float4 pos : SV_POSITION; float3 col : COLOR; };
        float4 main(VSOut i) : SV_Target { return float4(i.col, 1); }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> vsb, psb, err;

        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(ENABLE_SHADER_DEBUG) && ENABLE_SHADER_DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        HRESULT hr = D3DCompile(VS, strlen(VS), nullptr, nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, vsb.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                std::string errorMsg(static_cast<const char *>(err->GetBufferPointer()), err->GetBufferSize());
                DEBUGLOG_ERROR("[DebugDraw] 頂点シェーダーのコンパイル失敗: " + errorMsg);
            } else {
                DEBUGLOG_ERROR("[DebugDraw] 頂点シェーダーのコンパイル失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            }
            return false;
        }

        hr = D3DCompile(PS, strlen(PS), nullptr, nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            if (err) {
                std::string errorMsg(static_cast<const char *>(err->GetBufferPointer()), err->GetBufferSize());
                DEBUGLOG_ERROR("[DebugDraw] ピクセルシェーダーのコンパイル失敗: " + errorMsg);
            } else {
                DEBUGLOG_ERROR("[DebugDraw] ピクセルシェーダーのコンパイル失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            }
            return false;
        }

        hr = gfx.Dev()->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, vs_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] 頂点シェーダーの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        hr = gfx.Dev()->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, ps_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] ピクセルシェーダーの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        // 入力レイアウト
        D3D11_INPUT_ELEMENT_DESC il[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        hr = gfx.Dev()->CreateInputLayout(il, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), layout_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] 入力レイアウトの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw: シェーダーとレイアウトを作成");
        return true;
    }

    /**
     * @brief パイプラインステートの作成
     */
    bool CreatePipelineState(GfxDevice &gfx) {
        // 現在の実装では追加のパイプラインステートは不要
        // 将来的にラスタライザーステートやブレンドステートを追加可能
        return true;
    }

    /**
     * @brief 定数バッファの作成
  */
    bool CreateConstantBuffer(GfxDevice &gfx) {
        D3D11_BUFFER_DESC cbd{};
        cbd.ByteWidth = sizeof(DirectX::XMMATRIX);
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = 0;
        cbd.MiscFlags = 0;
        cbd.StructureByteStride = 0;

        HRESULT hr = gfx.Dev()->CreateBuffer(&cbd, nullptr, cb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] 定数バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw: 定数バッファを作成");
        return true;
    }

    /**
     * @brief 頂点バッファの作成
     */
    bool CreateVertexBuffer(GfxDevice &gfx) {
        D3D11_BUFFER_DESC vbd{};
        vbd.ByteWidth = static_cast<UINT>(maxLines_ * 2 * sizeof(Vertex)); // 1線分 = 2頂点
        vbd.Usage = D3D11_USAGE_DYNAMIC;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbd.MiscFlags = 0;
        vbd.StructureByteStride = 0;

        HRESULT hr = gfx.Dev()->CreateBuffer(&vbd, nullptr, vb_.GetAddressOf());
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("[DebugDraw] 頂点バッファの作成失敗 (HRESULT: 0x" + std::to_string(hr) + ")");
            return false;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "DebugDraw: 頂点バッファを作成 (最大線数: " + std::to_string(maxLines_) + ")");
        return true;
    }

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;    ///< 頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;     ///< ピクセルシェーダー
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_; ///< 入力レイアウト
    Microsoft::WRL::ComPtr<ID3D11Buffer> cb_;          ///< 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> vb_;          ///< 頂点バッファ

    std::vector<Line> lines_;  ///< 描画する線のリスト
    size_t maxLines_ = 10000;  ///< 最大線数
    bool isShutdown_ = false;  ///< シャットダウンフラグ
    bool initialized_ = false; ///< 初期化済みフラグ
    Statistics stats_;         ///< 統計情報
};
