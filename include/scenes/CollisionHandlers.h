/**
 * @file CollisionHandlers.h
 * @brief ゲーム内衝突ハンドラーの定義
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include "components/Collision.h"
#include "components/GameTags.h"
#include "components/PlayerComponents.h"
#include "components/StageComponents.h"
#include "components/GameStats.h"
#include "systems/SoundSystem.h"
#include "graphics/Effect.h"
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"
#include <limits>

// 前方宣言
class GameScene;

// GameSceneへのグローバルアクセス用ポインタ
// 後方互換性のため維持
inline GameScene *g_GameScene = nullptr;
// リスポーン待機状態のグローバルフラグ（GameSceneから更新）
inline bool g_respawnPending = false;

inline DirectX::XMFLOAT3 ResolvePlacementCenter(World &w, Entity entity, const Transform &transform) {
    if (auto *box = w.TryGet<CollisionBox>(entity)) {
        return box->GetWorldCenter(transform);
    }
    if (auto *sphere = w.TryGet<CollisionSphere>(entity)) {
        return sphere->GetWorldCenter(transform);
    }
    if (auto *capsule = w.TryGet<CollisionCapsule>(entity)) {
        return capsule->GetWorldCenter(transform);
    }
    return transform.position;
}

// GameScene の型に依存しないコールラッパー（PlayerComponents から利用）
void GameScene_OnChargeStart(World &w);
void GameScene_OnChargeRelease(World &w, float chargeAmount01);
void GameScene_OnTimeUp(World &w, Entity player);
void GameScene_ResetChargeState();

/**
 * @brief プレイヤーをスタート地点にリセット（速度も完全リセット）
 * @param w ワールドへの参照
 * @param player リセット対象のプレイヤーエンティティ
 * @param resetTimer trueの場合、ゲームタイマーもリセットする
 */
inline void ResetPlayerToStart(World &w, Entity player, bool resetTimer = false) {
    // プレイヤーエンティティが有効かどうかをチェック
    if (!w.IsAlive(player)) {
        return;
    }

    // 移動する障害物をリセット
    w.ForEach<MovingObstacle>([&](Entity e, MovingObstacle &mo) {
        mo.state = MovingObstacle::State::WaitStart;
        mo.timer = 0.0f;
        mo.firstLoop = true;
        if (auto *t = w.TryGet<Transform>(e)) {
            t->position = mo.startPos;
        }
    });

    DirectX::XMFLOAT3 spawnMin{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    DirectX::XMFLOAT3 spawnMax{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};
    bool hasStart = false;

    // スタート地点のタグとトランスフォームを持つエンティティを検索
    w.ForEach<StartTag, Transform>([&](Entity startEntity, StartTag &, Transform &tStart) {
        const DirectX::XMFLOAT3 spawnCenter = ResolvePlacementCenter(w, startEntity, tStart);
        if (!hasStart) {
            spawnMin = spawnMax = spawnCenter;
            hasStart = true;
            return;
        }

        spawnMin.x = std::min(spawnMin.x, spawnCenter.x);
        spawnMin.y = std::min(spawnMin.y, spawnCenter.y);
        spawnMin.z = std::min(spawnMin.z, spawnCenter.z);
        spawnMax.x = std::max(spawnMax.x, spawnCenter.x);
        spawnMax.y = std::max(spawnMax.y, spawnCenter.y);
        spawnMax.z = std::max(spawnMax.z, spawnCenter.z);
    });

    if (hasStart) {
        const DirectX::XMFLOAT3 spawnPoint{
            (spawnMin.x + spawnMax.x) * 0.5f,
            (spawnMin.y + spawnMax.y) * 0.5f,
            (spawnMin.z + spawnMax.z) * 0.5f};

        if (auto *tPlayer = w.TryGet<Transform>(player)) {
            tPlayer->position = spawnPoint;
            EffekseerManager::GetInstance().PlayEffectSafe("WarpOut", tPlayer->position, {1.0f, 1.0f, 1.0f}, false);
        }
        // リスポーン時にアニメーションを安全な姿勢（Idle）へ戻す
        AnimationTools::Play(w, player, AnimationConfig::Clips::PlayerIdle, true);

        

        if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
            vPlayer->velocity = DirectX::XMFLOAT2{0.0f, 0.0f};
            vPlayer->isBoosting = false;
            vPlayer->isDecelerating = false;
            vPlayer->boostSpeed = 0.0f;
            vPlayer->boostDir = DirectX::XMFLOAT2{0.0f, 0.0f};
        }

        if (auto *pmPlayer = w.TryGet<PlayerMovement>(player)) {
            pmPlayer->isCharging_ = false;
            pmPlayer->wasCharging_ = false;
            pmPlayer->wasChargingPrev_ = false;
            pmPlayer->ResetAngleHistory();
            pmPlayer->SwitchEffect(w, player, PlayerMovement::EffectState::Idle);
        }

        w.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
            if (resetTimer) {
                stats.elapsedTime = cfg_LimitTime;
            }
            stats.waitingForPlayerMove = true;
            stats.timerRunning = false;
        });

        w.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            sp.pressedSwitch = false;
            sp.goalUnlocked = !sp.hasSwitch;
        });

        // ゴール遷移フラグをクリアして、プレイヤーが再び操作可能になるようにする
        w.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            sp.goalTransitioning = false;
        });
    }
}

/**
 * @brief 制限時間をチェックし、超過した場合はプレイヤーをリセットする
 * @param w ワールドへの参照
 * @param player チェック対象のプレイヤーエンティティ
 * @param timeLimitSeconds 制限時間（秒）
 */
inline void CheckTimeLimit(World &w, Entity player, float timeLimitSeconds) {
    // GameStatusコンポーネントを持つエンティティを検索
    w.ForEach<GameStatus>([&](Entity e, GameStatus &stats) {
        if (stats.resetDone) {
            return;
        }
        if (!stats.timerRunning || stats.isPaused) {
            return;
        }
        // 経過時間が制限時間を超えたかチェック
        if (stats.elapsedTime <= 0.0f) {
            DEBUGLOG("Timeout");
            GameScene_OnTimeUp(w, player);
            stats.elapsedTime = 0.0f;
            stats.resetDone = true;
        }
    });
}




/**
 * @struct PlayerCollisionHandler
 * @brief プレイヤーの衝突イベントを処理
 */
struct PlayerCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        if (w.Has<GoalTag>(other)) {
            // If the game is waiting for player movement (entry grace), ignore goal collisions.
            bool waitingForMove = false;
            w.ForEach<GameStatus>([&](Entity, GameStatus &gs) {
                if (gs.waitingForPlayerMove) waitingForMove = true;
            });
            if (waitingForMove) {
                DEBUGLOG("Goal collision ignored: waiting for player to start moving");
                return;
            }
            StageProgress *progress = nullptr;
            w.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
                if (!progress)
                    progress = &sp;
            });

            if (progress && !progress->goalUnlocked) {
                DEBUGLOG("スイッチは押されていない");
                return;
            }

            auto *goalTag = w.TryGet<GoalTag>(other);
            if ((progress && progress->goalTransitioning) ||
                (goalTag && goalTag->consumed) ||
                w.Has<GoalAttractor>(self)) {
                return;
            }

            // 壁越しゴール対策: プレイヤーとゴールの間に壁(WallTag)があるかレイキャストで簡易チェック
            auto *tPlayer = w.TryGet<Transform>(self);
            auto *tGoal = w.TryGet<Transform>(other);
            auto *tEffect = w.TryGet<PlayerMovement>(self);
            if (tPlayer && tGoal) {
                DirectX::XMFLOAT3 startPos = tPlayer->position;
                DirectX::XMFLOAT3 endPos = ResolvePlacementCenter(w, other, *tGoal); // ゴール中心

                DirectX::XMVECTOR vStart = DirectX::XMLoadFloat3(&startPos);
                DirectX::XMVECTOR vEnd = DirectX::XMLoadFloat3(&endPos);
                DirectX::XMVECTOR vDiff = DirectX::XMVectorSubtract(vEnd, vStart);
                DirectX::XMVECTOR vDist = DirectX::XMVector3Length(vDiff);
                float dist = DirectX::XMVectorGetX(vDist);

                if (dist > 1e-4f) {
                    DirectX::XMVECTOR vDir = DirectX::XMVectorScale(vDiff, 1.0f / dist);

                    bool blocked = false;
                    // 全てのWallTagを持つエンティティに対してレイキャスト
                    // Note: World::ForEachは2コンポーネントまでしかサポートしていないため、Transformは内部で取得
                    w.ForEach<WallTag, CollisionBox>([&](Entity e, WallTag &, CollisionBox &box) {
                        if (blocked)
                            return; // 既にブロックされていたらスキップ

                        auto *tWallPtr = w.TryGet<Transform>(e);
                        if (!tWallPtr)
                            return;
                        const Transform &tWall = *tWallPtr;

                        // 壁のAABBを作成
                        DirectX::XMFLOAT3 center = box.GetWorldCenter(tWall);
                        DirectX::XMFLOAT3 scaledSize = box.GetScaledSize(tWall);
                        DirectX::XMFLOAT3 extents = {scaledSize.x * 0.5f, scaledSize.y * 0.5f, scaledSize.z * 0.5f};

                        DirectX::BoundingBox aabb(center, extents);
                        float hitDist = 0.0f;
                        // レイ判定 (origin, direction, dist)
                        if (aabb.Intersects(vStart, vDir, hitDist)) {
                            // プレイヤーより先、かつゴールより手前でヒットしたか
                            // (近い壁ほどhitDistは小さい。0なら自分の中。)
                            if (hitDist > 0.1f && hitDist < dist - 0.2f) { // マージンを考慮
                                blocked = true;
                                DEBUGLOG("Goal blocked by wall entity " + std::to_string(e.id));
                            }
                        }
                    });

                    if (blocked) {
                        return; // 壁があるのでゴールしない
                    }
                }
            }

            if (progress)
                progress->goalTransitioning = true;
            if (goalTag)
                goalTag->consumed = true;
            GameScene_ResetChargeState(); // チャージ中のフェードとズームをリセット
            DEBUGLOG("Player reached goal");

            //ゴールエフェクト停止
            EffekseerManager::GetInstance().StopEffect("Goal");

            //加速板エフェクト停止
            EffekseerManager::GetInstance().StopEffect("DashBoard");

            //エフェクトをIdleに変更
            auto *playerEffect = w.TryGet<PlayerMovement>(self);
            playerEffect->SwitchEffect(w, self, PlayerMovement::EffectState::Idle);

            // ゆっくり吸い込み: ゴール中心へイージングで寄せる
            // tPlayer, tGoalは既に上で取得済み

            //エフェクト実装：ゴールとリンク
            if (tGoal) {
                EffekseerManager::GetInstance().PlayEffect("WarpIn", tGoal->position, {0, 0, 0}, false);
            }

            if (tPlayer && tGoal) {
                const DirectX::XMFLOAT3 goalCenter = ResolvePlacementCenter(w, other, *tGoal);

                auto handleOpt = EffekseerManager::GetInstance().PlayEffectSafe("WarpIn", tPlayer->position, {1.0f, 1.0f, 1.0f}, false);
                int handle = handleOpt.value_or(-1);

                // 速度をリセット
                if (auto *v = w.TryGet<PlayerVelocity>(self)) {
                    v->velocity = {0.0f, 0.0f};
                    v->isBoosting = false;
                    v->isDecelerating = false;
                    v->boostSpeed = 0.0f;
                    v->boostDir = {0.0f, 0.0f};
                }
                // 吸い込み用のBehaviourを付与（ステージ進行は吸い込み完了後に行う）
                GoalAttractor attract;
                attract.target = goalCenter;
                attract.duration = GoalAttractor::cfg_GoalAttractDuration.Get();
                attract.effectHandle = handle;

                w.Add<GoalAttractor>(self, attract);
                SOUND_SYS.PlaySE(cfg_WarpUpMP3Pass.Get());
            }
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(PlayerCollisionHandler)

/**
 * @struct EnemyCollisionHandler
 * @brief 敵の衝突イベントを処理
 */
struct EnemyCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        if (w.Has<PlayerTag>(other)) {
            DEBUGLOG("Enemy collided with player");
        }
        SOUND_SYS.PlaySE(cfg_CollideMP3Pass);
    }
};
REGISTER_COLLISION_HANDLER_TYPE(EnemyCollisionHandler)

/**
 * @struct WallCollisionHandler
 * @brief 壁の衝突イベントを処理（カメラシェイク→遅延リスポーン）
 */
struct WallCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override;
};
REGISTER_COLLISION_HANDLER_TYPE(WallCollisionHandler)

/**
 * @struct FloorWallCollisionHandler
 * @brief ステージの壁の衝突イベントを処理（カメラシェイク→遅延リスポーン）
 */
struct FloorWallCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override;
};
REGISTER_COLLISION_HANDLER_TYPE(FloorWallCollisionHandler)

/**
 * @struct DashBordCollisionHandler
 * @brief 加速板の衝突イベントを処理
 */
struct DashBordCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        auto *v = w.TryGet<PlayerVelocity>(other);    //速度コンポーネント取得
        auto *dash = w.TryGet<DashBoardStatus>(self); //加速板のステータス取得
        auto *tDashBord = w.TryGet<Transform>(other);
        auto *tSelf = w.TryGet<Transform>(self);
        if (!w.Has<PlayerTag>(other) || !v || !dash) {
            return;
        }

        const float angleRad = DirectX::XMConvertToRadians(dash->accelAngle);
        const DirectX::XMFLOAT2 boostDir{std::cosf(angleRad), std::sinf(angleRad)};
        const float boostSpeed = v->speed * v->Acceleration * cfg_AccelerateAccfication;

        DirectX::XMFLOAT3 effectPos = tSelf->position;
        effectPos.y += 0.2f;
        //加速板のエフェクト(プレイヤーが加速板に触れたらエフェクトが出る)
        EffekseerManager::GetInstance().PlayEffect("SpeedUp", effectPos, {1.0f, 1.0f, 1.0f}, false);

        // 現在の速度を無視して、指定角度・指定大きさで上書き
        v->boostDir = boostDir;
        v->boostSpeed = boostSpeed;
        v->velocity.x = boostDir.x * boostSpeed;
        v->velocity.y = boostDir.y * boostSpeed;
        v->isBoosting = true;
        v->isDecelerating = false;

        SOUND_SYS.PlaySE(cfg_SpeedUpMP3Pass);

        DEBUGLOG("プレイヤーが加速板と接触 - 速度付与");
    }
};
REGISTER_COLLISION_HANDLER_TYPE(DashBordCollisionHandler)

struct SwitchCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &world, Entity self, Entity other, const CollisionInfo &info) override {
        if (world.Has<PlayerTag>(other)) {
            world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
                if (!sp.pressedSwitch) {
                    sp.pressedSwitch = true;
                    sp.goalUnlocked = true;
                    DEBUGLOG("スイッチが押されました");
                    //以下SEなど
                    SOUND_SYS.PlaySE(cfg_KeyMP3Pass.Get());
                }
            });
            //見た目変更系の処理
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(SwitchCollisionHandler)
