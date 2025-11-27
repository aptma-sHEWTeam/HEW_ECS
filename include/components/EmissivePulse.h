/**
 * @file EmissivePulse.h
 * @brief エミッション強度をパルスさせるBehaviour
 * @version 1.0
 */
#pragma once

#include "components/Component.h"
#include "EmissiveMaterial.h"
#include <cmath>

/**
 * @brief エミッションパルスBehaviour
 * @details 発光強度を周期的に変化させる
 */
struct EmissivePulse : public Behaviour {
    float minIntensity = 0.5f;
    float maxIntensity = 2.0f;
    float speed = 2.0f;
    float phase = 0.0f;

    EmissivePulse() = default;

    EmissivePulse(float minI, float maxI, float spd)
        : minIntensity(minI), maxIntensity(maxI), speed(spd) {}

    void OnUpdate(World& w, Entity self, float dt) override {
        phase += speed * dt;
        auto* emissive = w.TryGet<EmissiveMaterial>(self);
        if (emissive && emissive->enabled) {
            float t = (std::sin(phase) + 1.0f) * 0.5f;
            emissive->emissiveIntensity = minIntensity + t * (maxIntensity - minIntensity);
        }
    }
};
