/**
 * @file TransformHierarchy.h
 * @brief Transform階層構造管理コンポーネント
 * @author 山内陽
 * @date 2025
 * @version 6.0
 *
 * @details
 * エンティティ間の親子関係を管理し、階層的なTransform変換を実現します。
 */
#pragma once

#include "ecs/Entity.h"
#include "components/Component.h"
#include <vector>
#include <optional>

/**
 * @struct TransformHierarchy
 * @brief エンティティ間の親子関係を管理するコンポーネント
 *
 * @details
 * このコンポーネントを使用することで、3Dオブジェクトの階層構造を構築できます。
 * 親エンティティが移動・回転・スケールすると、子エンティティも同じ変換を受けます。
 *
 * ### 親子関係の特徴:
 * - 親の変換（位置・回転・スケール）は子に自動的に適用されます
 * - 子のローカル座標は親の座標系を基準とします
 * - 1つの親は複数の子を持つことができます
 * - 子は1つの親のみを持つことができます
 *
 * @par 使用例(基本)
 * @code
 * // 親エンティティを作成
 * Entity parent = world.Create()
 *     .With<Transform>(DirectX::XMFLOAT3{0, 0, 0})
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{1, 0, 0})
 * .With<TransformHierarchy>()
 *     .Build();
 *
 * // 子エンティティを作成
 * Entity child = world.Create()
 * .With<Transform>(DirectX::XMFLOAT3{2, 0, 0})  // 親から右に2単位
 *     .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 *     .With<TransformHierarchy>()
 *     .Build();
 *
 * // 親子関係を設定
 * auto* childHierarchy = world.TryGet<TransformHierarchy>(child);
 * auto* parentHierarchy = world.TryGet<TransformHierarchy>(parent);
 * if (childHierarchy && parentHierarchy) {
 *     childHierarchy->SetParent(parent);
 *     parentHierarchy->AddChild(child);
 * }
 * @endcode
 *
 * @par 使用例(複数の子)
 * @code
 * // 1つの親に複数の子を追加
 * for (int i = 0; i < 3; ++i) {
 *     Entity child = world.Create()
 *         .With<Transform>(DirectX::XMFLOAT3{static_cast<float>(i) * 2.0f, 0, 0})
 *         .With<TransformHierarchy>()
 *       .Build();
 *
 *     auto* childHierarchy = world.TryGet<TransformHierarchy>(child);
 *     auto* parentHierarchy = world.TryGet<TransformHierarchy>(parent);
 *     if (childHierarchy && parentHierarchy) {
 *         childHierarchy->SetParent(parent);
 *parentHierarchy->AddChild(child);
 *     }
 * }
 * @endcode
 */
struct TransformHierarchy : IComponent {
    /**
     * @brief デフォルトコンストラクタ
     */
    TransformHierarchy() = default;

    /**
     * @brief 親エンティティを設定
     * @param parentEntity 親エンティティ
   */
    void SetParent(Entity parentEntity) {
        parent = parentEntity;
    }

    /**
     * @brief 親エンティティを取得
     * @return 親エンティティ（存在しない場合はstd::nullopt）
     */
    std::optional<Entity> GetParent() const {
        return parent;
    }

    /**
     * @brief 親を持つかどうかを確認
     * @return true: 親が存在する, false: 親が存在しない
     */
    bool HasParent() const {
        return parent.has_value();
    }

    /**
     * @brief 親子関係を解除
     */
    void ClearParent() {
        parent = std::nullopt;
    }

    /**
     * @brief 子エンティティを追加
     * @param childEntity 子エンティティ
     */
    void AddChild(Entity childEntity) {
        for (const auto &child : children) {
            if (child.id == childEntity.id) {
                return;
            }
        }
        children.push_back(childEntity);
    }

    /**
     * @brief 子エンティティを削除
     * @param childEntity 削除する子エンティティ
     */
    void RemoveChild(Entity childEntity) {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                           [&](const Entity &e) { return e.id == childEntity.id; }),
            children.end());
    }

    /**
     * @brief すべての子エンティティを取得
     * @return 子エンティティのリスト
     */
    const std::vector<Entity> &GetChildren() const {
        return children;
    }

    /**
     * @brief 子を持つかどうかを確認
     * @return true: 子が存在する, false: 子が存在しない
     */
    bool HasChildren() const {
        return !children.empty();
    }

    /**
     * @brief 子の数を取得
     * @return 子エンティティの数
     */
    size_t GetChildCount() const {
        return children.size();
    }

  private:
    std::optional<Entity> parent;
    std::vector<Entity> children;
};
