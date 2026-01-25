/**
 * @file TimeScaleComponents.h
 * @brief ゲーム全体/演出用の時間倍率コンポーネント
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "components/Component.h"

inline float g_TimeScaleValue = 1.0f;

inline void SetGlobalTimeScale(float value) {
    g_TimeScaleValue = value;
}

inline float GetGlobalTimeScale() {
    return g_TimeScaleValue;
}

/**
 * @struct TimeScale
 * @brief World内で共有する時間倍率
 */
struct TimeScale : IComponent {
    float value = 1.0f;
};
