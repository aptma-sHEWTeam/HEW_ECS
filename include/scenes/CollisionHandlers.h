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

// 前方宣言
class GameScene;

// GameSceneへのグローバルアクセス用ポインタ
// 後方互換性のため維持
inline GameScene* g_GameScene = nullptr;

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
    
    // スタート地点を見つけて処理を一度だけ実行するためのフラグ
    bool done = false;
    // スタート地点のタグとトランスフォームを持つエンティティを検索
    w.ForEach<StartTag, Transform>([&](Entity, StartTag &, Transform &tStart) {
        // 既に処理済みならスキップ
        if (done) {
            return;
        }

        // プレイヤーのTransformコンポーネントを取得
        if (auto *tPlayer = w.TryGet<Transform>(player)) {
            // プレイヤーの位置をスタート地点のX, Z座標に設定（Yは0で固定）
            tPlayer->position = DirectX::XMFLOAT3{tStart.position.x, 0.0f, tStart.position.z};

            // プレイヤーの速度コンポーネントを取得
            if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
                // 速度とブースト関連のステータスを完全にリセット
                vPlayer->velocity = DirectX::XMFLOAT2{0.0f, 0.0f};
                vPlayer->isBoosting = false;
                vPlayer->isDecelerating = false;
                vPlayer->boostSpeed = 0.0f;
                vPlayer->boostDir = DirectX::XMFLOAT2{0.0f, 0.0f};
            }

            // プレイヤーの移動コンポーネントを取得
            if (auto *pmPlayer = w.TryGet<PlayerMovement>(player)) {
                // チャージ関連のステータスをリセット
                pmPlayer->isCharging_ = false;
                pmPlayer->wasCharging_ = false;
                pmPlayer->wasChargingPrev_ = false;
                // 角度の履歴もリセットして、移動方向の計算を初期化
                pmPlayer->ResetAngleHistory();
            }
        }
        
        // タイマーリセットが要求されている場合
        if (resetTimer) {
            // GameStatusコンポーネントを持つエンティティを検索
            w.ForEach<GameStatus>([](Entity, GameStatus &stats) {
                // 経過時間を0にリセット
                stats.elapsedTime = 0.0f;
            });
        }

        // 処理完了フラグを立て、以降のスタート地点が見つかっても処理しないようにする
        done = true;
    });
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
        // 経過時間が制限時間を超えたかチェック
        if (stats.elapsedTime >= timeLimitSeconds) {
            DEBUGLOG("Timeout");
            // プレイヤーをスタート地点にリセット（タイマーもリセット）
            ResetPlayerToStart(w, player, true);
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
            w.ForEach<StageProgress>([](Entity, StageProgress &sp) { sp.requestAdvance = true; });
            DEBUGLOG("Player reached goal");
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
        auto *v = w.TryGet<PlayerVelocity>(other);              //速度コンポーネント取得
        auto *dash = w.TryGet<DashBoardStatus>(self);           //加速板のステータス取得
        if (w.Has<PlayerTag>(other)) {
            if (v && dash) {
                DEBUGLOG("プレイヤーが加速板と接触 - プレイヤー加速-");
                v->isBoosting = true;
            }
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(DashBordCollisionHandler)
