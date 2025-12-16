#pragma once
#include "components/Component.h"
#include "components/ModelComponent.h"
#include <vector>
#include <string>
#include <map>

/**
 * @struct Animator
 * @brief スケルタルアニメーションを制御するコンポーネント
 */
struct Animator : Behaviour {
    // 読み込んだアニメーションクリップのリスト
    std::vector<ModelComponent::AnimationClip> animations;
    
    // 再生中のアニメーションインデックス (-1で停止)
    int currentAnimationIndex = -1;
    
    // 再生時間
    float currentTime = 0.0f;
    
    // 再生速度
    float speed = 1.0f;
    
    // ループ再生するか
    bool isLooping = true;
    
    // 再生終了フラグ
    bool isFinished = false;

    // ボーン名とインデックスのキャッシュ (再生開始時に構築)
    std::map<std::string, int> boneMapping;
    bool isMappingDirty = true;

    /**
     * @brief アニメーションを再生
     * @param name アニメーション名
     * @param loop ループするか
     */
    void Play(const std::string& name, bool loop = true) {
        for (size_t i = 0; i < animations.size(); ++i) {
            if (animations[i].name == name) {
                currentAnimationIndex = (int)i;
                currentTime = 0.0f;
                isLooping = loop;
                isFinished = false;
                isMappingDirty = true; // アニメーションが変わったらマッピング再確認の機会
                return;
            }
        }
    }

    /**
     * @brief アニメーションを停止
     */
    void Stop() {
        currentAnimationIndex = -1;
    }
    
    /**
     * @brief アニメーションを追加
     */
    void AddAnimation(const ModelComponent::AnimationClip& clip) {
        animations.push_back(clip);
    }
};
