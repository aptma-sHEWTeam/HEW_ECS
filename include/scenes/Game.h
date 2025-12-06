/**
 * @file Game.h
 * @brief UIを統合したゲームシーン
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"

#include <sstream>
#include <iomanip>
#include <cstdlib>

#include "config/ConfigVar.h"
#include "components/GameTags.h"
#include "components/PlayerComponents.h"
#include "components/UIComponents.h"
#include "components/UIImageComponents.h"
#include "components/CountUIComponent.h"
#include "components/Rotator.h"
#include "components/Light.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include "components/EmissiveMaterial.h"
#include "components/EmissivePulse.h"
#include "components/PointLight.h"
#include "systems/RenderingSystem.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "graphics/ImageSystem.h"
#include "app/ServiceLocator.h"
#include "SenesUIController.h"
#include "systems/ModelLoadingSystem.h"
#include "graphics/TextureManager.h"
#include "graphics/Camera.h"

// 前方宣言
class GameScene;

// GameSceneへのグローバルアクセス用ポインタ
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
            tPlayer->position = {tStart.position.x, 0.0f, tStart.position.z};

            // プレイヤーの速度コンポーネントを取得
            if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
                // 速度とブースト関連のステータスを完全にリセット
                vPlayer->velocity = {0.0f, 0.0f};
                vPlayer->isBoosting = false;
                vPlayer->isDecelerating = false;
                vPlayer->boostSpeed = 0.0f;
                vPlayer->boostDir = {0.0f, 0.0f};
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
            DEBUGLOG("時間切れ");
            // プレイヤーをスタート地点にリセット（タイマーもリセット）
            ResetPlayerToStart(w, player, true);
        }
    });
}

// =========================================
// カメラリアクションタイプ（前方定義）
// =========================================
enum class CameraReactionType {
    None,       ///< リアクションなし
    Shake,      ///< 揺れ（ランダム）
    Impulse,    ///< 衝撃（一方向へ跳ねる）
    Zoom,       ///< ズームイン/アウト
    Tilt        ///< 傾き
};

// =========================================
// 壁衝突リスポーン用ConfigVars
// =========================================
/** @brief 壁衝突時のカメラシェイクの強さ */
inline ConfigVar<float> cfg_WallHitShakeIntensity{"Game", "WallHitShakeIntensity", 0.5f};
/** @brief 壁衝突時のカメラシェイクの持続時間 */
inline ConfigVar<float> cfg_WallHitShakeDuration{"Game", "WallHitShakeDuration", 0.3f};
/** @brief 壁衝突後、プレイヤーがリスポーンするまでの遅延時間 */
inline ConfigVar<float> cfg_WallHitRespawnDelay{"Game", "WallHitRespawnDelay", 0.4f};

/**
 * @struct PlayerCollisionHandler
 * @brief プレイヤーの衝突イベントを処理
 */
struct PlayerCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        if (w.Has<GoalTag>(other)) {
            w.ForEach<StageProgress>([](Entity, StageProgress &sp) { sp.requestAdvance = true; });
            DEBUGLOG("プレイヤーがゴールに到達");
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
            DEBUGLOG("敵がプレイヤーと衝突");
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
                float accelAngle = dash->accelAngle;
            }

            DEBUGLOG("プレイヤーが加速板と接触 - プレイヤー加速-");
            v->isBoosting = true;
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(DashBordCollisionHandler)

/**
 * @class GameScene
 * @brief 3DゲームとUIを統合したシーン
 */
class GameScene : public IScene {
  public:
    // Configs
    inline static ConfigVar<float> cfg_PlayerScale{"Game", "PlayerScale", 0.8f};
    inline static ConfigVar<float> cfg_PlayerR{"Game", "PlayerColorR", 0.0f};
    inline static ConfigVar<float> cfg_PlayerG{"Game", "PlayerColorG", 0.0f};
    inline static ConfigVar<float> cfg_PlayerB{"Game", "PlayerColorB", 1.0f};
    inline static ConfigVar<float> cfg_PlayerStartY{"Game", "PlayerStartY", 5.0f};
    inline static ConfigVar<float> cfg_PlayerHeight{"Game", "PlayerHeight", 2.0f};

    inline static ConfigVar<float> cfg_FloorR{"Game", "FloorColorR", 0.5f};
    inline static ConfigVar<float> cfg_FloorG{"Game", "FloorColorG", 0.5f};
    inline static ConfigVar<float> cfg_FloorB{"Game", "FloorColorB", 0.5f};
    inline static ConfigVar<float> cfg_FloorYOffset{"Game", "FloorYOffset", -2.0f};
    inline static ConfigVar<float> cfg_FloorThickness{"Game", "FloorThickness", 0.2f};

    inline static ConfigVar<float> cfg_StartR{"Game", "StartColorR", 0.0f};
    inline static ConfigVar<float> cfg_StartG{"Game", "StartColorG", 0.0f};
    inline static ConfigVar<float> cfg_StartB{"Game", "StartColorB", 1.0f};

    inline static ConfigVar<float> cfg_GoalR{"Game", "GoalColorR", 1.0f};
    inline static ConfigVar<float> cfg_GoalG{"Game", "GoalColorG", 1.0f};
    inline static ConfigVar<float> cfg_GoalB{"Game", "GoalColorB", 0.0f};

    inline static ConfigVar<float> cfg_WallR{"Game", "WallColorR", 1.0f};
    inline static ConfigVar<float> cfg_WallG{"Game", "WallColorG", 1.0f};
    inline static ConfigVar<float> cfg_WallB{"Game", "WallColorB", 1.0f};

    inline static ConfigVar<float> cfg_FloorWallR{"Game", "FloorWallColorR", 0.5f};
    inline static ConfigVar<float> cfg_FloorWallG{"Game", "FloorWallColorG", 0.5f};
    inline static ConfigVar<float> cfg_FloorWallB{"Game", "FloorWallColorB", 0.5f};
    inline static ConfigVar<float> cfg_WallSize{"Game", "WallSize", 3.0f};

    inline static ConfigVar<float> cfg_UICountPosX{"UI", "CountPosX", 20.0f};
    inline static ConfigVar<float> cfg_UICountPosY{"UI", "CountPosY", 170.0f};
    inline static ConfigVar<float> cfg_UICountW{"UI", "CountWidth", 200.0f};
    inline static ConfigVar<float> cfg_UICountH{"UI", "CountHeight", 40.0f};
    inline static ConfigVar<float> cfg_UICountR{"UI", "CountColorR", 0.0f};
    inline static ConfigVar<float> cfg_UICountG{"UI", "CountColorG", 1.0f};
    inline static ConfigVar<float> cfg_UICountB{"UI", "CountColorB", 1.0f};

    inline static ConfigVar<std::string> cfg_PlayerFBXPass{"Player", "PlayerFBXPass", "Assets/Models/Player/obj_player.fbx"};
    inline static ConfigVar<std::string> cfg_FloorFBXPass{"Game", "FloorFBXPass", "Assets/Models/StageObj/Ground/obj_ground.fbx"};
    inline static ConfigVar<std::string> cfg_WallFBXPass{"Game", "WallFBXPass", "Assets/Models/StageObj/Wall/obj_wall.fbx"};

    inline static ConfigVar<std::string> cfg_RoomPath{"UI", "RoomPNGPass", "Assets/Textures/Count.png"};

    inline static ConfigVar<float> cfg_CollisionCellSize{"Game", "CollisionCellSize", 20.0f};

    inline static ConfigVar<float> cfg_LimitTime{"Game", "LimitTime", 10.0f};

    inline static ConfigVar<float> cfg_StartEmissiveR{"Game", "StartEmissiveR", 0.0f};
    inline static ConfigVar<float> cfg_StartEmissiveG{"Game", "StartEmissiveG", 0.5f};
    inline static ConfigVar<float> cfg_StartEmissiveB{"Game", "StartEmissiveB", 1.0f};
    inline static ConfigVar<float> cfg_StartEmissiveIntensity{"Game", "StartEmissiveIntensity", 1.5f};
    inline static ConfigVar<float> cfg_StartPulseMin{"Game", "StartPulseMin", 4.0f};
    inline static ConfigVar<float> cfg_StartPulseMax{"Game", "StartPulseMax", 10.0f};
    inline static ConfigVar<float> cfg_StartPulseSpeed{"Game", "StartPulseSpeed", 1.0f};

    inline static ConfigVar<float> cfg_GoalEmissiveR{"Game", "GoalEmissiveR", 1.0f};
    inline static ConfigVar<float> cfg_GoalEmissiveG{"Game", "GoalEmissiveG", 0.8f};
    inline static ConfigVar<float> cfg_GoalEmissiveB{"Game", "GoalEmissiveB", 0.0f};
    inline static ConfigVar<float> cfg_GoalEmissiveIntensity{"Game", "GoalEmissiveIntensity", 2.0f};
    inline static ConfigVar<float> cfg_GoalPulseMin{"Game", "GoalPulseMin", 4.0f};
    inline static ConfigVar<float> cfg_GoalPulseMax{"Game", "GoalPulseMax", 10.0f};
    inline static ConfigVar<float> cfg_GoalPulseSpeed{"Game", "GoalPulseSpeed", 1.0f};

    inline static ConfigVar<float> cfg_StartLightRange{"Game", "StartLightRange", 5.0f};
    inline static ConfigVar<float> cfg_GoalLightRange{"Game", "GoalLightRange", 8.0f};

    // =========================================
    // カメラリアクション用ConfigVars
    // =========================================
    /** @brief カメラシェイク時のX軸方向の揺れの周波数 */
    inline static ConfigVar<float> cfg_CameraShakeFreqX{"Camera", "ShakeFreqX", 35.0f};
    /** @brief カメラシェイク時のY軸方向の揺れの周波数 */
    inline static ConfigVar<float> cfg_CameraShakeFreqY{"Camera", "ShakeFreqY", 28.0f};
    /** @brief カメラシェイク時のZ軸方向の揺れの周波数 */
    inline static ConfigVar<float> cfg_CameraShakeFreqZ{"Camera", "ShakeFreqZ", 41.0f};
    /** @brief カメラシェイク効果の減衰率。大きいほど早く収まる */
    inline static ConfigVar<float> cfg_CameraShakeDecay{"Camera", "ShakeDecay", 3.0f};
    /** @brief カメラシェイクのランダム性の強さ。0で完全なsin波、1で完全なランダム */
    inline static ConfigVar<float> cfg_CameraShakeRandomness{"Camera", "ShakeRandomness", 0.3f};
    /** @brief カメラシェイク時のY軸方向の揺れのスケール（他軸との相対） */
    inline static ConfigVar<float> cfg_CameraShakeYScale{"Camera", "ShakeYScale", 0.5f};
    /** @brief カメラの衝撃（インパルス）効果の減衰率。大きいほど早く収まる */
    inline static ConfigVar<float> cfg_CameraImpulseDecay{"Camera", "ImpulseDecay", 5.0f};
    /**
     * @brief カメラのズームイン/アウト アニメーションの速度
     */
    inline static ConfigVar<float> cfg_CameraZoomSpeed{"Camera", "ZoomSpeed", 2.0f};
    /** @brief カメラがプレイヤーを追従する際の滑らかさ。大きいほど素早く追従する */
    inline static ConfigVar<float> cfg_CameraFollowSmooth{"Camera", "FollowSmooth", 5.0f};

    /**
     * @brief シーン開始時に呼び出される初期化処理
     * @details レンダリングシステム、UIシステム、カメラ、各種エンティティ（プレイヤー、ステージなど）の生成と設定を行う
     * @param world ECSワールドへの参照
     */
    void OnEnter(World &world) override {
        DEBUGLOG("<<<<< GameScene::OnEnter CALLED! >>>>>");
        DEBUGLOG("GameWithUIScene::OnEnter() 開始");

        // このシーンインスタンスへのグローバルポインタを設定
        g_GameScene = this;

        // サービスロケーターからグラフィックスデバイスを取得
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("GfxDevice が見つかりません");
            return;
        }

        // レンダリングシステムの初期化と環境光の設定
        RenderingSystem::GetInstance().Initialize(gfx->Dev());
        RenderingSystem::GetInstance().SetAmbientLight({0.1f, 0.1f, 0.15f}, 1.0f);

        // テキスト描画システムの初期化
        if (!textSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("TextSystem の初期化に失敗しました");
            return;
        }

        // 画像描画システムの初期化
        if (!imageSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("ImageSystem の初期化に失敗しました");
            return;
        }

        // UI用のテキストフォーマットを作成
        CreateTextFormats();

        // 画面サイズを取得
        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>(cfg_CollisionCellSize.Get()).Build();
        ownedEntities_.push_back(collisionSystem);

        // モデル読み込みシステムをエンティティとして生成し、シーンが所有
        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        // ステージ進行状況に応じて、対応するステージデータを読み込むエンティティを生成
        world.ForEach<StageProgress>([&](Entity e, StageProgress &status) {
            std::string Stagepath = "Assets/StageData/StageCollision/DebugStage" + std::to_string(status.selectStage) + "/room1.csv";
            Entity stageEntity = world.Create().With<StageCreate>(Stagepath).Build();
            ownedEntities_.push_back(stageEntity);
        });

        // 平行光源をエンティティとして生成
        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        CreateUI(world, screenWidth, screenHeight);

        SetupStage(world, 1);

        DEBUGLOG("GameWithUIScene の初期化が正常に完了しました");
    }

    /**
     * @brief 毎フレーム呼び出される更新処理
     * @details 入力処理、ゲームロジックの更新、物理演算、カメラの更新などを行う
     * @param world ECSワールドへの参照
     * @param input 入力システムへの参照
     * @param deltaTime 前フレームからの経過時間
     */
    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {

        // ゲームの一時停止/再開処理
        world.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
            // ESCキーまたはPキーでポーズ状態を切り替え
            if (input.GetKeyDown(VK_ESCAPE) || input.GetKeyDown('P')) {
                stats.isPaused = !stats.isPaused;
                DEBUGLOG(stats.isPaused ? "ゲームが一時停止されました" : "ゲームが再開されました");
            }

            // ポーズ中なら、このフレームのdeltaTimeを0にして、ゲームの時間を止める
            if (stats.isPaused) {
                deltaTime = 0.0f;
            }
        });

        // ステージ進行リクエストの処理
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.requestAdvance) {
                sp.requestAdvance = false; // リクエストを消費
                sp.currentStage++;         // ステージ番号をインクリメント
                DEBUGLOG("ステージが進行しました: " + std::to_string(sp.currentStage));
                SetupStage(world, sp.currentStage); // 新しいステージをセットアップ
            }
        });

        // PlayerMovementコンポーネントに、入力システムとゲームパッドシステムへの参照を渡す
        world.ForEach<PlayerMovement>([&](Entity, PlayerMovement &pm) {
            if (!pm.input_) {
                pm.input_ = &input;
            }
            if (!pm.gamepad_) {
                pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>();
            }
        });

        // UIInteractionSystemに、入力システムへの参照を渡す
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        // 発光マテリアルのパルス効果を更新
        RenderingSystem::GetInstance().UpdateEmissivePulse(world, deltaTime);

        // 遅延リスポーンのタイマーを更新
        UpdateDelayedRespawn(deltaTime, world);

        // カメラの揺れやズームなどのリアクションを更新
        UpdateCameraReaction(deltaTime, world);
        // ライトの状態を更新（カメラ位置に依存するライトもあるため）
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        // ECSワールドのTickを進め、各システムのUpdateを実行
        world.Tick(deltaTime);

        // プレイヤーが生存していれば、時間切れをチェック
        if (world.IsAlive(playerEntity_)) {
            CheckTimeLimit(world, playerEntity_, cfg_LimitTime);
        }
    }

    /**
     * @brief 毎フレーム呼び出される描画処理
     * @details レンダリングシステムを呼び出し、ゲームオブジェクトやUIを描画する
     * @param world ECSワールドへの参照
     */
    void OnRender(World &world) override {
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (gfx) {
            // ライト情報をGPUの定数バッファにバインド
            RenderingSystem::GetInstance().BindLightBuffer(gfx->Ctx(), 1);
        }

        // UIRenderSystemを実行してUIを描画
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.Render(world);
        });
    }

    /**
     * @brief シーン終了時に呼び出されるクリーンアップ処理
     * @details このシーンで生成したエンティティやリソースを破棄・解放する
     * @param world ECSワールドへの参照
     */
    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");

        // グローバルポインタをクリア
        g_GameScene = nullptr;

        // レンダリングシステムをシャットダウン
        RenderingSystem::GetInstance().Shutdown();

        // このシーンが所有するすべてのエンティティを破棄
        for (const auto &entity : ownedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        // テキスト・画像システムをシャットダウン
        textSystem_.Shutdown();
        imageSystem_.Shutdown();
        DEBUGLOG("GameWithUIScene のクリーンアップが完了しました");
    }

    // =========================================
    // カメラリアクション公開API
    // =========================================

    /**
     * @brief カメラシェイク（揺れ）を開始する
     * @param intensity 揺れの強さ
     * @param duration 持続時間（秒）
     */
    void TriggerCameraShake(float intensity, float duration) {
        reactionType_ = CameraReactionType::Shake;
        shakeIntensity_ = intensity;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f; // 経過時間をリセット
    }

    /**
     * @brief カメラインパルス（衝撃）を開始する
     * @param dirX 衝撃のX方向
     * @param dirY 衝撃のY方向
     * @param dirZ 衝撃のZ方向
     * @param intensity 衝撃の強さ
     * @param duration 持続時間（秒）
     */
    void TriggerCameraImpulse(float dirX, float dirY, float dirZ, float intensity, float duration) {
        reactionType_ = CameraReactionType::Impulse;
        impulseDir_ = {dirX, dirY, dirZ};
        impulseIntensity_ = intensity;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f; // 経過時間をリセット
    }

    /**
     * @brief カメラズームを開始する
     * @param zoomAmount ズーム量（視野角の減少量）。正の値でズームイン。
     * @param duration 持続時間（秒）
     */
    void TriggerCameraZoom(float zoomAmount, float duration) {
        reactionType_ = CameraReactionType::Zoom;
        zoomAmount_ = zoomAmount;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f; // 経過時間をリセット
    }

    /**
     * @brief 現在のカメラリアクションをすべて停止し、カメラを通常状態に戻す
     */
    void StopCameraReaction() {
        reactionType_ = CameraReactionType::None;
        camera_.position = cameraPosition_; // カメラ位置を基準位置に戻す
        camera_.fovY = baseFovY_;           // 視野角を基準値に戻す
        // プロジェクション行列を更新
        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update(); // ビュー行列、ビュープロジェクション行列を再計算
    }

    /**
     * @brief 現在のカメラオブジェクトへのconst参照を取得する
     * @return カメラオブジェクト
     */
    const Camera& GetCamera() const { return camera_; }

    /**
     * @brief カメラの基準位置を設定する
     * @param pos 新しい基準位置
     */
    void SetCameraBasePosition(const DirectX::XMFLOAT3& pos) {
        cameraPosition_ = pos;
    }

    /**
     * @brief 壁衝突時の処理（カメラシェイク＋遅延リスポーン）
     * @details プレイヤーの速度を止め、カメラを揺らし、一定時間後にリスポーン処理を予約する
     * @param player リスポーンするプレイヤーエンティティ
     */
    void OnWallHit(Entity player) {
        if (pendingRespawn_) return; // 既にリスポーン処理が進行中なら何もしない

        // プレイヤーの速度を即座に0にする
        if (world_ && world_->IsAlive(player)) {
            if (auto *v = world_->TryGet<PlayerVelocity>(player)) {
                v->velocity = {0.0f, 0.0f};
                v->isBoosting = false;
                v->isDecelerating = false;
                v->boostSpeed = 0.0f;
            }
        }

        // 設定ファイルから読み込んだ値でカメラシェイクを開始
        TriggerCameraShake(cfg_WallHitShakeIntensity.Get(), cfg_WallHitShakeDuration.Get());

        // 遅延リスポーンを設定
        pendingRespawn_ = true;
        respawnPlayer_ = player;
        respawnTimer_ = cfg_WallHitRespawnDelay.Get();

        DEBUGLOG("壁に衝突 - カメラシェイク開始、リスポーン待機中");
    }

    /**
     * @brief OnUpdate内で使用するために、ワールドへのポインタを設定する
     * @details OnWallHitなど、Updateループ外から呼び出される可能性があるメソッド内でワールドのエンティティを操作するために必要
     * @param w ワールドへのポインタ
     */
    void SetWorldRef(World* w) { world_ = w; }

  private:
    /**
     * @brief UIで使用するテキストフォーマット（フォント、サイズ、色など）を事前作成する
     */
    void CreateTextFormats();
    /**
     * @brief ゲーム内UI（スコア表示など）を生成する
     * @param world ワールドへの参照
     * @param screenWidth 画面の幅
     * @param screenHeight 画面の高さ
     */
    void CreateUI(World &world, float screenWidth, float screenHeight);

    /**
     * @brief プレイヤーエンティティを生成し、必要なコンポーネントを追加する
     * @param world ワールドへの参照
     */
    void CreatePlayer(World &world) {
        // 設定ファイルからスケール値を取得
        float s = cfg_PlayerScale;
        // 初期位置、回転、スケールを設定
        Transform transform{{0.0f, 0.0f, cfg_PlayerStartY}, {0.0f, 0.0f, 0.0f}, {s, s, s}};

        // プレイヤーエンティティを生成し、各種コンポーネントをアタッチ
        Entity player = world.Create()
                            .With<Transform>(transform)
                            .With<Model>(cfg_PlayerFBXPass)
                            .With<PlayerTag>()
                            .With<PlayerVelocity>()
                            .With<PlayerMovement>()
                            .With<PlayerGuide>()
                            .With<CollisionSphere>(0.4f)
                            .With<PlayerCollisionHandler>()
                            .Build();

        playerEntity_ = player;
        ownedEntities_.push_back(player);
    }

    /**
     * @brief ステージデータ（CSV）に基づいてステージのマップ（壁、スタート、ゴールなど）を生成する
     * @param world ワールドへの参照
     */
    void CreateStageMap(World &world) {
        // StageCreateコンポーネントを持つエンティティ（OnEnterで生成）からステージデータを取得
        world.ForEach<StageCreate>([&](Entity, StageCreate &stagecreate) {
            float tileSize = 1.0f; // 各タイルのサイズ

            // ステージデータが空なら処理を中断
            if (stagecreate.stageMap.empty() || stagecreate.stageMap[0].empty()) {
                return;
            }

            // マップの幅と高さをタイル数で計算
            float mapWidth = static_cast<float>(stagecreate.stageMap[0].size());
            float mapHeight = static_cast<float>(stagecreate.stageMap.size());

            // マップの最大インデックスを計算
            const int max_x_index = static_cast<int>(stagecreate.stageMap[0].size() - 1);
            const int max_y_index = static_cast<int>(stagecreate.stageMap.size() - 1);

            // マップの中心がワールド座標の原点(0,0,0)に来るようにオフセットを計算
            const float offsetX = (mapWidth * tileSize) * 0.5f - (tileSize * 0.5f);
            const float offsetZ = (mapHeight * tileSize) * 0.5f - (tileSize * 0.5f);

            // ステージマップに基づいてオブジェクトを生成
            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {
                    int blockType = stagecreate.stageMap[y][x];

                    // タイルの2Dインデックス(x, y)を3Dワールド座標に変換
                    float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                    float worldY = 0.0f;
                    float worldZ = offsetZ - (static_cast<float>(y) * tileSize); // Z軸は奥が正なので、yが大きくなるほどZは小さくなる

                    const DirectX::XMFLOAT3 blockposition = {worldX, worldY, worldZ};

                    // ステージの床を生成
                    CreateFloor(world, blockposition);

                    // ステージの境界には常に壁を生成
                    if (y == 0) {
                        CreatFloorWall(world, {worldX, worldY, worldZ + tileSize});
                    }
                    if (y == max_y_index) {
                        CreatFloorWall(world, {worldX, worldY, worldZ - tileSize});
                    }
                    if (x == 0) {
                        CreatFloorWall(world, {worldX - tileSize, worldY, worldZ});
                    }
                    if (x == max_x_index) {
                        CreatFloorWall(world, {worldX + tileSize, worldY, worldZ});
                    }

                    // ブロックタイプに応じてオブジェクトを生成
                    if (blockType != 0) { // 0は空白
                        switch (blockType) {
                            case 1:
                                CreateStart(world, blockposition);
                                break; // スタート地点
                            case 2:
                                CreateGoal(world, blockposition);
                                break; // ゴール地点
                            case 3:
                                CreateWall(world, blockposition);
                                break; // 通常の壁
                            case 5:
                                CreateRightDownCorner(world, blockposition);
                                break; // 右下が直角の三角形
                            case 6:
                                CreateLeftDownCorner(world, blockposition);
                                break; // 左下が直角の三角形
                            case 7:
                                CreateLeftUpCorner(world, blockposition);
                                break; // 左上が直角の三角形
                            case 8:
                                CreateRightUpCorner(world, blockposition);
                                break; // 右上が直角の三角形
                            case 10:
                            case 11:
                            case 12:
                            case 13:
                            case 14:
                            case 15:
                            case 16:
                            case 17:
                            case 18:
                            case 19:
                                if (blockType >= 10 && blockType < 20) {
                                    CreateMoveWall(world, blockposition, blockType);
                                    break; // 動く障害物10
                                }
                            case 30:
                            case 31:
                            case 32:
                            case 33:
                            case 34:
                            case 35:
                            case 36:
                            case 37:
                            case 38:
                            case 39:
                                if (blockType >= 30 && blockType < 40) {
                                    CreateDashBoard(world, blockposition, blockType);
                                    break; // 加速板10
                                }
                            default:
                                break; 
                        }
                    }
                }
            }
        });
    }

    void CreateFloor(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 floorPos = {
            position.x,
            position.y - 1,
            position.z,
        };

        Transform transform{{floorPos}, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_FloorThickness, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        //renderer.color = DirectX::XMFLOAT3{cfg_FloorR, cfg_FloorG, cfg_FloorB};

        Entity floor = world.Create()
                           .With<Transform>(transform)
                           .With<Model>(cfg_FloorFBXPass)
                           .With<MeshRenderer>(renderer)
                           .Build();

        ownedEntities_.push_back(floor);
    }

    /**
     * @brief スタート地点のオブジェクトを生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateStart(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition;
        diffPosition.x = position.x;
        diffPosition.y = position.y - 1.0f; // 少し下に配置
        diffPosition.z = position.z;

        Transform t{diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_StartR, cfg_StartG, cfg_StartB};

        // 発光マテリアル
        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_StartEmissiveR, cfg_StartEmissiveG, cfg_StartEmissiveB},
            cfg_StartEmissiveIntensity};

        // 発光の強さを脈動させるコンポーネント
        EmissivePulse pulse{cfg_StartPulseMin, cfg_StartPulseMax, cfg_StartPulseSpeed};

        // ポイントライト
        PointLight light{
            DirectX::XMFLOAT3{cfg_StartEmissiveR, cfg_StartEmissiveG, cfg_StartEmissiveB},
            cfg_StartEmissiveIntensity,
            cfg_StartLightRange};

        // エンティティを生成し、各種コンポーネントをアタッチ
        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<EmissiveMaterial>(emissive)
                       .With<EmissivePulse>(pulse)
                       .With<PointLight>(light)
                       .With<StartTag>() // スタート地点識別用タグ
                       .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                       .Build();

        // 生成したエンティティをステージオブジェクトとして管理
        startEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    /**
     * @brief ゴール地点のオブジェクトを生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateGoal(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition;
        diffPosition.x = position.x;
        diffPosition.y = position.y - 1.0f; // 少し下に配置
        diffPosition.z = position.z;

        Transform t{diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_GoalR, cfg_GoalG, cfg_GoalB};

        // 発光マテリアル
        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR, cfg_GoalEmissiveG, cfg_GoalEmissiveB},
            cfg_GoalEmissiveIntensity};

        // 発光の強さを脈動させるコンポーネント
        EmissivePulse pulse{cfg_GoalPulseMin, cfg_GoalPulseMax, cfg_GoalPulseSpeed};

        // ポイントライト
        PointLight light{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR, cfg_GoalEmissiveG, cfg_GoalEmissiveB},
            cfg_GoalEmissiveIntensity,
            cfg_GoalLightRange};

        // エンティティを生成し、各種コンポーネントをアタッチ
        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<EmissiveMaterial>(emissive)
                       .With<EmissivePulse>(pulse)
                       .With<PointLight>(light)
                       .With<GoalTag>() // ゴール地点識別用タグ
                       .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                       .Build();

        // 生成したエンティティをステージオブジェクトとして管理
        goalEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    /**
     * @brief 通常の壁オブジェクト（四角柱）を生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        // renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_WallFBXPass)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>() // 壁識別用タグ
                                .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>() // 壁専用の衝突ハンドラ
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    /**
     * @brief 右下コーナー用の壁オブジェクト（三角柱）を生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateRightDownCorner(World &world, const DirectX::XMFLOAT3 &position) {
        // 適切な向きになるように回転を設定
        Transform transform{position, {0.0f, 0.0f, 180.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::RightIsoTriPrism; // 直角二等辺三角柱メッシュ
        renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionRightIsoTriPrism>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f}) // 三角柱用の当たり判定
                                .With<WallCollisionHandler>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    /**
     * @brief 左下コーナー用の壁オブジェクト（三角柱）を生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateLeftDownCorner(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::RightIsoTriPrism;
        renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionRightIsoTriPrism>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    /**
     * @brief 左上コーナー用の壁オブジェクト（三角柱）を生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateLeftUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
        // 適切な向きになるように回転を設定
        Transform transform{position, {0.0f, 90.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::RightIsoTriPrism;
        renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionRightIsoTriPrism>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    /**
     * @brief 右上コーナー用の壁オブジェクト（三角柱）を生成する
     * @param world ワールドへの参照
     * @param position 生成する位置
     */
    void CreateRightUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
        // 適切な向きになるように回転を設定
        Transform transform{position, {0.0f, 180.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::RightIsoTriPrism;
        renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionRightIsoTriPrism>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateMoveWall(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{cfg_WallR, cfg_WallG, cfg_WallB};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>()
                                .Build();


        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity worldwallEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<MeshRenderer>(renderer)
                                     .With<WallTag>()
                                     .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                     .With<FloorWallCollisionHandler>()
                                     .Build();

        stageOwnedEntities_.push_back(worldwallEntity);
    }

    //加速板
        void CreateDashBoard(World & world, const DirectX::XMFLOAT3 &position, int blockType) {
            Transform transform{{position}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
            MeshRenderer renderer;

            renderer.meshType = MeshType::Cube;
            renderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f};

            DashBoardStatus status;
            status.blockID = blockType;
            float angle = 0.0f;

            world.ForEach<LoadAngle>([&](Entity, LoadAngle &loadAngle) {
                for (const auto &row : loadAngle.stageAngle) {
                    if (row.size() > 1) {
                        angle = static_cast<float>(row[1]);
                        break;
                    }
                }
            });

            status.accelAngle = angle;

            Entity dashBoardEntity = world.Create()
                                         .With<Transform>(transform)
                                         .With<MeshRenderer>(renderer)
                                         .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                         .With<GimmickTag>()
                                         .With<DashBordCollisionHandler>()
                                         .With<DashBoardStatus>(status)
                                         .Build();

            stageOwnedEntities_.push_back(dashBoardEntity);
        }

        void SetupStage(World & world, int stage) {
            // ステージリセット: 現在のステージに関連するエンティティを破棄
            for (const auto &entity : stageOwnedEntities_) {
                if (world.IsAlive(entity)) {
                    world.DestroyEntityWithCause(entity, World::Cause::StageReset);
                }
            }
            stageOwnedEntities_.clear();

            startEntity_ = {};
            goalEntity_ = {};

            // 新しいステージのマップを生成
            CreateStageMap(world);

            // 簡易ライトベイク（アンビエント＋ディレクショナル＋ポイントライトの影なし近似）
            BakeStageLighting(world);

            // プレイヤーをスタート地点にリセット
            if (world.IsAlive(playerEntity_)) {
                ResetPlayerToStart(world, playerEntity_);
            }
        }

        // 簡易ライトベイク（影なし近似）: ステージ生成直後にメッシュカラーへ反映
        void BakeStageLighting(World & world) {
            using namespace DirectX;

            // 1) 環境光
            XMFLOAT3 ambient{0.15f, 0.15f, 0.2f};
            float ambientIntensity = 1.0f;

            // 2) ディレクショナルライト（最初の1つ）
            bool hasDir = false;
            XMFLOAT3 dirDir{0.0f, -1.0f, 0.0f};
            XMFLOAT3 dirColor{1.0f, 1.0f, 1.0f};
            world.ForEach<DirectionalLight>([&](Entity, DirectionalLight &dl) {
                dirDir = dl.direction;
                dirColor = XMFLOAT3{dl.color.x, dl.color.y, dl.color.z};
                hasDir = true;
            });

            // 3) ポイントライト収集
            struct PL {
                XMFLOAT3 pos;
                XMFLOAT3 col;
                float range;
                float I;
                float kc, kl, kq;
            };
            std::vector<PL> lights;
            world.ForEach<Transform, PointLight>([&](Entity, Transform &t, PointLight &pl) {
                if (!pl.enabled)
                    return;
                lights.push_back(PL{t.position, pl.color, pl.range, pl.intensity, pl.constantAttenuation, pl.linearAttenuation, pl.quadraticAttenuation});
            });

            auto saturate = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
            auto mul3 = [](const XMFLOAT3 &a, const XMFLOAT3 &b) { return XMFLOAT3{a.x * b.x, a.y * b.y, a.z * b.z}; };
            auto add3 = [](const XMFLOAT3 &a, const XMFLOAT3 &b) { return XMFLOAT3{a.x + b.x, a.y + b.y, a.z + b.z}; };
            auto scale3 = [](const XMFLOAT3 &a, float s) { return XMFLOAT3{a.x * s, a.y * s, a.z * s}; };

            // Up法線で近似（床や壁の簡易ベイク）
            XMFLOAT3 upN{0.0f, 1.0f, 0.0f};
            XMVECTOR vUp = XMLoadFloat3(&upN);
            XMVECTOR vDir = XMLoadFloat3(&dirDir);
            if (hasDir) {
                vDir = XMVector3Normalize(vDir);
            }

            for (auto e : stageOwnedEntities_) {
                if (!world.IsAlive(e))
                    continue;
                auto *t = world.TryGet<Transform>(e);
                auto *mr = world.TryGet<MeshRenderer>(e);
                if (!t || !mr)
                    continue;

                // ベースカラー
                XMFLOAT3 base = mr->color;

                // アンビエント
                XMFLOAT3 accum = scale3(ambient, ambientIntensity);

                // ディレクショナル（Up法線近似）
                if (hasDir) {
                    XMVECTOR n = vUp;
                    float ndotl = saturate(DirectX::XMVectorGetX(DirectX::XMVector3Dot(n, XMVectorNegate(vDir))));
                    accum = add3(accum, scale3(dirColor, ndotl));
                }

                // ポイントライト（位置のみ、法線考慮なし・距離減衰のみ）
                XMVECTOR pos = XMLoadFloat3(&t->position);
                for (const auto &L : lights) {
                    XMVECTOR lp = XMLoadFloat3(&L.pos);
                    XMVECTOR d = XMVectorSubtract(lp, pos);
                    float dist = XMVectorGetX(XMVector3Length(d));
                    if (dist > L.range)
                        continue;
                    float att = 1.0f / std::max(1e-4f, L.kc + L.kl * dist + L.kq * dist * dist);
                    accum = add3(accum, scale3(L.col, L.I * att));
                }

                // ベースカラーへ乗算し、0..1にクランプ
                XMFLOAT3 shaded = mul3(base, accum);
                shaded.x = saturate(shaded.x);
                shaded.y = saturate(shaded.y);
                shaded.z = saturate(shaded.z);
                mr->color = shaded;
            }
        }

        TextSystem textSystem_;
        ImageSystem imageSystem_;
        std::vector<Entity> ownedEntities_;
        std::vector<Entity> stageOwnedEntities_;
        Entity playerEntity_{};
        Entity stageEntity_{};
        Entity startEntity_{};
        Entity wall_{};
        Entity worldwall_{};
        Entity goalEntity_{};
        Entity gimmickEntity_{};
        DirectX::XMFLOAT3 cameraPosition_ = {0.0f, 10.0f, -10.0f};
    };
