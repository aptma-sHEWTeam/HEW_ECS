#pragma once
#include "components/UIImageComponents.h"
#include "animation/Animation.h"
#include "components/Animator.h"
#include "components/UIComponents.h"
#include "graphics/ModelLoader.h"
#include <string>
#include <vector>
#include <algorithm>

/**
 * 汎用アニメーションユーティリティ
 * - スプライトシート: 作成/付与/再生/所要時間
 * - モデルアニメ: クリップロードとAnimator初期化/再生
 */

struct SpriteSheetDesc {
    int frameCount = 1;
    int columns = 0;             // 0なら横方向自動
    int rows = 0;                // 0なら縦方向自動
    float frameTime = 0.1f;
    bool loop = true;
    int startFrame = 0;
    int direction = 1;           // 1:正方向, -1:逆方向
    bool playOnStart = true;

    static SpriteSheetDesc Grid(int frames, int cols, float frameTimeSec = 0.1f, bool loop = true) {
        SpriteSheetDesc d;
        d.frameCount = std::max(1, frames);
        d.columns = std::max(1, cols);
        d.frameTime = frameTimeSec;
        d.loop = loop;
        return d;
    }

    static SpriteSheetDesc FromPixelSize(int frameW, int frameH, int sheetW, int sheetH, int frames, float frameTimeSec = 0.1f, bool loop = true) {
        SpriteSheetDesc d;
        d.frameCount = std::max(1, frames);
        d.columns = std::max(1, sheetW / std::max(1, frameW));
        d.rows = std::max(1, sheetH / std::max(1, frameH));
        d.frameTime = frameTimeSec;
        d.loop = loop;
        return d;
    }
};

namespace AnimationTools {

inline SpriteSheetAnimation MakeSpriteSheet(const SpriteSheetDesc& desc) {
    SpriteSheetAnimation anim;
    anim.frameCount = std::max(1, desc.frameCount);
    anim.columns = desc.columns;
    anim.rows = desc.rows;
    anim.frameTime = std::max(1e-4f, desc.frameTime);
    anim.isLooping = desc.loop;
    anim.playbackDirection = (desc.direction >= 0) ? 1 : -1;
    anim.currentFrame = std::clamp(desc.startFrame, 0, std::max(0, anim.frameCount - 1));
    anim.isFinished = false;
    anim.UpdateUV();
    if (desc.playOnStart) {
        anim.StartAnimation(anim.playbackDirection, false /*resetFrame*/);
    }
    return anim;
}

inline SpriteSheetAnimation& AddSpriteSheet(World& world, Entity entity, const SpriteSheetDesc& desc) {
    auto anim = MakeSpriteSheet(desc);
    auto& comp = world.Add<SpriteSheetAnimation>(entity, anim);
    if (comp.uv.size() != static_cast<size_t>(comp.frameCount)) comp.UpdateUV();
    if (auto* img = world.TryGet<UIImage>(entity)) {
        const int idx = std::clamp(comp.currentFrame, 0, (int)comp.uv.size() - 1);
        if (!comp.uv.empty()) img->uvRect = comp.uv[idx];
    }
    return comp;
}

inline void PlaySpriteSheet(World& world, Entity entity, int direction = 1, bool loop = false, bool reset = true) {
    if (auto* anim = world.TryGet<SpriteSheetAnimation>(entity)) {
        if (anim->uv.size() != static_cast<size_t>(std::max(anim->frameCount, 0))) {
            anim->UpdateUV();
        }
        anim->isLooping = loop;
        anim->StartAnimation(direction, reset);
        if (auto* img = world.TryGet<UIImage>(entity)) {
            const int idx = std::clamp(anim->currentFrame, 0, (int)anim->uv.size() - 1);
            if (!anim->uv.empty()) img->uvRect = anim->uv[idx];
        }
    }
}

inline float DurationSeconds(const SpriteSheetAnimation& anim) {
    const int count = std::max(anim.frameCount, 0);
    return anim.frameTime * static_cast<float>(count);
}

inline std::string ClipNameFromPath(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    size_t start = (slash == std::string::npos) ? 0 : slash + 1;
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot < start) dot = path.size();
    return path.substr(start, dot - start);
}

inline std::vector<ModelComponent::AnimationClip> LoadClips(const std::string& path, const std::string& fallback = "") {
    auto clips = ModelLoader::LoadAnimation(path);
    if (clips.empty() && !fallback.empty()) {
        clips = ModelLoader::LoadAnimation(fallback);
    }
    return clips;
}

// 複数ファイルからまとめてクリップをロードし、名前が空ならファイル名由来で補填する
inline std::vector<ModelComponent::AnimationClip> LoadClipsFromFiles(const std::vector<std::string>& paths,
                                                                     const std::vector<std::string>& fallbacks = {},
                                                                     const std::vector<std::string>& aliases = {}) {
    std::vector<ModelComponent::AnimationClip> merged;
    for (size_t i = 0; i < paths.size(); ++i) {
        const std::string& path = paths[i];
        const std::string fallback = (i < fallbacks.size()) ? fallbacks[i] : "";
        const std::string alias = (i < aliases.size()) ? aliases[i] : "";
        auto clips = LoadClips(path, fallback);
        for (auto& c : clips) {
            // 名前を補完/上書き
            if (!alias.empty()) {
                c.name = alias;
            } else if (c.name.empty()) {
                c.name = ClipNameFromPath(path);
            }
            // 同名クリップが既にある場合は末尾に番号を付けて重複回避
            std::string original = c.name;
            int suffix = 1;
            while (std::any_of(merged.begin(), merged.end(), [&](const auto& exist) { return exist.name == c.name; })) {
                c.name = original + "_" + std::to_string(suffix++);
            }
            merged.push_back(c);
        }
    }
    return merged;
}

inline bool InitAnimator(World& world, Entity entity, const std::vector<ModelComponent::AnimationClip>& clips, const std::string& defaultClip) {
    Animator* animator = world.TryGet<Animator>(entity);
    if (!animator) animator = &world.Add<Animator>(entity);
    animator->animations = clips;
    animator->currentAnimationIndex = -1;
    animator->isFinished = false;
    animator->isMappingDirty = true;
    if (!clips.empty() && !defaultClip.empty()) {
        animator->Play(defaultClip, true);
    }
    return !clips.empty();
}

inline bool Play(World& world, Entity entity, const std::string& name, bool loop = true) {
    if (auto* animator = world.TryGet<Animator>(entity)) {
        animator->Play(name, loop);
        return true;
    }
    return false;
}

inline bool HasClip(World& world, Entity entity, const std::string& name) {
    if (auto* animator = world.TryGet<Animator>(entity)) {
        for (auto& c : animator->animations) if (c.name == name) return true;
    }
    return false;
}

} // namespace AnimationTools
