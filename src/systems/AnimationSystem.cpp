#include "systems/AnimationSystem.h"
#include "components/Animator.h"
#include "components/ModelComponent.h"
#include "app/DebugLog.h"

using namespace DirectX;

namespace {
    // 補間ヘルパー
    XMVECTOR InterpolatePosition(const ModelComponent::BoneAnimation& ch, float t) {
        if (ch.keyframes.empty()) return XMVectorZero();
        if (ch.keyframes.size() == 1) return XMLoadFloat3(&ch.keyframes[0].position);

        // キーを探索 (線形探索)
        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break; // 右開区間 [i, i+1)
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx; // 終端保護

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        
        XMVECTOR p1 = XMLoadFloat3(&k1.position);
        XMVECTOR p2 = XMLoadFloat3(&k2.position);
        
        return XMVectorLerp(p1, p2, factor);
    }

    XMVECTOR InterpolateRotation(const ModelComponent::BoneAnimation& ch, float t) {
        if (ch.keyframes.empty()) return XMVectorSet(0, 0, 0, 1);
        if (ch.keyframes.size() == 1) return XMLoadFloat4(&ch.keyframes[0].rotation);

        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break; // 右開区間 [i, i+1)
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx;

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        
        XMVECTOR r1 = XMLoadFloat4(&k1.rotation);
        XMVECTOR r2 = XMLoadFloat4(&k2.rotation);
        
        return XMQuaternionSlerp(r1, r2, factor);
    }

    XMVECTOR InterpolateScale(const ModelComponent::BoneAnimation& ch, float t) {
        if (ch.keyframes.empty()) return XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        if (ch.keyframes.size() == 1) return XMLoadFloat3(&ch.keyframes[0].scale);

        size_t idx = 0;
        for (; idx < ch.keyframes.size() - 1; ++idx) {
            if (t < ch.keyframes[idx + 1].time) break; // 右開区間 [i, i+1)
        }
        size_t nextIdx = idx + 1;
        if (nextIdx >= ch.keyframes.size()) nextIdx = idx;

        const auto& k1 = ch.keyframes[idx];
        const auto& k2 = ch.keyframes[nextIdx];
        
        float dt = k2.time - k1.time;
        float factor = (dt > 0.0001f) ? (t - k1.time) / dt : 0.0f;
        
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
        
        // 累積時間（秒）を更新
        anim.currentTime += dt * anim.speed;
        // サンプリング時刻（ticks）を計算し、キー境界一致回避の微小オフセットを付与
        float sampleTicks = anim.currentTime * clip.ticksPerSecond + 1e-4f;
        if (sampleTicks >= clip.duration) {
            if (anim.isLooping) {
                sampleTicks = fmodf(sampleTicks, clip.duration);
                anim.currentTime = sampleTicks / std::max(clip.ticksPerSecond, 1.0f);
            } else {
                sampleTicks = clip.duration;
                anim.currentTime = sampleTicks / std::max(clip.ticksPerSecond, 1.0f);
                anim.isFinished = true;
            }
        }

        // マッピング更新 (初回または変更時)
        if (anim.isMappingDirty || anim.boneMapping.empty()) {
            anim.boneMapping.clear();
            for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
                anim.boneMapping[mc.skeleton.bones[i].name] = (int)i;
            }
#ifdef _DEBUG
            DEBUGLOG("AnimationSystem: boneMapping for entity " + std::to_string(e.id) + ":");
            for (const auto& kv : anim.boneMapping) {
                DEBUGLOG("  boneName=" + kv.first + ", index=" + std::to_string(kv.second));
            }
#endif
            anim.isMappingDirty = false;
        }

        // クリップ内チャンネルの deferred mapping を完了（名前→mesh配列インデックス）。
        // ここで boneAnim.boneIndex を実インデックスに確定する。
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
#ifdef _DEBUG
        static int s_mapLogCount = 0;
        if (s_mapLogCount < 3) {
            DEBUGLOG("AnimationSystem: clipBoneAnims total=" + std::to_string(clip.boneAnimations.size()) +
                     ", unmapped=" + std::to_string(unmappedCount));
            DEBUGLOG("  time: seconds=" + std::to_string(anim.currentTime) +
                     ", ticksPerSec=" + std::to_string(clip.ticksPerSecond) +
                     ", timeInTicks=" + std::to_string(sampleTicks));
            // 代表例を出力
            auto dumpIndex = [&](const char* n){
                auto it = anim.boneMapping.find(n);
                DEBUGLOG(std::string("  rep '") + n + "' -> " + (it!=anim.boneMapping.end()? std::to_string(it->second): std::string("(none)")) );
            };
            dumpIndex("Hips");
            dumpIndex("Spine");
            dumpIndex("Head");
            s_mapLogCount++;
        }
#endif

        // ボーンローカル変換行列の計算 (アニメーション適用)
        std::vector<XMMATRIX> nodeTransforms(mc.skeleton.bones.size());

        // 1. バインドポーズのローカル変換で初期化（アニメ未適用時にTポーズになるのを防ぐ）
        std::vector<XMMATRIX> bindGlobals(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<XMMATRIX> bindLocals(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<XMVECTOR> bindScales(mc.skeleton.bones.size(), XMVectorSet(1,1,1,0));
        std::vector<XMVECTOR> bindRotations(mc.skeleton.bones.size(), XMQuaternionIdentity());
        std::vector<XMVECTOR> bindTranslations(mc.skeleton.bones.size(), XMVectorZero());
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            XMMATRIX offset = XMLoadFloat4x4(&mc.skeleton.bones[i].offsetMatrix); // inverse bind pose
            bindGlobals[i] = XMMatrixInverse(nullptr, offset); // bind global
        }
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            int parentIdx = mc.skeleton.bones[i].parentIndex;
            if (parentIdx >= 0 && parentIdx < (int)mc.skeleton.bones.size()) {
                bindLocals[i] = bindGlobals[i] * XMMatrixInverse(nullptr, bindGlobals[(size_t)parentIdx]);
            } else {
                bindLocals[i] = bindGlobals[i];
            }

            // バインドローカルを分解して保存（キーが無い成分のフォールバックに使う）
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
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            // 初期はバインドローカル姿勢にしておく（キーが無いボーンはバインドを維持）
            nodeTransforms[i] = bindLocals[i];
        }

        for (const auto& boneAnim : clip.boneAnimations) {
            // インデックスが確定していないチャンネルはスキップ
            if (boneAnim.boneIndex < 0 || boneAnim.boneIndex >= (int)mc.skeleton.bones.size()) {
                continue;
            }
            int boneIndex = boneAnim.boneIndex;

            XMVECTOR S = bindScales[boneIndex];
            XMVECTOR R = bindRotations[boneIndex];
            XMVECTOR T = bindTranslations[boneIndex];

            if (boneAnim.hasScaleKeys) {
                S = InterpolateScale(boneAnim, sampleTicks);
            }
            if (boneAnim.hasRotationKeys) {
                R = InterpolateRotation(boneAnim, sampleTicks);
            }
            if (boneAnim.hasPositionKeys) {
                T = InterpolatePosition(boneAnim, sampleTicks);
            }

            nodeTransforms[boneIndex] = XMMatrixAffineTransformation(S, XMVectorZero(), R, T);
        }

#ifdef _DEBUG
        // 代表ボーンのキーインデックス/補間係数と回転クォータニオンの診断
        auto logChannel = [&](const char* name){
            for (const auto& ba : clip.boneAnimations) {
                if (ba.boneName == name && ba.boneIndex >= 0 && !ba.keyframes.empty()) {
                    size_t idx = 0;
                    for (;// idx < ba.keyframes.size() - 1; ++idx) {
                        if (sampleTicks < ba.keyframes[idx + 1].time) break;
                    }
                    size_t nextIdx = (idx + 1 < ba.keyframes.size()) ? idx + 1 : idx;
                    float dtK = ba.keyframes[nextIdx].time - ba.keyframes[idx].time;
                    float tK = (dtK > 1e-5f) ? (sampleTicks - ba.keyframes[idx].time) / dtK : 0.0f;
                    const auto& q = ba.keyframes[idx].rotation;
                    DEBUGLOG(std::string("Sample diag '") + name + "': keyIdx=" + std::to_string(idx) +
                             ", t=" + std::to_string(tK) +
                             ", keyTime=" + std::to_string(ba.keyframes[idx].time) +
                             ", quat=(" + std::to_string(q.x) + "," + std::to_string(q.y) + "," + std::to_string(q.z) + "," + std::to_string(q.w) + ")");
                    return;
                }
            }
            DEBUGLOG(std::string("Sample diag '") + name + "': channel not found or no keys");
        };
        static int s_sampleLog = 0;
        if (s_sampleLog < 3) {
            logChannel("Spine");
            logChannel("Head");
            s_sampleLog++;
        }
#endif

        // 2. 階層構造に従ってグローバル変換を計算
        //    Bone配列の順序が親→子である保証がないため、親を再帰的に解決する。
        std::vector<XMMATRIX> globalTransforms(mc.skeleton.bones.size(), XMMatrixIdentity());
        std::vector<bool>     globalComputed(mc.skeleton.bones.size(), false);
        std::vector<bool>     visiting(mc.skeleton.bones.size(), false);
        static bool loggedCycle = false;

        std::function<XMMATRIX(size_t)> resolveGlobal = [&](size_t idx) -> XMMATRIX {
            if (globalComputed[idx]) return globalTransforms[idx];
            if (visiting[idx]) {
#ifdef _DEBUG
                if (!loggedCycle) {
                    DEBUGLOG_WARNING("AnimationSystem: detected potential parent cycle at bone index " + std::to_string(idx) +
                                     " name=" + mc.skeleton.bones[idx].name);
                    loggedCycle = true;
                }
#endif
                globalTransforms[idx] = nodeTransforms[idx];
                globalComputed[idx] = true;
                return globalTransforms[idx];
            }
            visiting[idx] = true;

            const int parentIdx = mc.skeleton.bones[idx].parentIndex;
            XMMATRIX parentGlobal = XMMatrixIdentity();
            if (parentIdx >= 0 && parentIdx < (int)mc.skeleton.bones.size()) {
                parentGlobal = resolveGlobal(static_cast<size_t>(parentIdx));
            }

            // 親→子の順で適用（行列はRow-Major）
            globalTransforms[idx] = parentGlobal * nodeTransforms[idx];
            globalComputed[idx] = true;
            visiting[idx] = false;
            return globalTransforms[idx];
        };

        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            resolveGlobal(i);
        }
        
        // 3. Offset Matrix (Inverse Bind Pose) を掛けて、最終的なスキニング行列
        for (size_t i = 0; i < mc.skeleton.bones.size(); ++i) {
            XMMATRIX offset = XMLoadFloat4x4(&mc.skeleton.bones[i].offsetMatrix);
            XMMATRIX finalMat = offset * globalTransforms[i];
            XMStoreFloat4x4(&mc.skeleton.boneTransforms[i], finalMat);
        }

#ifdef _DEBUG
        static int s_loggedCount = 0;
        if (s_loggedCount < 3) {
            DEBUGLOG("Anim debug: clip='" + clip.name +
                     "', duration=" + std::to_string(clip.duration) +
                     ", ticksPerSec=" + std::to_string(clip.ticksPerSecond) +
                     ", boneAnims=" + std::to_string(clip.boneAnimations.size()));
            DEBUGLOG("Anim debug: skeleton bones=" + std::to_string(mc.skeleton.bones.size()));

            // Hips のキーフレーム概要を出力
            for (const auto& ba : clip.boneAnimations) {
                if (ba.boneName == "Hips") {
                    DEBUGLOG("Anim debug: Hips keyframes=" + std::to_string(ba.keyframes.size()) +
                             ", hasPos=" + std::to_string(ba.hasPositionKeys) +
                             ", hasRot=" + std::to_string(ba.hasRotationKeys) +
                             ", hasScale=" + std::to_string(ba.hasScaleKeys) +
                             ", mappedIndex=" + std::to_string(ba.boneIndex));
                    break;
                }
            }

            if (!mc.skeleton.boneTransforms.empty()) {
                const auto& m = mc.skeleton.boneTransforms[0]; // 代表: bone0
                DEBUGLOG("Anim debug: entity=" + std::to_string(e.id) +
                         ", bone0 finalMat m41,m42,m43=" +
                         std::to_string(m._41) + "," +
                         std::to_string(m._42) + "," +
                         std::to_string(m._43) +
                         ", currentTime=" + std::to_string(anim.currentTime));
            }
            s_loggedCount++;
        }
#endif

#ifdef _DEBUG
        static int s_log = 0;
        if (s_log < 3) {
            // Head(5)想定の最終行列とグローバル行列の一部をダンプ
            int headIdx = -1;
            auto itH = anim.boneMapping.find("Head");
            if (itH != anim.boneMapping.end()) headIdx = itH->second;
            if (headIdx >= 0 && headIdx < (int)mc.skeleton.bones.size()) {
                const auto& g = globalTransforms[headIdx];
                const auto& f = mc.skeleton.boneTransforms[headIdx];
                DEBUGLOG("Head global m11,m12,m13=" + std::to_string(g.r[0].m128_f32[0]) + "," + std::to_string(g.r[0].m128_f32[1]) + "," + std::to_string(g.r[0].m128_f32[2]) +
                         ", m41,m42,m43=" + std::to_string(g.r[3].m128_f32[0]) + "," + std::to_string(g.r[3].m128_f32[1]) + "," + std::to_string(g.r[3].m128_f32[2]));
                DEBUGLOG("Head final m11,m12,m13=" + std::to_string(f._11) + "," + std::to_string(f._12) + "," + std::to_string(f._13) +
                         ", m41,m42,m43=" + std::to_string(f._41) + "," + std::to_string(f._42) + "," + std::to_string(f._43));
            }
            s_log++;
        }
#endif
    });
}
