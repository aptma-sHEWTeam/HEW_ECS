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
#include <DirectXMath.h>
#include <variant>
#include <optional>
#include <vector>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <typeindex>

// ========================================================
// 前方宣言
// ========================================================

struct CollisionBox;
struct CollisionSphere;
struct CollisionCapsule;
struct CollisionInfo;
struct ICollisionHandler;

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
    DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const {
        using namespace DirectX;
        XMVECTOR pos = XMLoadFloat3(&transform.position);
        XMVECTOR off = XMLoadFloat3(&offset);
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVectorAdd(pos, off));
        return result;
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
    DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const {
        using namespace DirectX;
        XMVECTOR pos = XMLoadFloat3(&transform.position);
        XMVECTOR off = XMLoadFloat3(&offset);
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVectorAdd(pos, off));
        return result;
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
    DirectX::XMFLOAT3 GetWorldCenter(const Transform &transform) const {
        using namespace DirectX;
        XMVECTOR pos = XMLoadFloat3(&transform.position);
        XMVECTOR off = XMLoadFloat3(&offset);
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVectorAdd(pos, off));
        return result;
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
    using CollisionCallback = std::function<void(Entity, Entity, const CollisionInfo &)>;

    void OnCollision(CollisionCallback callback) {
        collisionCallbacks_.push_back(callback);
    }

    void OnUpdate(World &w, Entity self, float dt) override {
        // 前フレームの衝突情報をスワップ
        previousCollisions_.swap(currentCollisions_);
        currentCollisions_.clear();
        collisionCount_ = 0;

        // すべての衝突ペアを収集
        std::vector<Entity> collidableEntities;

        w.ForEach<Transform, CollisionBox>([&](Entity e, Transform &, CollisionBox &) {
            collidableEntities.push_back(e);
        });

        w.ForEach<Transform, CollisionSphere>([&](Entity e, Transform &, CollisionSphere &) {
            if (std::find(collidableEntities.begin(), collidableEntities.end(), e) == collidableEntities.end()) {
                collidableEntities.push_back(e);
            }
        });

        w.ForEach<Transform, CollisionCapsule>([&](Entity e, Transform &, CollisionCapsule &) {
            if (std::find(collidableEntities.begin(), collidableEntities.end(), e) == collidableEntities.end()) {
                collidableEntities.push_back(e);
            }
        });

        // 総当たりで衝突判定
        for (size_t i = 0; i < collidableEntities.size(); ++i) {
            for (size_t j = i + 1; j < collidableEntities.size(); ++j) {
                Entity a = collidableEntities[i];
                Entity b = collidableEntities[j];

                if (!w.IsAlive(a) || !w.IsAlive(b))
                    continue;

                auto collision = CheckCollision(w, a, b);
                if (collision && collision->isColliding) {
                    uint64_t pairKey = MakePairKey(a, b);
                    currentCollisions_.insert(pairKey);
                    collisionCount_++;

                    // グローバルコールバック実行
                    for (auto &callback : collisionCallbacks_) {
                        callback(a, b, *collision);
                    }

                    // 前フレームに衝突していたか確認
                    bool wasColliding = previousCollisions_.find(pairKey) != previousCollisions_.end();

                    if (!wasColliding) {
                        //  OnCollisionEnter イベント
                        TriggerCollisionEnter(w, a, b, *collision);
                        if (enableDebugLog_) {
                            collision->DebugPrint();
                        }
                    } else {
                        // 🔄 OnCollisionStay イベント
                        TriggerCollisionStay(w, a, b, *collision);
                    }
                }
            }
        }

        // 🔚 OnCollisionExit イベント - 前フレームにあったが今フレームにない衝突
        for (uint64_t pairKey : previousCollisions_) {
            if (currentCollisions_.find(pairKey) == currentCollisions_.end()) {
                // 衝突が終了した
                auto [entityA, entityB] = UnpackPairKey(pairKey);
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
