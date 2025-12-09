#pragma once

#include <cstdint>
#include <functional>
#include "components/Component.h"
#include "scenes/StageConfig.h"

struct SoundStatas : IComponent{
    bool soundEnabled = true;
    float masterVolume = 1.0;       //0..1（サウンドの基）
    float seVolume = 1.0f;          //0..1
    float bgmVolume = 1.0f;         //0..1
    bool visualFallback = false;    //視覚代替（デバッグ用）
};
