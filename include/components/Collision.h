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

        // Box vs TriPrism (use AABB test then refine plane)
        if (auto *boxA = w.TryGet<CollisionBox>(a)) {
            if (auto *triB = w.TryGet<CollisionRightIsoTriPrism>(b)) {
                CollisionBox triBBox(triB->GetScaledSize(*transformB), triB->offset);
                auto info = CheckAABB_AABB(*transformA, *boxA, *transformB, triBBox, a, b);
                if (info) { info->isColliding = RefineTriPrism(boxA, transformA, triB, transformB, *info, true); if(info->isColliding) return info; }
            }
        }
        if (auto *triA = w.TryGet<CollisionRightIsoTriPrism>(a)) {
            if (auto *boxB = w.TryGet<CollisionBox>(b)) {
                CollisionBox triABBox(triA->GetScaledSize(*transformA), triA->offset);
                auto info = CheckAABB_AABB(*transformA, triABBox, *transformB, *boxB, a, b);
                if (info) { info->isColliding = RefineTriPrism(boxB, transformB, triA, transformA, *info, false); if(info->isColliding) return info; }
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
                if (info) {
                    // 法線を反転してエンティティ順序に合わせる
                    info->normal.x = -info->normal.x;
                    info->normal.y = -info->normal.y;
                    info->normal.z = -info->normal.z;
                    std::swap(info->entityA, info->entityB);
                }
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

    static std::optional<CollisionInfo> CheckSphere_TriPrism(
        const Transform &tSphere, const CollisionSphere &sphere,
        const Transform &tTri, const CollisionRightIsoTriPrism &tri,
        Entity entitySphere, Entity entityTri) {
        using namespace DirectX;

        XMFLOAT3 sphereCenter = sphere.GetWorldCenter(tSphere);
        float radius = sphere.GetScaledRadius(tSphere);

        XMFLOAT3 triCenter = tri.GetWorldCenter(tTri);
        XMFLOAT3 triSize = tri.GetScaledSize(tTri);
        XMFLOAT3 triHalf{triSize.x * 0.5f, triSize.y * 0.5f, triSize.z * 0.5f};

        XMVECTOR rotQuat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(tTri.rotation.x),
            XMConvertToRadians(tTri.rotation.y),
            XMConvertToRadians(tTri.rotation.z));
        XMVECTOR invRot = XMQuaternionConjugate(rotQuat);

        // ワールド -> 三角柱ローカル
        XMVECTOR relWorld = XMVectorSubtract(XMLoadFloat3(&sphereCenter), XMLoadFloat3(&triCenter));
        XMVECTOR relLocal = XMVector3Rotate(relWorld, invRot);
        XMFLOAT3 rel;
        XMStoreFloat3(&rel, relLocal);

        // 三角柱の形状をローカル座標で定義
        // mainDiagonalXZ=true:  残る頂点 (+hx,-hz), (-hx,+hz), (+hx,+hz) - カット角(-hx,-hz)
        // mainDiagonalXZ=false: 残る頂点 (-hx,-hz), (+hx,-hz), (+hx,+hz) - カット角(-hx,+hz)
        
        // 斜め平面の法線（ローカル座標、正規化済み）
        float diagSign = tri.mainDiagonalXZ ? 1.0f : -1.0f;
        float invSqrt2 = 0.70710678f;
        XMFLOAT3 planeNormal{invSqrt2, 0.0f, diagSign * invSqrt2};
        
        // 球体の中心から斜め平面への符号付き距離
        float planeDist = rel.x * planeNormal.x + rel.z * planeNormal.z;
        
        // 球体の中心が平面の外側（カット側）にある場合は早期リターン
        if (planeDist < -radius) {
            return std::nullopt;
        }

        // Y軸方向のクランプ
        float clampedY = std::clamp(rel.y, -triHalf.y, triHalf.y);
        
        // XZ平面で三角柱の断面（三角形）への最近傍点を計算
        // 三角形の3頂点（XZ平面）
        XMFLOAT2 v0, v1, v2;
        if (tri.mainDiagonalXZ) {
            // カット角(-hx,-hz), 残る頂点: (+hx,-hz), (-hx,+hz), (+hx,+hz)
            v0 = {+triHalf.x, -triHalf.z};
            v1 = {-triHalf.x, +triHalf.z};
            v2 = {+triHalf.x, +triHalf.z};
        } else {
            // カット角(-hx,+hz), 残る頂点: (-hx,-hz), (+hx,-hz), (+hx,+hz)
            v0 = {-triHalf.x, -triHalf.z};
            v1 = {+triHalf.x, -triHalf.z};
            v2 = {+triHalf.x, +triHalf.z};
        }
        
        XMFLOAT2 p{rel.x, rel.z};
        
        // 三角形への最近傍点を計算（2D）
        auto closestPointOnTriangle2D = [&](XMFLOAT2 point, XMFLOAT2 a, XMFLOAT2 b, XMFLOAT2 c) -> XMFLOAT2 {
            // エッジベクトル
            XMFLOAT2 ab{b.x - a.x, b.y - a.y};
            XMFLOAT2 ac{c.x - a.x, c.y - a.y};
            XMFLOAT2 ap{point.x - a.x, point.y - a.y};
            
            float d1 = ab.x * ap.x + ab.y * ap.y;
            float d2 = ac.x * ap.x + ac.y * ap.y;
            if (d1 <= 0.0f && d2 <= 0.0f) return a; // 頂点Aが最近傍
            
            XMFLOAT2 bp{point.x - b.x, point.y - b.y};
            float d3 = ab.x * bp.x + ab.y * bp.y;
            float d4 = ac.x * bp.x + ac.y * bp.y;
            if (d3 >= 0.0f && d4 <= d3) return b; // 頂点Bが最近傍
            
            float vc = d1 * d4 - d3 * d2;
            if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
                float v = d1 / (d1 - d3);
                return {a.x + v * ab.x, a.y + v * ab.y}; // エッジAB上
            }
            
            XMFLOAT2 cp{point.x - c.x, point.y - c.y};
            float d5 = ab.x * cp.x + ab.y * cp.y;
            float d6 = ac.x * cp.x + ac.y * cp.y;
            if (d6 >= 0.0f && d5 <= d6) return c; // 頂点Cが最近傍
            
            float vb = d5 * d2 - d1 * d6;
            if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
                float w = d2 / (d2 - d6);
                return {a.x + w * ac.x, a.y + w * ac.y}; // エッジAC上
            }
            
            float va = d3 * d6 - d5 * d4;
            if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
                float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                return {b.x + w * (c.x - b.x), b.y + w * (c.y - b.y)}; // エッジBC上
            }
            
            // 点は三角形の内部にある
            float denom = 1.0f / (va + vb + vc);
            float vw = vb * denom;
            float ww = vc * denom;
            return {a.x + ab.x * vw + ac.x * ww, a.y + ab.y * vw + ac.y * ww};
        };
        
        XMFLOAT2 closestXZ = closestPointOnTriangle2D(p, v0, v1, v2);
        
        // 最近傍点（ローカル座標）
        XMFLOAT3 clamped{closestXZ.x, clampedY, closestXZ.y};

        // ローカル最近傍点をワールドへ戻す
        XMVECTOR closestLocal = XMLoadFloat3(&clamped);
        XMVECTOR closestWorld = XMVectorAdd(XMVector3Rotate(closestLocal, rotQuat), XMLoadFloat3(&triCenter));

        XMVECTOR diff = XMVectorSubtract(closestWorld, XMLoadFloat3(&sphereCenter));
        float distSq = XMVectorGetX(XMVector3LengthSq(diff));

        #if defined(_DEBUG)
        float planeDist2 = (rel.x + diagSign * rel.z) * invSqrt2;
        if (std::abs(planeDist2) < 0.5f) {
            std::string result = distSq <= radius * radius ? "HIT" : "MISS";
            DEBUGLOG("[TriPrism2] Player(" + std::to_string(entitySphere.id) + ") vs Wall(" + std::to_string(entityTri.id) + 
                     ") rel=(" + std::to_string(rel.x) + "," + std::to_string(rel.z) + 
                     ") closest=(" + std::to_string(closestXZ.x) + "," + std::to_string(closestXZ.y) + 
                     ") dist=" + std::to_string(std::sqrt(distSq)) + " -> " + result);
        }
        #endif

        if (distSq <= radius * radius) {
            CollisionInfo info;
            info.entityA = entitySphere;
            info.entityB = entityTri;
            info.isColliding = true;

            float dist = std::sqrt(std::max(distSq, 0.0f));
            info.penetrationDepth = radius - dist;

            if (dist > 1e-6f) {
                XMVECTOR n = XMVector3Normalize(diff); // tri -> sphere
                XMStoreFloat3(&info.normal, XMVectorNegate(n)); // sphere -> tri
                XMStoreFloat3(&info.contactPoint, closestWorld);
            } else {
                info.normal = tri.mainDiagonalXZ ? XMFLOAT3{-0.707f, 0.0f, -0.707f} : XMFLOAT3{-0.707f, 0.0f, 0.707f};
                info.contactPoint = sphereCenter;
            }

            return info;
        }

        return std::nullopt;
    }


    static bool RefineTriPrism(const CollisionBox* box, const Transform* tBox, const CollisionRightIsoTriPrism* tri, const Transform* tTri, CollisionInfo &info, bool triIsB){
        if(!box||!tBox||!tri||!tTri) return false;
        using namespace DirectX;

        XMFLOAT3 triCenter = tri->GetWorldCenter(*tTri);
        XMFLOAT3 triSize = tri->GetScaledSize(*tTri);
        XMFLOAT3 triHalf{triSize.x * 0.5f, triSize.y * 0.5f, triSize.z * 0.5f};
        XMVECTOR rotQuat = DirectX::XMQuaternionRotationRollPitchYaw(
            DirectX::XMConvertToRadians(tTri->rotation.x),
            DirectX::XMConvertToRadians(tTri->rotation.y),
            DirectX::XMConvertToRadians(tTri->rotation.z));

        XMFLOAT3 boxCenter = box->GetWorldCenter(*tBox);
        XMFLOAT3 boxSize = box->GetScaledSize(*tBox);
        XMFLOAT3 boxHalf{boxSize.x * 0.5f, boxSize.y * 0.5f, boxSize.z * 0.5f};

        // 三角柱ローカル空間での中心位置
        XMFLOAT3 rel{
            boxCenter.x - triCenter.x,
            boxCenter.y - triCenter.y,
            boxCenter.z - triCenter.z
        };

        XMVECTOR baseN = tri->mainDiagonalXZ
            ? XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f)
            : XMVectorSet(1.0f, 0.0f, -1.0f, 0.0f);
        XMVECTOR n = XMVector3Normalize(XMVector3Rotate(baseN, rotQuat));

        // 平面法線方向への射影
        XMFLOAT3 absN;
        XMStoreFloat3(&absN, XMVectorAbs(n));

        // 回転後の各軸を取得（Y軸回転対応）
        XMVECTOR axisX = XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotQuat);
        XMVECTOR axisY = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotQuat);
        XMVECTOR axisZ = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotQuat);

        float projCenter = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&rel), n));
        float boxExtent = absN.x * boxHalf.x + absN.y * boxHalf.y + absN.z * boxHalf.z;
        float triExtent =
            std::abs(XMVectorGetX(XMVector3Dot(axisX, n))) * triHalf.x +
            std::abs(XMVectorGetX(XMVector3Dot(axisY, n))) * triHalf.y +
            std::abs(XMVectorGetX(XMVector3Dot(axisZ, n))) * triHalf.z;

        float minProjBox = projCenter - boxExtent;
        float maxProjBox = projCenter + boxExtent;

        // 箱が平面の反対側に完全にある、または三角柱の正面範囲を超える場合は非衝突
        if (maxProjBox < 0.0f) {
            return false;
        }
        if (minProjBox > triExtent) {
            return false;
        }

        // 平面へのめり込み量を優先して法線を更新
        float planePenetration = std::max(0.0f, -minProjBox);
        float capPenetration = std::max(0.0f, maxProjBox - triExtent);
        float penetration = planePenetration > 0.0f ? planePenetration : capPenetration;
        if (penetration > 0.0f) {
            info.penetrationDepth = std::max(info.penetrationDepth, penetration);
        }

        XMFLOAT3 worldNormal;
        XMStoreFloat3(&worldNormal, n);
        if (!triIsB) {
            worldNormal.x = -worldNormal.x;
            worldNormal.y = -worldNormal.y;
            worldNormal.z = -worldNormal.z;
        }
        info.normal = worldNormal;

        // 接触点は法線方向にクランプした最近傍点を使用
        float clampedProj = std::clamp(projCenter, 0.0f, triExtent);
        XMVECTOR relVec = XMLoadFloat3(&rel);
        XMVECTOR correctedRel = XMVectorSubtract(relVec, XMVectorScale(n, projCenter - clampedProj));
        XMVECTOR contactWorld = XMVectorAdd(correctedRel, XMLoadFloat3(&triCenter));
        XMStoreFloat3(&info.contactPoint, contactWorld);

        return true;
    }
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
