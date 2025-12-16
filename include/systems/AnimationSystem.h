#pragma once
#include "ecs/World.h"
#include "graphics/RenderSystem.h" // RenderSystem has ModelComponent

/**
 * @class AnimationSystem
 * @brief アニメーション更新システム
 *
 * @details
 * Animatorコンポーネントを持つエンティティを更新し、
 * ボーン変換行列を計算してModelComponentに適用します。
 */
class AnimationSystem {
public:
    static AnimationSystem& GetInstance() {
        static AnimationSystem instance;
        return instance;
    }

    void Update(World& world, float dt);

private:
    AnimationSystem() = default;
};
