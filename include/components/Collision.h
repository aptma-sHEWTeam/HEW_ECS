/**************************************************/ /**
 * @file Collision.h
 * @brief 当たり判定システム - ECS設計準拠版
 * @author 立山悠朔・上手涼太郎・山内陽
 * @date 2025
 * @version 2.1
 *
 * @details
 * Entity Component System (ECS) アーキテクチャに準拠した当たり判定システムです。
 *
 * ### 主な特徴:
 * - データとロジックの完全分離
 * - std::variantによる型安全な形状管理
 * - Broad-phase/Narrow-phase分離による最適化
 * - 衝突イベントコールバックシステム
 * - **OnEnter/OnStay/OnExit イベントシステム** (v2.1 NEW!)
 * - 拡張可能な設計
 *
 * @par 使用例(基本)
 * @code
 * // AABB衝突判定を持つエンティティ
 * Entity player = world.Create()
 *   .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<CollisionBox>(DirectX::XMFLOAT3{1, 2, 1})
 *     .With<PlayerTag>()
 *     .Build();
 * @endcode
 *
 * @par 使用例(OnEnterイベント) **NEW!**
 * @code
 * // 衝突イベントハンドラーを実装
 * struct PlayerCollisionHandler : ICollisionHandler {
 *     void OnCollisionEnter(World& w, Entity self, Entity other, const CollisionInfo& info) override {
 *     DEBUGLOG("衝突開始!");
 *     }
 *
 *  void OnCollisionStay(World& w, Entity self, Entity other, const CollisionInfo& info) override {
 *       // 衝突中の処理
 *     }
 *
 *     void OnCollisionExit(World& w, Entity self, Entity other) override {
 *         DEBUGLOG("衝突終了!");
 *     }
 * };
 *
 * // プレイヤーに追加
 * Entity player = world.Create()
 *     .With<Transform>()
 *     .With<CollisionBox>()
 *     .With<PlayerCollisionHandler>()
 *     .Build();
 * @endcode
 */
#pragma once

#include "components/Component.h"
#include "components/Transform.h"
#include "ecs/Entity.h"
#include "ecs/World.h"
#include "app/DebugLog.h"
#include "app/BuildConfig.h"
#include <DirectXMath.h>
#include <variant>
#include <optional>
#include <vector>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <typeindex>
#include <cmath>
#include <limits>
#include <DirectXCollision.h> // For BoundingBox

#include "systems/SpatialHashGrid.h" // Added for spatial hash grid
#if ENABLE_DEBUG_VISUALS
#include "graphics/DebugDraw.h"
#include "app/ServiceLocator.h"
#endif

// ========================================================
// 前方宣言
// ========================================================

struct CollisionBox;
struct CollisionSphere;
struct CollisionCapsule;
struct CollisionInfo;
struct ICollisionHandler;

/**
 * @struct StaticCollider
 * @brief 動かないオブジェクトであることを示すタグ
 * @details このタグ同士の衝突判定はスキップされます
 */
struct StaticCollider : IComponent {};

// ========================================================
// 衝突形状の定義 (データコンポーネント)
// ========================================================

/**
 * @struct CollisionBox
 * @brief AABB (Axis-Aligned Bounding Box) 軸平行境界ボックス
 *
 * @details
 * 回転しない矩形の当たり判定です。計算が高速で、
 * 多くのゲームオブジェクトに適しています。
 *
 * @note サイズは**半分のサイズ**ではなく**全体のサイズ**です
 */
struct CollisionBox : IComponent {
    DirectX::XMFLOAT3 size{1.0f, 1.0f, 1.0f};   ///< ボックスのサイズ(幅, 高さ, 奥行き)
    DirectX::XMFLOAT3 offset{0.0f, 0.0f, 0.0f}; ///< Transformからのオフセット

    /**
     * @brief コンストラクタ
     * @param[in] boxSize ボックスのサイズ
     * @param[in] centerOffset 中心オフセット
     */
    explicit CollisionBox(
        const DirectX::XMFLOAT3 &boxSize = {1.0f, 1.0f, 1.0f},
        const DirectX::XMFLOAT3 &centerOffset = {0.0f, 0.0f, 0.0f})
        : size(boxSize), offset(centerOffset) {}

    /**
     * @brief 均等サイズのボックスを作成
     * @param[in] uniformSize すべての軸で同じサイズ
     */
    explicit CollisionBox(float uniformSize)
        : size{uniformSize, uniformSize, uniformSize}, offset{0.0f, 0.0f, 0.0f} {}

    /**
     * @brief ワールド空間での中心座標を取得
     * @param[in] transform エンティティのTransform
   * @return DirectX::XMFLOAT3 ワールド座標での中心
     */
    inline DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const noexcept {
        return {
            transform.position.x + offset.x,
            transform.position.y + offset.y,
            transform.position.z + offset.z
        };
    }

    /**
     * @brief スケールを適用したサイズを取得
     * @param[in] transform エンティティのTransform
     * @return DirectX::XMFLOAT3 スケール適用後のサイズ
     */
    DirectX::XMFLOAT3 GetScaledSize(const Transform &transform) const {
        return {
            size.x * transform.scale.x,
            size.y * transform.scale.y,
            size.z * transform.scale.z};
    }
};

/**
 * @struct CollisionSphere
 * @brief 球体の当たり判定
 *
 * @details
 * すべての方向で均等な当たり判定です。
 * 回転に影響されず、計算が非常に高速です。
 */
struct CollisionSphere : IComponent {
    float radius{0.5f};                         ///< 球の半径
    DirectX::XMFLOAT3 offset{0.0f, 0.0f, 0.0f}; ///< Transformからのオフセット

    /**
     * @brief コンストラクタ
     * @param[in] r 半径
     * @param[in] centerOffset 中心オフセット
     */
    explicit CollisionSphere(
        float r = 0.5f,
        const DirectX::XMFLOAT3 &centerOffset = {0.0f, 0.0f, 0.0f})
        : radius(r), offset(centerOffset) {}

    /**
     * @brief ワールド空間での中心座標を取得
     */
    inline DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const noexcept {
        return {
            transform.position.x + offset.x,
            transform.position.y + offset.y,
            transform.position.z + offset.z
        };
    }

    /**
     * @brief スケールを適用した半径を取得
   * @note 非均等スケールの場合、最大値を使用
     */
    float GetScaledRadius(const Transform &transform) const {
        float maxScale = std::max({transform.scale.x, transform.scale.y, transform.scale.z});
        return radius * maxScale;
    }
};

/**
 * @struct CollisionCapsule
 * @brief カプセル形状の当たり判定
 *
 * @details
 * 2つの球を線分で結んだ形状です。
 * キャラクターの当たり判定に適しています。
 */
struct CollisionCapsule : IComponent {
    float radius{0.5f};                         ///< カプセルの半径
    float height{2.0f};                         ///< カプセルの高さ(中心間距離)
    DirectX::XMFLOAT3 offset{0.0f, 0.0f, 0.0f}; ///< Transformからのオフセット

    /**
     * @brief コンストラクタ
     * @param[in] r 半径
     * @param[in] h 高さ
     * @param[in] centerOffset 中心オフセット
*/
    explicit CollisionCapsule(
        float r = 0.5f,
        float h = 2.0f,
        const DirectX::XMFLOAT3 &centerOffset = {0.0f, 0.0f, 0.0f})
        : radius(r), height(h), offset(centerOffset) {}

    /**
     * @brief ワールド空間での中心座標を取得
     */
    inline DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const noexcept {
        return {
            transform.position.x + offset.x,
            transform.position.y + offset.y,
            transform.position.z + offset.z
        };
    }

    /**
     * @brief カプセルの上端点を取得
     */
    DirectX::XMFLOAT3 GetTopPoint(const Transform &transform) const {
        auto center = GetWorldCenter(transform);
        center.y += height * 0.5f * transform.scale.y;
        return center;
    }

    /**
     * @brief カプセルの下端点を取得
     */
    DirectX::XMFLOAT3 GetBottomPoint(const Transform &transform) const {
        auto center = GetWorldCenter(transform);
        center.y -= height * 0.5f * transform.scale.y;
        return center;
    }
};

/**
 * @struct CollisionRightIsoTriPrism
 * @brief 直方体半分の直角二等辺三角柱 (軸アライン想定)
 *
 * @details
 * XZ平面での対称性を持ち、上下いずれかの面でカットされた形状です。
 * 主に床や壁の一部として使用されることを想定しています。
 */
struct CollisionRightIsoTriPrism : IComponent { // 立方体半分の直角二等辺三角柱 (軸アライン想定)
    DirectX::XMFLOAT3 size{1.0f,1.0f,1.0f}; // 包含する元キューブサイズ
    DirectX::XMFLOAT3 offset{0,0,0};
    bool mainDiagonalXZ{true}; // true:x+z>=0面で切断 / false:x - z >=0
    explicit CollisionRightIsoTriPrism(const DirectX::XMFLOAT3 &boxSize={1,1,1}, const DirectX::XMFLOAT3 &centerOffset={0,0,0}, bool diag=true)
        : size(boxSize), offset(centerOffset), mainDiagonalXZ(diag) {}
    inline DirectX::XMFLOAT3 GetWorldCenter(const Transform &t) const noexcept {
        return {
            t.position.x + offset.x,
            t.position.y + offset.y,
            t.position.z + offset.z
        };
    }
    DirectX::XMFLOAT3 GetScaledSize(const Transform &t) const { return {size.x*t.scale.x,size.y*t.scale.y,size.z*t.scale.z}; }
};

// ========================================================
// 衝突情報
// ========================================================

/**
 * @struct CollisionInfo
 * @brief 衝突情報を格納する構造体
 */
struct CollisionInfo {
    Entity entityA;                          ///< 衝突したエンティティA
    Entity entityB;                          ///< 衝突したエンティティB
    DirectX::XMFLOAT3 contactPoint{0, 0, 0}; ///< 接触点
    DirectX::XMFLOAT3 normal{0, 1, 0};       ///< 衝突法線(A -> B方向)
    float penetrationDepth{0.0f};            ///< 侵入深度
    bool isColliding{false};                 ///< 衝突しているか

    /**
     * @brief 衝突情報をログ出力
 */
    void DebugPrint() const {
        if (isColliding) {
            DEBUGLOG("Collision: Entity " + std::to_string(entityA.id) +
                     " <-> Entity " + std::to_string(entityB.id) +
                     " | Depth: " + std::to_string(penetrationDepth));
        }
    }
};

// ========================================================
// 衝突イベントハンドラーインターフェース (NEW!)
// ========================================================

/**
 * @struct ICollisionHandler
 * @brief 衝突イベントを受け取るインターフェース
 *
 * @details
 * このインターフェースを継承したコンポーネントを持つエンティティは、
 * 衝突時に自動的にイベントハンドラーが呼び出されます。
 *
 * ### イベントの種類:
 * - **OnCollisionEnter**: 衝突が開始した瞬間(1フレームのみ)
 * - **OnCollisionStay**: 衝突中(毎フレーム)
 * - **OnCollisionExit**: 衝突が終了した瞬間(1フレームのみ)
 *
 * @par 使用例
 * @code
 * struct PlayerCollisionHandler : ICollisionHandler {
 *     void OnCollisionEnter(World& w, Entity self, Entity other, const CollisionInfo& info) override {
 *         if (w.Has<EnemyTag>(other)) {
 *             DEBUGLOG("敵に衝突!");
 *     auto* health = w.TryGet<Health>(self);
 *             if (health) health->TakeDamage(10.0f);
 *}
 *     }
 *
 *     void OnCollisionStay(World& w, Entity self, Entity other, const CollisionInfo& info) override {
 *         // 継続的なダメージなど
 *     }
 *
 *     void OnCollisionExit(World& w, Entity self, Entity other) override {
 *         DEBUGLOG("衝突終了");
 *     }
 * };
 *
 * // エンティティに追加
 * Entity player = world.Create()
 *     .With<Transform>()
 *     .With<CollisionBox>()
 *     .With<PlayerCollisionHandler>()
 *     .Build();
 * @endcode
 *
 * @note IComponentを継承しているため、通常のコンポーネントとして追加できます
 * @author 山内陽
 */
struct ICollisionHandler : IComponent {
    /**
  * @brief 衝突が開始した瞬間に呼ばれる
     * @param[in,out] w ワールド参照
     * @param[in] self このハンドラーを持つエンティティ
     * @param[in] other 衝突相手のエンティティ
  * @param[in] info 衝突情報
     */
    virtual void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {}

    /**
 * @brief 衝突中に毎フレーム呼ばれる
     * @param[in,out] w ワールド参照
     * @param[in] self このハンドラーを持つエンティティ
 * @param[in] other 衝突相手のエンティティ
     * @param[in] info 衝突情報
     */
    virtual void OnCollisionStay(World &w, Entity self, Entity other, const CollisionInfo &info) {}

    /**
  * @brief 衝突が終了した瞬間に呼ばれる
     * @param[in,out] w ワールド参照
     * @param[in] self このハンドラーを持つエンティティ
     * @param[in] other 衝突相手のエンティティ
     */
    virtual void OnCollisionExit(World &w, Entity self, Entity other) {}
};

// ========================================================
// 衝突検出システム (Behaviour)
// ========================================================

/**
 * @struct CollisionDetectionSystem
 * @brief 衝突検出を行うシステムコンポーネント
 *
 * @details
 * Worldに1つだけ配置し、すべての衝突判定を管理します。
 * v2.1から**OnEnter/OnStay/OnExit**イベントシステムを搭載。
 */
struct CollisionDetectionSystem : Behaviour {
    CollisionDetectionSystem(float cellSize = 10.0f) : m_grid(cellSize) {}

    using CollisionCallback = std::function<void(Entity, Entity, const CollisionInfo &)>;

    void OnCollision(CollisionCallback callback) {
        collisionCallbacks_.push_back(callback);
    }

        void OnUpdate(World &w, Entity self, float dt) override {

            // 前フレームの衝突情報をスワップ

            previousCollisions_.swap(currentCollisions_);

            currentCollisions_.clear();

            collisionCount_ = 0;

    

            m_grid.Clear();

    

            using namespace DirectX;

#if ENABLE_DEBUG_VISUALS
            DebugDraw* debugDraw = ServiceLocator::TryGet<DebugDraw>();
            if (debugDraw && !debugDraw->IsInitialized()) {
                debugDraw = nullptr;
            }
            const DirectX::XMFLOAT3 colorBox{0.0f, 0.6f, 1.0f};
            const DirectX::XMFLOAT3 colorSphere{0.0f, 1.0f, 0.4f};
            const DirectX::XMFLOAT3 colorCapsule{1.0f, 0.2f, 1.0f};
            const DirectX::XMFLOAT3 colorTri{1.0f, 0.85f, 0.0f};
            const DirectX::XMFLOAT3 colorHit{1.0f, 0.0f, 0.0f};
#endif

    

            struct Collidable {

                Entity entity;

                BoundingBox box;

            };

            std::vector<Collidable> collidables;

            collidables.reserve(128); // 事前確保

    

            w.ForEach<CollisionBox, Transform>([&](Entity e, CollisionBox& box, Transform& t) {

                XMFLOAT3 center = box.GetWorldCenter(t);

                XMFLOAT3 scaledSize = box.GetScaledSize(t);

                XMFLOAT3 halfExtents = {scaledSize.x * 0.5f, scaledSize.y * 0.5f, scaledSize.z * 0.5f};

                BoundingBox bb(center, halfExtents);

                collidables.push_back({e, bb});

                m_grid.Insert(e, bb);

#if ENABLE_DEBUG_VISUALS
                if (debugDraw) {
                    debugDraw->DrawBox(center, halfExtents, colorBox);
                }
#endif

            });

    

            w.ForEach<CollisionSphere, Transform>([&](Entity e, CollisionSphere& sphere, Transform& t) {

                XMFLOAT3 center = sphere.GetWorldCenter(t);

                float radius = sphere.GetScaledRadius(t);

                XMFLOAT3 extents = {radius, radius, radius};

                BoundingBox bb(center, extents);

                collidables.push_back({e, bb});

                m_grid.Insert(e, bb);

#if ENABLE_DEBUG_VISUALS
                if (debugDraw) {
                    debugDraw->DrawSphere(center, radius, colorSphere);
                }
#endif

            });

    

            w.ForEach<CollisionCapsule, Transform>([&](Entity e, CollisionCapsule& capsule, Transform& t) {

                XMFLOAT3 top = capsule.GetTopPoint(t);

                XMFLOAT3 bottom = capsule.GetBottomPoint(t);

                float radius = capsule.radius * std::max({t.scale.x, t.scale.y, t.scale.z});

    

                float minX = std::min(top.x, bottom.x) - radius;

                float minY = std::min(top.y, bottom.y) - radius;

                float minZ = std::min(top.z, bottom.z) - radius;

                float maxX = std::max(top.x, bottom.x) + radius;

                float maxY = std::max(top.y, bottom.y) + radius;

                float maxZ = std::max(top.z, bottom.z) + radius;

                

                XMFLOAT3 center = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f};

                XMFLOAT3 extents = {(maxX - minX) * 0.5f, (maxY - minY) * 0.5f, (maxZ - minZ) * 0.5f};

                BoundingBox bb(center, extents);

                collidables.push_back({e, bb});

                m_grid.Insert(e, bb);

#if ENABLE_DEBUG_VISUALS
                if (debugDraw) {
                    debugDraw->AddLine(bottom, top, colorCapsule);
                    const int segments = 12;
                    for (int i = 0; i < segments; ++i) {
                        float a0 = DirectX::XM_2PI * (float)i / segments;
                        float a1 = DirectX::XM_2PI * (float)(i + 1) / segments;
                        float c0 = cosf(a0), s0 = sinf(a0);
                        float c1 = cosf(a1), s1 = sinf(a1);
                        DirectX::XMFLOAT3 p0{bottom.x + radius * c0, bottom.y, bottom.z + radius * s0};
                        DirectX::XMFLOAT3 p1{bottom.x + radius * c1, bottom.y, bottom.z + radius * s1};
                        DirectX::XMFLOAT3 q0{top.x + radius * c0, top.y, top.z + radius * s0};
                        DirectX::XMFLOAT3 q1{top.x + radius * c1, top.y, top.z + radius * s1};
                        debugDraw->AddLine(p0, p1, colorCapsule);
                        debugDraw->AddLine(q0, q1, colorCapsule);
                        debugDraw->AddLine(p0, q0, colorCapsule);
                    }
                }
#endif

            });

            w.ForEach<CollisionRightIsoTriPrism, Transform>([&](Entity e, CollisionRightIsoTriPrism& tri, Transform& t) {
                using namespace DirectX;
                XMFLOAT3 center = tri.GetWorldCenter(t);
                XMFLOAT3 scaled = tri.GetScaledSize(t);
                XMFLOAT3 halfExtents = {scaled.x*0.5f, scaled.y*0.5f, scaled.z*0.5f};
                BoundingBox bb(center, halfExtents); // 包含AABB
                collidables.push_back({e, bb});
                m_grid.Insert(e, bb);

#if ENABLE_DEBUG_VISUALS
                if (debugDraw) {
                    // 外接ボックス全体を水色で描画（当たり判定を持たない可能性がある領域）
                    const DirectX::XMFLOAT3 colorCyan{0.0f, 0.8f, 1.0f};  // 水色
                    debugDraw->DrawBox(center, halfExtents, colorCyan);
                    
                    // 回転を適用するためのクォータニオンを計算
                    XMVECTOR rotQuat = XMQuaternionRotationRollPitchYaw(
                        XMConvertToRadians(t.rotation.x),
                        XMConvertToRadians(t.rotation.y),
                        XMConvertToRadians(t.rotation.z));
                    XMVECTOR centerVec = XMLoadFloat3(&center);
                    
                    // ローカル座標での半分サイズ
                    float hx = halfExtents.x;
                    float hy = halfExtents.y;
                    float hz = halfExtents.z;
                    
                    // ヘルパーラムダ：ローカル座標をワールド座標に変換
                    auto toWorld = [&](float lx, float ly, float lz) -> XMFLOAT3 {
                        XMVECTOR local = XMVectorSet(lx, ly, lz, 0.0f);
                        XMVECTOR rotated = XMVector3Rotate(local, rotQuat);
                        XMVECTOR world = XMVectorAdd(rotated, centerVec);
                        XMFLOAT3 result;
                        XMStoreFloat3(&result, world);
                        return result;
                    };
                    
                    // 三角柱の実際の形状を黄色で描画
                    // 三角柱は5面：底面三角形、上面三角形、3つの側面
                    if (tri.mainDiagonalXZ) {
                        // mainDiag=true: ローカル座標での残る頂点は (+hx,-hz), (-hx,+hz), (+hx,+hz)
                        // 底面の三角形
                        debugDraw->AddLine(toWorld(+hx, -hy, -hz), toWorld(-hx, -hy, +hz), colorTri);  // 斜め辺
                        debugDraw->AddLine(toWorld(-hx, -hy, +hz), toWorld(+hx, -hy, +hz), colorTri);  // 直角辺1
                        debugDraw->AddLine(toWorld(+hx, -hy, +hz), toWorld(+hx, -hy, -hz), colorTri);  // 直角辺2
                        // 上面の三角形
                        debugDraw->AddLine(toWorld(+hx, +hy, -hz), toWorld(-hx, +hy, +hz), colorTri);  // 斜め辺
                        debugDraw->AddLine(toWorld(-hx, +hy, +hz), toWorld(+hx, +hy, +hz), colorTri);  // 直角辺1
                        debugDraw->AddLine(toWorld(+hx, +hy, +hz), toWorld(+hx, +hy, -hz), colorTri);  // 直角辺2
                        // 側面の垂直辺
                        debugDraw->AddLine(toWorld(+hx, -hy, -hz), toWorld(+hx, +hy, -hz), colorTri);
                        debugDraw->AddLine(toWorld(-hx, -hy, +hz), toWorld(-hx, +hy, +hz), colorTri);
                        debugDraw->AddLine(toWorld(+hx, -hy, +hz), toWorld(+hx, +hy, +hz), colorTri);
                    } else {
                        // mainDiag=false: ローカル座標での残る頂点は (-hx,-hz), (+hx,-hz), (+hx,+hz)
                        // 底面の三角形
                        debugDraw->AddLine(toWorld(-hx, -hy, -hz), toWorld(+hx, -hy, +hz), colorTri);  // 斜め辺
                        debugDraw->AddLine(toWorld(-hx, -hy, -hz), toWorld(+hx, -hy, -hz), colorTri);  // 直角辺1
                        debugDraw->AddLine(toWorld(+hx, -hy, -hz), toWorld(+hx, -hy, +hz), colorTri);  // 直角辺2
                        // 上面の三角形
                        debugDraw->AddLine(toWorld(-hx, +hy, -hz), toWorld(+hx, +hy, +hz), colorTri);  // 斜め辺
                        debugDraw->AddLine(toWorld(-hx, +hy, -hz), toWorld(+hx, +hy, -hz), colorTri);  // 直角辺1
                        debugDraw->AddLine(toWorld(+hx, +hy, -hz), toWorld(+hx, +hy, +hz), colorTri);  // 直角辺2
                        // 側面の垂直辺
                        debugDraw->AddLine(toWorld(-hx, -hy, -hz), toWorld(-hx, +hy, -hz), colorTri);
                        debugDraw->AddLine(toWorld(+hx, -hy, -hz), toWorld(+hx, +hy, -hz), colorTri);
                        debugDraw->AddLine(toWorld(+hx, -hy, +hz), toWorld(+hx, +hy, +hz), colorTri);
                    }
                }
#endif
            });

            

            std::unordered_set<uint64_t> uniquePairs; // 重複ペアを避けるため

            std::vector<Entity> potentialColliders;

            potentialColliders.reserve(128); // 衝突候補を保持するのに十分なサイズを事前確保

    

            for (const auto& collidable_a : collidables) {

                Entity e_a = collidable_a.entity;

                const BoundingBox& bb_a = collidable_a.box;

    

                if (!w.IsAlive(e_a)) continue;

    

                // グリッドから衝突候補を取得

                m_grid.Query(bb_a, potentialColliders);

    

                for (Entity e_b : potentialColliders) {

                    if (!w.IsAlive(e_b) || e_a.id >= e_b.id) continue;

                    // 最適化: 両方が静的コライダーならスキップ
                    if (w.Has<StaticCollider>(e_a) && w.Has<StaticCollider>(e_b)) {
                        continue;
                    }

                    uint64_t pairKey = MakePairKey(e_a, e_b);

                    auto collision = CheckCollision(w, e_a, e_b);

                    if (collision && collision->isColliding) {

                        currentCollisions_.insert(pairKey);

                        collisionCount_++;

#if ENABLE_DEBUG_VISUALS
                        if (debugDraw) {
                            DirectX::XMFLOAT3 end{
                                collision->contactPoint.x + collision->normal.x * (0.5f + collision->penetrationDepth),
                                collision->contactPoint.y + collision->normal.y * (0.5f + collision->penetrationDepth),
                                collision->contactPoint.z + collision->normal.z * (0.5f + collision->penetrationDepth)};
                            debugDraw->AddLine(collision->contactPoint, end, colorHit);
                        }
#endif

    

                        // グローバルコールバック実行

                        for (auto &callback : collisionCallbacks_) {

                            callback(e_a, e_b, *collision);

                        }

    

                        // 前フレームに衝突していたか確認

                        bool wasColliding = previousCollisions_.count(pairKey) > 0;

    

                        if (!wasColliding) {

                            //  OnCollisionEnter イベント

                            TriggerCollisionEnter(w, e_a, e_b, *collision);

                            if (enableDebugLog_) {

                                collision->DebugPrint();

                            }

                        } else {

                            // 🔄 OnCollisionStay イベント

                            TriggerCollisionStay(w, e_a, e_b, *collision);

                        }

                    }

                }

            }

    

            // 🔚 OnCollisionExit イベント - 前フレームにあったが今フレームにない衝突

            for (uint64_t pairKey : previousCollisions_) {

                if (currentCollisions_.find(pairKey) == currentCollisions_.end()) {

                    // 衝突が終了した

                    auto [entityA, entityB] = UnpackPairKey(pairKey);

                    if(w.IsAlive(entityA) && w.IsAlive(entityB))

                        TriggerCollisionExit(w, entityA, entityB);

                }

            }

        }

    void SetDebugLog(bool enable) {
        enableDebugLog_ = enable;
    }
    size_t GetCollisionCount() const {
        return collisionCount_;
    }

  private:
    std::vector<CollisionCallback> collisionCallbacks_;
    std::unordered_set<uint64_t> currentCollisions_;
    std::unordered_set<uint64_t> previousCollisions_;
    size_t collisionCount_ = 0;
    bool enableDebugLog_ = false;
    SpatialHashGrid m_grid; // Spatial Hash Grid instance

    static uint64_t MakePairKey(Entity a, Entity b) {
        uint32_t minId = std::min(a.id, b.id);
        uint32_t maxId = std::max(a.id, b.id);
        return (static_cast<uint64_t>(minId) << 32) | maxId;
    }

    static std::pair<Entity, Entity> UnpackPairKey(uint64_t pairKey) {
        uint32_t minId = static_cast<uint32_t>(pairKey >> 32);
        uint32_t maxId = static_cast<uint32_t>(pairKey & 0xFFFFFFFF);
        // 世代番号は不明なので0を使用(IsAliveでチェックされる)
        return {Entity{minId, 0}, Entity{maxId, 0}};
    }

    /**
  * @brief エンティティが持つICollisionHandler派生コンポーネントを取得
     * @details すべての具体的なハンドラー型を試行して、最初に見つかったものを返す
     */
    template <typename HandlerType>
    HandlerType *TryGetHandler(World &w, Entity e) {
        return w.TryGet<HandlerType>(e);
    }

    /**
 * @brief 特定のハンドラー型を試行（テンプレート宣言のみ）
     * @details 実装はCollision.cppで明示的特殊化されます
     */
    template <typename HandlerType, typename Func>
    void TryCallHandler(World &w, Entity e, Func &&func);

    /**
     * @brief すべての既知のハンドラー型を試してコールバックを呼び出す
     * @details 各具象ハンドラー型を明示的に試行します
     */
    void ForEachHandler(World &w, Entity e, const std::function<void(ICollisionHandler *)> &func);

    /**
     * @brief OnCollisionEnter イベントをトリガー
     */
    void TriggerCollisionEnter(World &w, Entity a, Entity b, const CollisionInfo &info);

    /**
     * @brief OnCollisionStay イベントをトリガー
     */
    void TriggerCollisionStay(World &w, Entity a, Entity b, const CollisionInfo &info);

    /**
     * @brief OnCollisionExit イベントをトリガー
     */
    void TriggerCollisionExit(World &w, Entity a, Entity b);

    std::optional<CollisionInfo> CheckCollision(World &w, Entity a, Entity b) {
        auto *transformA = w.TryGet<Transform>(a);
        auto *transformB = w.TryGet<Transform>(b);

        if (!transformA || !transformB) {
            return std::nullopt;
        }

#if defined(_DEBUG)
        RunTriPrismSelfTest();
#endif

        // Box vs Box
        if (auto *boxA = w.TryGet<CollisionBox>(a)) {
            if (auto *boxB = w.TryGet<CollisionBox>(b)) {
                return CheckAABB_AABB(*transformA, *boxA, *transformB, *boxB, a, b);
            }
        }

        // Sphere vs Sphere
        if (auto *sphereA = w.TryGet<CollisionSphere>(a)) {
            if (auto *sphereB = w.TryGet<CollisionSphere>(b)) {
                return CheckSphere_Sphere(*transformA, *sphereA, *transformB, *sphereB, a, b);
            }
        }

        // Box vs Sphere
        if (auto *boxA = w.TryGet<CollisionBox>(a)) {
            if (auto *sphereB = w.TryGet<CollisionSphere>(b)) {
                return CheckAABB_Sphere(*transformA, *boxA, *transformB, *sphereB, a, b);
            }
        }

        // Sphere vs Box
        if (auto *sphereA = w.TryGet<CollisionSphere>(a)) {
            if (auto *boxB = w.TryGet<CollisionBox>(b)) {
                auto result = CheckAABB_Sphere(*transformB, *boxB, *transformA, *sphereA, b, a);
                if (result) {
                    result->normal.x = -result->normal.x;
                    result->normal.y = -result->normal.y;
                    result->normal.z = -result->normal.z;
                    std::swap(result->entityA, result->entityB);
                }
                return result;
            }
        }

        // Box vs TriPrism
        if (auto *boxA = w.TryGet<CollisionBox>(a)) {
            if (auto *triB = w.TryGet<CollisionRightIsoTriPrism>(b)) {
                auto info = CheckBox_TriPrism(*transformA, *boxA, *transformB, *triB, a, b);
                if (info) return info;
            }
        }
        if (auto *triA = w.TryGet<CollisionRightIsoTriPrism>(a)) {
            if (auto *boxB = w.TryGet<CollisionBox>(b)) {
                auto info = CheckBox_TriPrism(*transformB, *boxB, *transformA, *triA, b, a);
                if (info) return info;
            }
        }

        // Sphere vs TriPrism
        if (auto *sphereA = w.TryGet<CollisionSphere>(a)) {
            if (auto *triB = w.TryGet<CollisionRightIsoTriPrism>(b)) {
                auto info = CheckSphere_TriPrism(*transformA, *sphereA, *transformB, *triB, a, b);
                if (info) return info;
            }
        }
        if (auto *triA = w.TryGet<CollisionRightIsoTriPrism>(a)) {
            if (auto *sphereB = w.TryGet<CollisionSphere>(b)) {
                auto info = CheckSphere_TriPrism(*transformB, *sphereB, *transformA, *triA, b, a);
                if (info) return info;
            }
        }
        return std::nullopt;
    }

    static std::optional<CollisionInfo> CheckAABB_AABB(
        const Transform &tA, const CollisionBox &boxA,
        const Transform &tB, const CollisionBox &boxB,
        Entity entityA, Entity entityB) {
        using namespace DirectX;

        auto centerA = boxA.GetWorldCenter(tA);
        auto centerB = boxB.GetWorldCenter(tB);
        auto sizeA = boxA.GetScaledSize(tA);
        auto sizeB = boxB.GetScaledSize(tB);

        float overlapX = (sizeA.x + sizeB.x) * 0.5f - std::abs(centerA.x - centerB.x);
        float overlapY = (sizeA.y + sizeB.y) * 0.5f - std::abs(centerA.y - centerB.y);
        float overlapZ = (sizeA.z + sizeB.z) * 0.5f - std::abs(centerA.z - centerB.z);

        if (overlapX > 0 && overlapY > 0 && overlapZ > 0) {
            CollisionInfo info;
            info.entityA = entityA;
            info.entityB = entityB;
            info.isColliding = true;

            float minOverlap = std::min({overlapX, overlapY, overlapZ});
            info.penetrationDepth = minOverlap;

            XMFLOAT3 direction = {
                centerB.x - centerA.x,
                centerB.y - centerA.y,
                centerB.z - centerA.z};

            if (minOverlap == overlapX) {
                info.normal = {direction.x > 0 ? 1.0f : -1.0f, 0, 0};
                info.contactPoint = {
                    centerA.x + (sizeA.x * 0.5f) * info.normal.x,
                    centerA.y,
                    centerA.z};
            } else if (minOverlap == overlapY) {
                info.normal = {0, direction.y > 0 ? 1.0f : -1.0f, 0};
                info.contactPoint = {
                    centerA.x,
                    centerA.y + (sizeA.y * 0.5f) * info.normal.y,
                    centerA.z};
            } else {
                info.normal = {0, 0, direction.z > 0 ? 1.0f : -1.0f};
                info.contactPoint = {
                    centerA.x,
                    centerA.y,
                    centerA.z + (sizeA.z * 0.5f) * info.normal.z};
            }

            return info;
        }

        return std::nullopt;
    }

    static std::optional<CollisionInfo> CheckSphere_Sphere(
        const Transform &tA, const CollisionSphere &sphereA,
        const Transform &tB, const CollisionSphere &sphereB,
        Entity entityA, Entity entityB) {
        using namespace DirectX;

        auto centerA = sphereA.GetWorldCenter(tA);
        auto centerB = sphereB.GetWorldCenter(tB);
        float radiusA = sphereA.GetScaledRadius(tA);
        float radiusB = sphereB.GetScaledRadius(tB);

        XMVECTOR vA = XMLoadFloat3(&centerA);
        XMVECTOR vB = XMLoadFloat3(&centerB);
        XMVECTOR diff = XMVectorSubtract(vB, vA);

        float distSq = XMVectorGetX(XMVector3LengthSq(diff));
        float radiusSum = radiusA + radiusB;
        float radiusSumSq = radiusSum * radiusSum;

        if (distSq < radiusSumSq) {
            CollisionInfo info;
            info.entityA = entityA;
            info.entityB = entityB;
            info.isColliding = true;

            float dist = std::sqrt(distSq);
            info.penetrationDepth = radiusSum - dist;

            if (dist > 1e-6f) {
                XMVECTOR normalized = XMVector3Normalize(diff);
                XMStoreFloat3(&info.normal, normalized);
                XMVECTOR contact = XMVectorAdd(vA, XMVectorScale(normalized, radiusA));
                XMStoreFloat3(&info.contactPoint, contact);
            } else {
                info.normal = {0, 1, 0};
                info.contactPoint = centerA;
            }

            return info;
        }

        return std::nullopt;
    }

    static std::optional<CollisionInfo> CheckAABB_Sphere(
        const Transform &tBox, const CollisionBox &box,
        const Transform &tSphere, const CollisionSphere &sphere,
        Entity entityBox, Entity entitySphere) {
        using namespace DirectX;

        auto boxCenter = box.GetWorldCenter(tBox);
        auto boxSize = box.GetScaledSize(tBox);
        auto sphereCenter = sphere.GetWorldCenter(tSphere);
        float radius = sphere.GetScaledRadius(tSphere);

        XMFLOAT3 boxMin = {
            boxCenter.x - boxSize.x * 0.5f,
            boxCenter.y - boxSize.y * 0.5f,
            boxCenter.z - boxSize.z * 0.5f};
        XMFLOAT3 boxMax = {
            boxCenter.x + boxSize.x * 0.5f,
            boxCenter.y + boxSize.y * 0.5f,
            boxCenter.z + boxSize.z * 0.5f};

        XMFLOAT3 closestPoint = {
            std::clamp(sphereCenter.x, boxMin.x, boxMax.x),
            std::clamp(sphereCenter.y, boxMin.y, boxMax.y),
            std::clamp(sphereCenter.z, boxMin.z, boxMax.z)};

        XMVECTOR vSphere = XMLoadFloat3(&sphereCenter);
        XMVECTOR vClosest = XMLoadFloat3(&closestPoint);
        XMVECTOR diff = XMVectorSubtract(vSphere, vClosest);
        float distSq = XMVectorGetX(XMVector3LengthSq(diff));

        if (distSq < radius * radius) {
            CollisionInfo info;
            info.entityA = entityBox;
            info.entityB = entitySphere;
            info.isColliding = true;

            float dist = std::sqrt(distSq);
            info.penetrationDepth = radius - dist;
            info.contactPoint = closestPoint;

            if (dist > 1e-6f) {
                XMVECTOR normalized = XMVector3Normalize(diff);
                XMStoreFloat3(&info.normal, normalized);
            } else {
                info.normal = {0, 1, 0};
            }

            return info;
        }

        return std::nullopt;
    }

    struct TriPrismShape {
        DirectX::XMFLOAT3 center;
        DirectX::XMFLOAT3 half;
        bool mainDiagonal;
        DirectX::XMVECTOR rot;
        DirectX::XMVECTOR invRot;
        DirectX::XMFLOAT2 planeNormal; // XZ 平面での正規化済み法線
        float planeOffset;             // n・p = offset が斜面
    };

    static TriPrismShape BuildTriPrismShape(const Transform &tTri, const CollisionRightIsoTriPrism &tri) {
        using namespace DirectX;
        TriPrismShape shape;
        shape.center = tri.GetWorldCenter(tTri);
        auto scaled = tri.GetScaledSize(tTri);
        shape.half = {scaled.x * 0.5f, scaled.y * 0.5f, scaled.z * 0.5f};
        shape.mainDiagonal = tri.mainDiagonalXZ;

        // 斜め壁はYaw回転のみを想定（壁配置の使用実態に合わせる）
        float yawRad = XMConvertToRadians(tTri.rotation.y);
        shape.rot = XMQuaternionRotationRollPitchYaw(0.0f, yawRad, 0.0f);
        shape.invRot = XMQuaternionConjugate(shape.rot);

        float nX = shape.half.z;
        float nZ = tri.mainDiagonalXZ ? shape.half.x : -shape.half.x;
        float len = std::sqrt(std::max(nX * nX + nZ * nZ, 1e-6f));
        shape.planeNormal = {nX / len, nZ / len};

        DirectX::XMFLOAT2 cut = tri.mainDiagonalXZ ? DirectX::XMFLOAT2{-shape.half.x, -shape.half.z}
                                                   : DirectX::XMFLOAT2{-shape.half.x, shape.half.z};
        shape.planeOffset = shape.planeNormal.x * cut.x + shape.planeNormal.y * cut.y;
        return shape;
    }

    static DirectX::XMFLOAT3 ToLocal(const TriPrismShape &shape, const DirectX::XMFLOAT3 &world) {
        using namespace DirectX;
        XMVECTOR rel = XMVectorSubtract(XMLoadFloat3(&world), XMLoadFloat3(&shape.center));
        XMVECTOR local = XMVector3Rotate(rel, shape.invRot);
        DirectX::XMFLOAT3 out;
        XMStoreFloat3(&out, local);
        return out;
    }

    static DirectX::XMVECTOR ToWorld(const TriPrismShape &shape, const DirectX::XMFLOAT3 &local) {
        using namespace DirectX;
        return XMVectorAdd(XMVector3Rotate(XMLoadFloat3(&local), shape.rot), XMLoadFloat3(&shape.center));
    }

    static float Dot2(const DirectX::XMFLOAT2 &a, const DirectX::XMFLOAT2 &b) {
        return a.x * b.x + a.y * b.y;
    }

    static std::vector<DirectX::XMFLOAT2> ClipPolygonAgainstPlane(
        const std::vector<DirectX::XMFLOAT2> &poly,
        const DirectX::XMFLOAT2 &normal,
        float offset) {
        std::vector<DirectX::XMFLOAT2> result;
        if (poly.empty()) return result;

        auto lerp = [](const DirectX::XMFLOAT2 &a, const DirectX::XMFLOAT2 &b, float t) {
            return DirectX::XMFLOAT2{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        };

        const float epsilon = 1e-6f;
        for (size_t i = 0; i < poly.size(); ++i) {
            const auto &s = poly[i];
            const auto &e = poly[(i + 1) % poly.size()];
            float ds = Dot2(s, normal) - offset;
            float de = Dot2(e, normal) - offset;
            bool sInside = ds >= -epsilon;
            bool eInside = de >= -epsilon;

            if (sInside && eInside) {
                result.push_back(e);
            } else if (sInside && !eInside) {
                float denom = ds - de;
                float t = std::abs(denom) < epsilon ? 0.0f : ds / denom;
                result.push_back(lerp(s, e, t));
            } else if (!sInside && eInside) {
                float denom = ds - de;
                float t = std::abs(denom) < epsilon ? 0.0f : ds / denom;
                result.push_back(lerp(s, e, t));
                result.push_back(e);
            }
        }
        return result;
    }

    static DirectX::XMFLOAT2 ClosestPointOnSegment2D(
        const DirectX::XMFLOAT2 &p,
        const DirectX::XMFLOAT2 &a,
        const DirectX::XMFLOAT2 &b) {
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float lenSq = vx * vx + vy * vy;
        if (lenSq <= 1e-8f) return a;
        float t = ((p.x - a.x) * vx + (p.y - a.y) * vy) / lenSq;
        t = std::clamp(t, 0.0f, 1.0f);
        return {a.x + vx * t, a.y + vy * t};
    }

    static bool IsInsideConvexPolygon(
        const std::vector<DirectX::XMFLOAT2> &poly,
        const DirectX::XMFLOAT2 &p) {
        if (poly.empty()) return false;
        bool hasPos = false;
        bool hasNeg = false;
        for (size_t i = 0; i < poly.size(); ++i) {
            const auto &a = poly[i];
            const auto &b = poly[(i + 1) % poly.size()];
            float cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
            hasPos |= (cross > 0);
            hasNeg |= (cross < 0);
            if (hasPos && hasNeg) return false;
        }
        return true;
    }

    static DirectX::XMFLOAT2 ClosestPointOnPolygon(
        const std::vector<DirectX::XMFLOAT2> &poly,
        const DirectX::XMFLOAT2 &p) {
        if (poly.empty()) return p;
        if (IsInsideConvexPolygon(poly, p)) return p;

        float best = std::numeric_limits<float>::max();
        DirectX::XMFLOAT2 bestPt = poly.front();
        for (size_t i = 0; i < poly.size(); ++i) {
            const auto &a = poly[i];
            const auto &b = poly[(i + 1) % poly.size()];
            auto cand = ClosestPointOnSegment2D(p, a, b);
            float dx = cand.x - p.x;
            float dy = cand.y - p.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < best) {
                best = d2;
                bestPt = cand;
            }
        }
        return bestPt;
    }

    static std::vector<DirectX::XMFLOAT2> BuildTriPrismPolygon(
        const TriPrismShape &shape,
        const DirectX::XMFLOAT3 &expand) {
        float hx = shape.half.x + expand.x;
        float hz = shape.half.z + expand.z;
        std::vector<DirectX::XMFLOAT2> poly = {
            {-hx, -hz},
            {hx, -hz},
            {hx, hz},
            {-hx, hz},
        };

        float planeInflate = shape.planeNormal.x * expand.x + shape.planeNormal.y * expand.z;
        float effectiveOffset = shape.planeOffset - planeInflate;
        return ClipPolygonAgainstPlane(poly, shape.planeNormal, effectiveOffset);
    }

    struct TriPrismQueryResult {
        bool inside = false;
        DirectX::XMFLOAT3 closestLocal{0, 0, 0}; // 最近傍点（ローカル）
        DirectX::XMFLOAT3 normalLocal{0, 1, 0};  // point -> surface 方向
        float minMargin = 0.0f;                  // 内部時の最短余裕距離
        float distance = 0.0f;                   // 最近傍点までの距離
    };

    static TriPrismQueryResult QueryPointToTriPrism(
        const TriPrismShape &shape,
        const DirectX::XMFLOAT3 &pointLocal,
        const DirectX::XMFLOAT3 &expand) {
        using namespace DirectX;
        TriPrismQueryResult result;

        float hx = shape.half.x + expand.x;
        float hy = shape.half.y + expand.y;
        float hz = shape.half.z + expand.z;

        float planeInflate = shape.planeNormal.x * expand.x + shape.planeNormal.y * expand.z;
        float effectivePlaneOffset = shape.planeOffset - planeInflate;
        float planeSigned = shape.planeNormal.x * pointLocal.x + shape.planeNormal.y * pointLocal.z - effectivePlaneOffset;

        bool inside = std::abs(pointLocal.x) <= hx + 1e-6f &&
                      std::abs(pointLocal.y) <= hy + 1e-6f &&
                      std::abs(pointLocal.z) <= hz + 1e-6f &&
                      planeSigned >= -1e-6f;
        result.inside = inside;

        auto polygon = BuildTriPrismPolygon(shape, expand);
        XMFLOAT2 p2{pointLocal.x, pointLocal.z};
        XMFLOAT2 closest2 = polygon.empty()
                                ? XMFLOAT2{std::clamp(pointLocal.x, -hx, hx), std::clamp(pointLocal.z, -hz, hz)}
                                : ClosestPointOnPolygon(polygon, p2);
        float clampedY = std::clamp(pointLocal.y, -hy, hy);
        result.closestLocal = {closest2.x, clampedY, closest2.y};

        XMVECTOR vPoint = XMLoadFloat3(&pointLocal);
        XMVECTOR vClosest = XMLoadFloat3(&result.closestLocal);
        XMVECTOR diff = XMVectorSubtract(vClosest, vPoint);
        result.distance = XMVectorGetX(XMVector3Length(diff));

        if (inside) {
            float marginXPos = hx - pointLocal.x;
            float marginXNeg = hx + pointLocal.x;
            float marginYPos = hy - pointLocal.y;
            float marginYNeg = hy + pointLocal.y;
            float marginZPos = hz - pointLocal.z;
            float marginZNeg = hz + pointLocal.z;
            float marginPlane = planeSigned;

            result.minMargin = marginXPos;
            result.normalLocal = {1.0f, 0.0f, 0.0f};

            auto update = [&](float candidate, const XMFLOAT3 &n) {
                if (candidate < result.minMargin) {
                    result.minMargin = candidate;
                    result.normalLocal = n;
                }
            };

            update(marginXNeg, {-1.0f, 0.0f, 0.0f});
            update(marginYPos, {0.0f, 1.0f, 0.0f});
            update(marginYNeg, {0.0f, -1.0f, 0.0f});
            update(marginZPos, {0.0f, 0.0f, 1.0f});
            update(marginZNeg, {0.0f, 0.0f, -1.0f});
            update(marginPlane, {-shape.planeNormal.x, 0.0f, -shape.planeNormal.y});

            result.closestLocal = {
                pointLocal.x + result.normalLocal.x * result.minMargin,
                pointLocal.y + result.normalLocal.y * result.minMargin,
                pointLocal.z + result.normalLocal.z * result.minMargin};
            result.distance = 0.0f;
        } else if (result.distance > 1e-6f) {
            XMVECTOR n = XMVector3Normalize(diff);
            XMStoreFloat3(&result.normalLocal, n);
        } else {
            result.normalLocal = {-shape.planeNormal.x, 0.0f, -shape.planeNormal.y};
        }

        return result;
    }

    static std::optional<CollisionInfo> CheckSphere_TriPrism(
        const Transform &tSphere, const CollisionSphere &sphere,
        const Transform &tTri, const CollisionRightIsoTriPrism &tri,
        Entity entitySphere, Entity entityTri) {
        using namespace DirectX;

        float radius = sphere.GetScaledRadius(tSphere);
        XMFLOAT3 sphereCenter = sphere.GetWorldCenter(tSphere);

        TriPrismShape shape = BuildTriPrismShape(tTri, tri);
        XMFLOAT3 centerLocal = ToLocal(shape, sphereCenter);

        auto query = QueryPointToTriPrism(shape, centerLocal, {0.0f, 0.0f, 0.0f});
        if (!query.inside && query.distance > radius) {
            return std::nullopt;
        }

        CollisionInfo info;
        info.entityA = entitySphere;
        info.entityB = entityTri;
        info.isColliding = true;

        XMVECTOR normalWorld = XMVector3Rotate(XMLoadFloat3(&query.normalLocal), shape.rot);
        XMStoreFloat3(&info.normal, normalWorld);
        XMVECTOR contactWorld = ToWorld(shape, query.closestLocal);
        XMStoreFloat3(&info.contactPoint, contactWorld);

        if (query.distance > 1e-6f) {
            info.penetrationDepth = radius - query.distance;
        } else {
            info.penetrationDepth = std::max(radius - query.minMargin, 0.0f);
        }

        return info;
    }

    static std::optional<CollisionInfo> CheckBox_TriPrism(
        const Transform &tBox, const CollisionBox &box,
        const Transform &tTri, const CollisionRightIsoTriPrism &tri,
        Entity entityBox, Entity entityTri) {
        using namespace DirectX;

        XMFLOAT3 center = box.GetWorldCenter(tBox);
        auto size = box.GetScaledSize(tBox);
        XMFLOAT3 half{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};

        TriPrismShape shape = BuildTriPrismShape(tTri, tri);
        XMFLOAT3 centerLocal = ToLocal(shape, center);

        auto query = QueryPointToTriPrism(shape, centerLocal, half);
        if (!query.inside) {
            return std::nullopt;
        }

        CollisionInfo info;
        info.entityA = entityBox;
        info.entityB = entityTri;
        info.isColliding = true;
        info.penetrationDepth = std::max(query.minMargin, 0.0f);

        XMVECTOR normalWorld = XMVector3Rotate(XMLoadFloat3(&query.normalLocal), shape.rot);
        XMStoreFloat3(&info.normal, normalWorld);

        float correction =
            std::abs(query.normalLocal.x) * half.x +
            std::abs(query.normalLocal.y) * half.y +
            std::abs(query.normalLocal.z) * half.z;
        DirectX::XMFLOAT3 contactLocal{
            query.closestLocal.x - query.normalLocal.x * correction,
            query.closestLocal.y - query.normalLocal.y * correction,
            query.closestLocal.z - query.normalLocal.z * correction};
        XMVECTOR contactWorld = ToWorld(shape, contactLocal);
        XMStoreFloat3(&info.contactPoint, contactWorld);

        return info;
    }

#if defined(_DEBUG)
    static void RunTriPrismSelfTest() {
        static bool ran = false;
        if (ran) return;
        ran = true;

        Transform triT;
        triT.scale = {1.0f, 1.0f, 1.0f};
        CollisionRightIsoTriPrism tri({2.0f, 2.0f, 2.0f});

        Transform sphereT;
        sphereT.position = {0.8f, 0.0f, 0.8f};
        CollisionSphere sphere(0.3f);

        auto hit = CheckSphere_TriPrism(sphereT, sphere, triT, tri, Entity{1, 0}, Entity{2, 0});
        if (!hit || !hit->isColliding) {
            DEBUGLOG("[CollisionSelfTest] Sphere vs TriPrism expected hit but missed");
        } else if (hit->normal.y > 0.2f) {
            DEBUGLOG("[CollisionSelfTest] Unexpected normal for TriPrism test (expected lateral push)");
        }
    }
#endif
};

// ========================================================
// ユーティリティ: 衝突レイヤーシステム
// ========================================================

/**
 * @struct CollisionLayer
 * @brief 衝突レイヤーを管理するコンポーネント
 */
struct CollisionLayer : IComponent {
    uint8_t layer{0};
    uint8_t mask{0xFF};

    explicit CollisionLayer(uint8_t myLayer = 0, uint8_t collisionMask = 0xFF)
        : layer(myLayer), mask(collisionMask) {}

    bool CanCollideWith(uint8_t otherLayer) const {
        return (mask & (1 << otherLayer)) != 0;
    }
};

// ========================================================
// 作成者: 立山悠朔・上手涼太郎・山内陽
// バージョン: v2.1 - OnEnter/OnStay/OnExit イベントシステム追加
// ========================================================

/**
 * @brief ICollisionHandler 自動登録レジストリ
 */
struct CollisionHandlerRegistry {
    using TryFunc = void (*)(World&, Entity, const std::function<void(ICollisionHandler*)>&);
    static void RegisterType(std::type_index type, TryFunc func);
    static void ForEach(World& w, Entity e, const std::function<void(ICollisionHandler*)>& func);
};

/**
 * @brief 型Tをレジストリへ自動登録するヘルパ
 * @tparam T ICollisionHandler を継承したコンポーネント型
 */
template <typename T>
struct CollisionHandlerAutoRegister {
    CollisionHandlerAutoRegister() {
        CollisionHandlerRegistry::RegisterType(std::type_index(typeid(T)),
            +[](World& w, Entity e, const std::function<void(ICollisionHandler*)>& func){
                if (auto* h = w.TryGet<T>(e)) { func(static_cast<ICollisionHandler*>(h)); }
            }
        );
    }
};

/**
 * @brief ICollisionHandler 実装型の自動登録マクロ
 */
#define REGISTER_COLLISION_HANDLER_TYPE(T) \
    static CollisionHandlerAutoRegister<T> g_collisionHandlerAutoRegister_##T;
