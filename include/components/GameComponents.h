#pragma once

#include "components/Component.h"
#include "components/Animator.h"
#include <DirectXMath.h>

/**
 * @struct Health
 * @brief 体力管理コンポーネント
 *
 * @details
 * エンティティの現在の体力と最大体力を管理します。
 * ダメージを受けたり、回復したりする機能を提供します。
 *
 * @par 使用例
 * @code
 * Health hp;
 * hp.current = 50.0f;
 * hp.max = 100.0f;
 * world.Add<Health>(entity, hp);
 * 
 * // ダメージを受ける
 * auto* health = world.TryGet<Health>(entity);
 * if (health) {
 *     health->TakeDamage(10.0f);
 *     if (health->IsDead()) {
 *         // 死亡処理
 *     }
 * }
 * @endcode
 *
 * @author 山内陽
 */
struct Health : IComponent {
    float current = 100.0f;  ///< 現在の体力
    float max = 100.0f;      ///< 最大体力

    /**
     * @brief ダメージを受ける
     * @param[in] damage ダメージ量
     * 
     * @details
     * 体力を減少させます。0未満にはなりません。
     */
    void TakeDamage(float damage) {
        current -= damage;
        if (current < 0.0f) current = 0.0f;
    }

    /**
     * @brief 回復する
     * @param[in] amount 回復量
     * 
     * @details
     * 体力を増加させます。最大値を超えることはありません。
     */
    void Heal(float amount) {
        current += amount;
        if (current > max) current = max;
    }

    /**
     * @brief 死亡しているか確認
     * @return true 死亡している, false 生存している
     */
    bool IsDead() const {
        return current <= 0.0f;
    }
};

/**
 * @struct Velocity
 * @brief 速度ベクトルコンポーネント
 *
 * @details
 * エンティティの移動速度を保持します。
 * 物理演算や移動処理に使用します。
 *
 * @author 山内陽
 */
struct Velocity : IComponent {
    DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };  ///< 速度ベクトル

    /**
     * @brief 速度を加算
     * @param[in] x X軸方向の速度
     * @param[in] y Y軸方向の速度
     * @param[in] z Z軸方向の速度
     */
    void AddVelocity(float x, float y, float z) {
        velocity.x += x;
        velocity.y += y;
        velocity.z += z;
    }
};
