#pragma once
#include <DirectXMath.h>
#include "graphics/TextureManager.h"

/**
 * @file MeshRenderer.h
 * @brief メッシュ描画コンポーネントの定義
 * @author 山内陽
 * @date 2025
 * @version 6.0
 * 
 * @details
 * このファイルは3Dオブジェクトの「見た目」を制御するコンポーネントを定義します。
 * 複数の基本形状(キューブ、球体、円柱など)をサポートします。
 */

/**
 * @enum MeshType
 * @brief 3Dメッシュの形状タイプ
 * 
 * @details
 * 描画する3D形状の種類を指定します。
 * 
 * @par 利用可能な形状:
 * - Cube: 立方体(デフォルト)
 * - Sphere: 球体
 * - Cylinder: 円柱
 * - Cone: 円錐
 * - Plane: 平面(地面などに使用)
 * - Capsule: カプセル(円柱の両端が半球)
 * 
 * @par 使用例
 * @code
 * MeshRenderer renderer;
 * renderer.meshType = MeshType::Sphere;  // 球体を描画
 * renderer.color = DirectX::XMFLOAT3{1, 0, 0};  // 赤色
 * @endcode
 * 
 * @author 山内陽
 */
enum class MeshType {
    Cube = 0,      ///< 立方体(デフォルト)
    Sphere,        ///< 球体
    Cylinder,      ///< 円柱
    Cone,          ///< 円錐
    Plane,         ///< 平面
    Capsule        ///< カプセル
};

/**
 * @struct MeshRenderer
 * @brief オブジェクトの見た目(色・テクスチャ・形状)を管理するデータコンポーネント
 * 
 * @details
 * このコンポーネントは3Dオブジェクトの描画設定を保持します。
 * 単色表示とテクスチャ表示の両方に対応し、複数の基本形状を選択できます。
 * 
 * ### 描画の仕組み:
 * 1. テクスチャが設定されていない場合: colorで指定した単色で描画
 * 2. テクスチャが設定されている場合: テクスチャ画像で描画(colorは色調として使用)
 * 3. meshTypeで指定された形状で描画
 * 
 * ### UV座標について:
 * UV座標は、テクスチャのどの部分を表示するかを指定します。
 * - uvOffset: テクスチャの開始位置をずらす(アニメーション等に使用)
 * - uvScale: テクスチャの繰り返し回数(タイリング)
 * 
 * @par 使用例(単色キューブ)
 * @code
 * Entity cube = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})  // 赤色
 *     .Build();
 * @endcode
 * 
 * @par 使用例(球体)
 * @code
 * MeshRenderer renderer;
 * renderer.meshType = MeshType::Sphere;
 * renderer.color = DirectX::XMFLOAT3{0, 1, 0};  // 緑色の球
 * 
 * Entity sphere = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(renderer)
 *     .Build();
 * @endcode
 * 
 * @par 使用例(テクスチャ付き円柱)
 * @code
 * MeshRenderer renderer;
 * renderer.meshType = MeshType::Cylinder;
 * renderer.color = DirectX::XMFLOAT3{1, 1, 1};  // 白(テクスチャ本来の色)
 * renderer.texture = texManager.LoadFromFile("wood.png");
 * 
 * Entity pillar = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(renderer)
 *     .Build();
 * @endcode
 * 
 * @note Transformコンポーネントと組み合わせて使用します
 * @see Transform 位置・回転・スケールを管理するコンポーネント
 * @see TextureManager テクスチャ管理クラス
 * @see MeshType 形状タイプの列挙型
 * 
 * @author 山内陽
 */
struct MeshRenderer {
    /**
     * @var meshType
     * @brief 描画する3D形状のタイプ
     * 
     * @details
     * どの形状を描画するかを指定します。
     * 
     * @par 形状の種類:
     * - MeshType::Cube: 立方体(デフォルト)
     * - MeshType::Sphere: 球体
     * - MeshType::Cylinder: 円柱(Y軸方向)
     * - MeshType::Cone: 円錐
     * - MeshType::Plane: 平面(地面など)
     * - MeshType::Capsule: カプセル
     * 
     * @par 使用例(球体に変更)
     * @code
     * auto* renderer = world.TryGet<MeshRenderer>(entity);
     * if (renderer) {
     *     renderer->meshType = MeshType::Sphere;
     * }
     * @endcode
     * 
     * @par 使用例(ランタイムで形状変更)
     * @code
     * // 状態によって形状を切り替え
     * if (isPowerUp) {
     *     renderer->meshType = MeshType::Sphere;  // パワーアップ時は球
     * } else {
     *     renderer->meshType = MeshType::Cube;    // 通常時は立方体
     * }
     * @endcode
     * 
     * @note デフォルトはMeshType::Cube(立方体)
     */
    MeshType meshType = MeshType::Cube;
    
    /**
     * @var color
     * @brief オブジェクトの基本色(RGB: 0.0～1.0)
     * 
     * @details
     * - テクスチャなし: この色で描画
     * - テクスチャあり: テクスチャにこの色を乗算(色調補正)
     * 
     * ### 色の指定方法:
     * - color.x = R(赤) 0.0～1.0
     * - color.y = G(緑) 0.0～1.0
     * - color.z = B(青) 0.0～1.0
     * 
     * @par よく使う色:
     * @code
     * DirectX::XMFLOAT3{1, 0, 0}  // 赤
     * DirectX::XMFLOAT3{0, 1, 0}  // 緑
     * DirectX::XMFLOAT3{0, 0, 1}  // 青
     * DirectX::XMFLOAT3{1, 1, 0}  // 黄
     * DirectX::XMFLOAT3{1, 0, 1}  // マゼンタ
     * DirectX::XMFLOAT3{0, 1, 1}  // シアン
     * DirectX::XMFLOAT3{1, 1, 1}  // 白
     * DirectX::XMFLOAT3{0, 0, 0}  // 黒
     * DirectX::XMFLOAT3{0.5, 0.5, 0.5}  // 灰色
     * @endcode
     * 
     * @par 使用例(テクスチャの色調整)
     * @code
     * // テクスチャに赤みを足す
     * renderer.color = DirectX::XMFLOAT3{1.5f, 1.0f, 1.0f};
     * 
     * // テクスチャを暗くする
     * renderer.color = DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f};
     * @endcode
     * 
     * @note デフォルトは白
     */
    DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
    
    /**
     * @var texture
     * @brief 表面に貼り付けるテクスチャ画像のハンドル
     * 
     * @details
     * TextureManagerから取得したテクスチャハンドルを設定します。
     * INVALID_TEXTUREの場合、テクスチャは使用されず単色で描画されます。
     * 
     * @par 使用例(基本)
     * @code
     * auto* renderer = world.TryGet<MeshRenderer>(entity);
     * if (renderer) {
     *     renderer->texture = texManager.LoadFromFile("brick.png");
     * }
     * @endcode
     * 
     * @par 使用例(テクスチャの変更)
     * @code
     * // 実行時にテクスチャを切り替え
     * if (isDamaged) {
     *     renderer->texture = texManager.LoadFromFile("damaged.png");
     * } else {
     *     renderer->texture = texManager.LoadFromFile("normal.png");
     * }
     * @endcode
     * 
     * @note デフォルトはINVALID_TEXTURE(テクスチャなし)
     * @see TextureManager::LoadFromFile テクスチャ読み込み
     */
    TextureManager::TextureHandle texture = TextureManager::INVALID_TEXTURE;
    
    /**
     * @var uvOffset
     * @brief UV座標のオフセット(テクスチャ位置のずらし)
     * 
     * @details
     * テクスチャの表示開始位置をずらします。
     * アニメーションやスクロール効果に使用します。
     * 
     * - uvOffset.x: 横方向のオフセット(0.0～1.0で1周)
     * - uvOffset.y: 縦方向のオフセット(0.0～1.0で1周)
     * 
     * @par 使用例(横スクロール)
     * @code
     * // 毎フレーム少しずつ横にずらす
     * renderer->uvOffset.x += 0.5f * dt;  // 毎秒0.5ずつスクロール
     * @endcode
     * 
     * @par 使用例(水の流れ)
     * @code
     * // 水のテクスチャをゆっくり流す
     * renderer->uvOffset.x += 0.1f * dt;
     * renderer->uvOffset.y += 0.05f * dt;
     * @endcode
     * 
     * @par 使用例(スプライトアニメーション)
     * @code
     * // 4フレームのスプライトシートを切り替え
     * int frame = (int)(time * 10) % 4;  // 0,1,2,3を繰り返し
     * renderer->uvOffset.x = frame * 0.25f;  // 各フレームは幅0.25
     * renderer->uvScale.x = 0.25f;           // 1/4だけ表示
     * @endcode
     * 
     * @note デフォルトは{0.0, 0.0}(ずらしなし)
     */
    DirectX::XMFLOAT2 uvOffset{ 0.0f, 0.0f };
    
    /**
     * @var uvScale
     * @brief UV座標のスケール(テクスチャの繰り返し)
     * 
     * @details
     * テクスチャの繰り返し回数を指定します(タイリング)。
     * 
     * - uvScale.x: 横方向の繰り返し回数
     * - uvScale.y: 縦方向の繰り返し回数
     * 
     * @par 使用例(2x2タイリング)
     * @code
     * renderer->uvScale = DirectX::XMFLOAT2{2.0f, 2.0f};  // 2x2で繰り返し
     * @endcode
     * 
     * @par 使用例(レンガ壁)
     * @code
     * // 横に3回、縦に2回繰り返し
     * renderer->uvScale = DirectX::XMFLOAT2{3.0f, 2.0f};
     * @endcode
     * 
     * @par 使用例(ミラー効果)
     * @code
     * // 負の値で反転
     * renderer->uvScale = DirectX::XMFLOAT2{-1.0f, 1.0f};  // 横反転
     * @endcode
     * 
     * @note 1.0が標準(繰り返しなし)
     * @note デフォルトは{1.0, 1.0}
     */
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };
};
