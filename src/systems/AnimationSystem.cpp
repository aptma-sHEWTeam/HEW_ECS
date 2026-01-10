#include "systems/AnimationSystem.h"
#include "components/Animator.h"
#include "components/ModelComponent.h"
#include "app/DebugLog.h"
#include <limits>

using namespace DirectX;

namespace {
    // NaN チェックヘルパー
    inline bool IsNaN(float v) { return v != v; }
    inline bool IsNaN(const XMFLOAT3& v) { return IsNaN(v.x) || IsNaN(v.y) || IsNaN(v.z); }
    inline bool IsNaN(const XMFLOAT4& v) { return IsNaN(v.x) || IsNaN(v.y) || IsNaN(v.z) || IsNaN(v.w); }

    // 補間ヘルパー - 時刻 t に対応する Position を補間
    XMVECTOR InterpolatePosition(const ModelComponent::BoneAnimation& ch, float t, const XMVECTOR& fallback) {
        if (ch.keyframes.empty() || !ch.hasPositionKeys) return fallback;
        
        // キーフレームが1つだけの場合
        if (ch.keyframes.size() == 1) {
            const auto& k = ch.keyframes[0];
            if (IsNaN(k.position)) return fallback;
            return XMLoadFloat3(&k.position);
        }

        // サンプル時刻が最初のキー以前の場合 → 最初のキーの値を使用
        if (t <= ch.keyframes.front().time) {
            const auto& k = ch.keyframes.front();
            if (IsNaN(k.position)) return fallback;
            return XMLoadFloat3(&k.position);
        }
        
        // サンプル時刻が最後のキー以降の場合 → 最後のキーの値を使用
        if (t >= ch.keyframes.back().time) {
            const auto& k = ch.keyframes.back();
            if (IsNaN(k.position)) return fallback;
            return XMLoadFloat3(&k.position);
        }

        // キーを探索 (線形探索) - t が [idx].time と [idx+1].time の間にあるインデックスを探す
        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break;
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx;

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        // NaN チェック
        if (IsNaN(k1.position) && IsNaN(k2.position)) return fallback;
        if (IsNaN(k1.position)) return XMLoadFloat3(&k2.position);
        if (IsNaN(k2.position)) return XMLoadFloat3(&k1.position);
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        factor = std::clamp(factor, 0.0f, 1.0f);
        
        XMVECTOR p1 = XMLoadFloat3(&k1.position);
        XMVECTOR p2 = XMLoadFloat3(&k2.position);
        
        return XMVectorLerp(p1, p2, factor);
    }

    // 補間ヘルパー - 時刻 t に対応する Rotation を補間
    XMVECTOR InterpolateRotation(const ModelComponent::BoneAnimation& ch, float t, const XMVECTOR& fallback) {
        if (ch.keyframes.empty() || !ch.hasRotationKeys) return fallback;
        
        // キーフレームが1つだけの場合
        if (ch.keyframes.size() == 1) {
            const auto& k = ch.keyframes[0];
            if (IsNaN(k.rotation)) return fallback;
            return XMLoadFloat4(&k.rotation);
        }

        // サンプル時刻が最初のキー以前の場合
        if (t <= ch.keyframes.front().time) {
            const auto& k = ch.keyframes.front();
            if (IsNaN(k.rotation)) return fallback;
            return XMLoadFloat4(&k.rotation);
        }
        
        // サンプル時刻が最後のキー以降の場合
        if (t >= ch.keyframes.back().time) {
            const auto& k = ch.keyframes.back();
            if (IsNaN(k.rotation)) return fallback;
            return XMLoadFloat4(&k.rotation);
        }

        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break;
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx;

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        // NaN チェック
        if (IsNaN(k1.rotation) && IsNaN(k2.rotation)) return fallback;
        if (IsNaN(k1.rotation)) return XMLoadFloat4(&k2.rotation);
        if (IsNaN(k2.rotation)) return XMLoadFloat4(&k1.rotation);
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        factor = std::clamp(factor, 0.0f, 1.0f);
        
        XMVECTOR r1 = XMLoadFloat4(&k1.rotation);
        XMVECTOR r2 = XMLoadFloat4(&k2.rotation);

        // 最短経路補間 (Shortest Path Slerp)
        // クォータニオンの内積が負の場合、遠回り（360度以上の回転）をしてしまうため、
        // 片方を反転させて最短経路を取るようにする。
        if (XMVectorGetX(XMQuaternionDot(r1, r2)) < 0.0f) {
            r1 = XMVectorNegate(r1);
        }
        
        return XMQuaternionSlerp(r1, r2, factor);
    }

    // 補間ヘルパー - 時刻 t に対応する Scale を補間
    XMVECTOR InterpolateScale(const ModelComponent::BoneAnimation& ch, float t, const XMVECTOR& fallback) {
        if (ch.keyframes.empty() || !ch.hasScaleKeys) return fallback;
        
        // キーフレームが1つだけの場合
        if (ch.keyframes.size() == 1) {
            const auto& k = ch.keyframes[0];
            if (IsNaN(k.scale)) return fallback;
            return XMLoadFloat3(&k.scale);
        }

        // サンプル時刻が最初のキー以前の場合
        if (t <= ch.keyframes.front().time) {
            const auto& k = ch.keyframes.front();
            if (IsNaN(k.scale)) return fallback;
            return XMLoadFloat3(&k.scale);
        }
        
        // サンプル時刻が最後のキー以降の場合
        if (t >= ch.keyframes.back().time) {
            const auto& k = ch.keyframes.back();
            if (IsNaN(k.scale)) return fallback;
            return XMLoadFloat3(&k.scale);
        }

        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break;
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx;

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        // NaN チェック
        if (IsNaN(k1.scale) && IsNaN(k2.scale)) return fallback;
        if (IsNaN(k1.scale)) return XMLoadFloat3(&k2.scale);
        if (IsNaN(k2.scale)) return XMLoadFloat3(&k1.scale);
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        factor = std::clamp(factor, 0.0f, 1.0f);
        
        XMVECTOR s1 = XMLoadFloat3(&k1.scale);
        XMVECTOR s2 = XMLoadFloat3(&k2.scale);
        
        return XMVectorLerp(s1, s2, factor);
    }
}

void AnimationSystem::Update(World& world, float dt) {
    world.ForEach<Animator, ModelComponent>([&](Entity e, Animator& anim, ModelComponent& mc) {
        if (!mc.isSkinned) return;
        if (anim.currentAnimationIndex < 0 || anim.currentAnimationIndex >= (int)anim.animations.size()) return;

        auto& clip = anim.animations[anim.currentAnimationIndex];
        const float ticksPerSecond = std::max(clip.ticksPerSecond, 1.0f);
        const float duration = std::max(clip.duration, 1e-4f);
        
        // 累積時間（秒）を更新
        anim.currentTime += dt * anim.speed;
        
        // サンプリング時刻（ticks）を計算
        float sampleTicks = anim.currentTime * ticksPerSecond;
        if (!std::isfinite(sampleTicks)) {
            sampleTicks = 0.0f;
        }
        
        // ループ処理
        if (sampleTicks >= duration) {
            if (anim.isLooping) {
                sampleTicks = fmodf(sampleTicks, duration);
                anim.currentTime = sampleTicks / ticksPerSecond;
            } else {
                sampleTicks = duration;
                anim.currentTime = sampleTicks / ticksPerSecond;
                anim.isFinished = true;
            }
        }
        
        // 負のサンプル時刻を防止
        sampleTicks = std::max(0.0f, sampleTicks);

        // マッピング更新 (初回または変更時)
        if (anim.isMappingDirty || anim.boneMapping.empty()) {
            anim.boneMapping.clear();
            for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
                anim.boneMapping[mc.skeleton.bones[i].name] = (int)i;
            }
            anim.isMappingDirty = false;
        }

        // クリップ内チャンネルの deferred mapping を完了
        int unmappedCount = 0;
        for (auto& boneAnim : clip.boneAnimations) {
            auto it = anim.boneMapping.find(boneAnim.boneName);
            if (it != anim.boneMapping.end()) {
                boneAnim.boneIndex = it->second;
            } else {
                boneAnim.boneIndex = -1;
                ++unmappedCount;
            }
        }

        // バインドポーズのローカル変換を事前計算
        std::vector<XMMATRIX> bindGlobals(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<XMMATRIX> bindLocals(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<XMVECTOR> bindScales(mc.skeleton.bones.size(), XMVectorSet(1,1,1,0));
        std::vector<XMVECTOR> bindRotations(mc.skeleton.bones.size(), XMQuaternionIdentity());
        std::vector<XMVECTOR> bindTranslations(mc.skeleton.bones.size(), XMVectorZero());
        
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            XMMATRIX offset = XMLoadFloat4x4(&mc.skeleton.bones[i].offsetMatrix);
            bindGlobals[i] = XMMatrixInverse(nullptr, offset);
        }
        
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            int parentIdx = mc.skeleton.bones[i].parentIndex;
            if (parentIdx >= 0 && parentIdx < (int)mc.skeleton.bones.size()) {
                bindLocals[i] = bindGlobals[i] * XMMatrixInverse(nullptr, bindGlobals[(size_t)parentIdx]);
            } else {
                bindLocals[i] = bindGlobals[i];
            }

            XMVECTOR scale, rot, trans;
            if (XMMatrixDecompose(&scale, &rot, &trans, bindLocals[i])) {
                bindScales[i] = scale;
                bindRotations[i] = rot;
                bindTranslations[i] = trans;
            } else {
                bindScales[i] = XMVectorSet(1,1,1,0);
                bindRotations[i] = XMQuaternionIdentity();
                bindTranslations[i] = XMVectorZero();
            }
        }

        // ボーンローカル変換行列の計算
        std::vector<XMMATRIX> nodeTransforms(mc.skeleton.bones.size());
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            nodeTransforms[i] = bindLocals[i];
        }

        // アニメーションキーフレームから変換を適用
        for (const auto& boneAnim : clip.boneAnimations) {
            if (boneAnim.boneIndex < 0 || boneAnim.boneIndex >= (int)mc.skeleton.bones.size()) {
                continue;
            }
            int boneIndex = boneAnim.boneIndex;

            // フォールバック値としてバインドポーズを使用
            XMVECTOR S = bindScales[boneIndex];
            XMVECTOR R = bindRotations[boneIndex];
            XMVECTOR T = bindTranslations[boneIndex];

            // キーフレームが存在する成分のみ補間で上書き
            if (boneAnim.hasScaleKeys && !boneAnim.keyframes.empty()) {
                S = InterpolateScale(boneAnim, sampleTicks, S);
            }
            if (boneAnim.hasRotationKeys && !boneAnim.keyframes.empty()) {
                R = InterpolateRotation(boneAnim, sampleTicks, R);
            }
            if (boneAnim.hasPositionKeys && !boneAnim.keyframes.empty()) {
                T = InterpolatePosition(boneAnim, sampleTicks, T);
            }

            const std::string& bName = mc.skeleton.bones[boneIndex].name;
            // ユーザー提供の修正: 右肩(RightShoulder)をX軸-180度回転させる
            if (bName.find("RightShoulder") != std::string::npos) {
                XMVECTOR qFix = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -XM_PI); // X軸 -180度
                R = XMQuaternionMultiply(qFix, R);
            }

            //if (bName.find("LeftHand") != std::string::npos && 
            //   (bName.find("Thumb1") != std::string::npos || 
            //    bName.find("Index1") != std::string::npos || 
            //    bName.find("Middle1") != std::string::npos || 
            //    bName.find("Ring1") != std::string::npos || 
            //    bName.find("Pinky1") != std::string::npos)) {
            //    
            //    XMVECTOR qFix = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -XM_PI);
            //    R = XMQuaternionMultiply(qFix, R);
            //}

            nodeTransforms[boneIndex] = XMMatrixAffineTransformation(S, XMVectorZero(), R, T);
        }

        // 階層構造に従ってグローバル変換を計算
        XMMATRIX rootTransform = XMMatrixIdentity();
        {
            XMMATRIX globalInvForRoot = XMLoadFloat4x4(&mc.globalInverse);
            rootTransform = XMMatrixInverse(nullptr, globalInvForRoot);
            
            // 補正: 全体の回転（X180度, Z-90度）
            // 位置ずれを防ぐため、行列を分解して回転成分のみを変更し、移動成分は維持する。
            XMVECTOR s, r, t;
            if (XMMatrixDecompose(&s, &r, &t, rootTransform)) {
                // 追加回転: X軸180度 -> Z軸-90度
                XMVECTOR qRotX = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XM_PI);
                XMVECTOR qRotZ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), -XM_PIDIV2);
                
                // 回転順序: 元の回転 -> X回転 -> Z回転 (左から掛ける)
                r = XMQuaternionMultiply(qRotX, r);
                r = XMQuaternionMultiply(qRotZ, r);
                
                rootTransform = XMMatrixAffineTransformation(s, XMVectorZero(), r, t);
            }
        }
        
        std::vector<XMMATRIX> globalTransforms(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<bool> globalComputed(mc.skeleton.bones.size(), false);
        std::vector<bool> visiting(mc.skeleton.bones.size(), false);

        std::function<XMMATRIX(size_t)> resolveGlobal = [&](size_t idx) -> XMMATRIX {
            if (globalComputed[idx]) return globalTransforms[idx];
            if (visiting[idx]) {
                globalTransforms[idx] = nodeTransforms[idx];
                globalComputed[idx] = true;
                return globalTransforms[idx];
            }
            visiting[idx] = true;

            const int parentIdx = mc.skeleton.bones[idx].parentIndex;
            XMMATRIX parentGlobal = rootTransform;
            if (parentIdx >= 0 && parentIdx < (int)mc.skeleton.bones.size()) {
                parentGlobal = resolveGlobal(static_cast<size_t>(parentIdx));
            }

            // ローカル変換 × 親のグローバル = 自身のグローバル (row-major の場合 local * parent)
            globalTransforms[idx] = nodeTransforms[idx] * parentGlobal;
            globalComputed[idx] = true;
            visiting[idx] = false;
            return globalTransforms[idx];
        };

        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            resolveGlobal(i);
        }

        // 位置補正: 全体の回転やアニメーション移動による位置ずれを解消するため、
        // Hips/Rootボーンを原点に引き戻すオフセットを全ボーンに適用する。
        XMVECTOR rootPos = XMVectorZero();
        bool rootFound = false;
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            if (mc.skeleton.bones[i].name.find("Hips") != std::string::npos || 
                mc.skeleton.bones[i].name.find("Root") != std::string::npos || 
                mc.skeleton.bones[i].name.find("Pelvis") != std::string::npos) {
                rootPos = globalTransforms[i].r[3];
                rootFound = true;
                break;
            }
        }
        if (!rootFound && !mc.skeleton.bones.empty()) {
            rootPos = globalTransforms[0].r[3];
        }
        // Y軸（高さ）は維持したい場合はYを0にするなどの調整も可能だが、
        // 今回は「x+z+方向に移動」という横ずれ対策なので、全成分引いて原点に置く。
        XMMATRIX invTrans = XMMatrixTranslationFromVector(XMVectorNegate(rootPos));
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            globalTransforms[i] = globalTransforms[i] * invTrans;
        }
        
        // 最終スキニング行列を計算: globalInv * boneGlobal * offset （列ベクトル前提でCBへは転置）
        // 最終スキニング行列を計算: offset * boneGlobal * globalInv
        XMMATRIX globalInv = XMLoadFloat4x4(&mc.globalInverse);
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            XMMATRIX offset = XMLoadFloat4x4(&mc.skeleton.bones[i].offsetMatrix);
            XMMATRIX finalMat = offset * globalTransforms[i] * globalInv;
            XMStoreFloat4x4(&mc.skeleton.boneTransforms[i], finalMat);
        }
    });
}
