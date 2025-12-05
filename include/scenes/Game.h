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
 */
inline void ResetPlayerToStart(World &w, Entity player, bool resetTimer = false) {
    if (!w.IsAlive(player)) {
        return;
    }
    
    bool done = false;
    w.ForEach<StartTag, Transform>([&](Entity, StartTag &, Transform &tStart) {
        if (done) {
            return;
        }

        if (auto *tPlayer = w.TryGet<Transform>(player)) {
            tPlayer->position = {tStart.position.x, 0.0f, tStart.position.z};

            // 速度を完全リセット
            if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
                vPlayer->velocity = {0.0f, 0.0f};
                vPlayer->isBoosting = false;
                vPlayer->isDecelerating = false;
                vPlayer->boostSpeed = 0.0f;
                vPlayer->boostDir = {0.0f, 0.0f};
            }

            // PlayerMovementの状態もリセット
            if (auto *pmPlayer = w.TryGet<PlayerMovement>(player)) {
                pmPlayer->isCharging_ = false;
                pmPlayer->wasCharging_ = false;
                pmPlayer->wasChargingPrev_ = false;
                pmPlayer->ResetAngleHistory();
            }
        }
        
        if (resetTimer) {
            w.ForEach<GameStatus>([](Entity, GameStatus &stats) {
                stats.elapsedTime = 0.0f;
            });
        }

        done = true;
    });
}

inline void CheckTimeLimit(World &w, Entity player, float timeLimitSeconds) {
    w.ForEach<GameStatus>([&](Entity e, GameStatus &stats) {
        if (stats.elapsedTime >= timeLimitSeconds) {
            DEBUGLOG("時間切れ");
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
inline ConfigVar<float> cfg_WallHitShakeIntensity{"Game", "WallHitShakeIntensity", 0.5f};
inline ConfigVar<float> cfg_WallHitShakeDuration{"Game", "WallHitShakeDuration", 0.3f};
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
        auto *v = w.TryGet<PlayerVelocity>(other);
        if (w.Has<PlayerTag>(other)) {
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
    inline static ConfigVar<float> cfg_CameraShakeFreqX{"Camera", "ShakeFreqX", 35.0f};
    inline static ConfigVar<float> cfg_CameraShakeFreqY{"Camera", "ShakeFreqY", 28.0f};
    inline static ConfigVar<float> cfg_CameraShakeFreqZ{"Camera", "ShakeFreqZ", 41.0f};
    inline static ConfigVar<float> cfg_CameraShakeDecay{"Camera", "ShakeDecay", 3.0f};
    inline static ConfigVar<float> cfg_CameraShakeRandomness{"Camera", "ShakeRandomness", 0.3f};
    inline static ConfigVar<float> cfg_CameraShakeYScale{"Camera", "ShakeYScale", 0.5f};
    inline static ConfigVar<float> cfg_CameraImpulseDecay{"Camera", "ImpulseDecay", 5.0f};
    inline static ConfigVar<float> cfg_CameraZoomSpeed{"Camera", "ZoomSpeed", 2.0f};
    inline static ConfigVar<float> cfg_CameraFollowSmooth{"Camera", "FollowSmooth", 5.0f};

    void OnEnter(World &world) override {
        DEBUGLOG("<<<<< GameScene::OnEnter CALLED! >>>>>");
        DEBUGLOG("GameWithUIScene::OnEnter() 開始");

        // グローバルポインタを設定
        g_GameScene = this;

        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("GfxDevice が見つかりません");
            return;
        }

        RenderingSystem::GetInstance().Initialize(gfx->Dev());
        RenderingSystem::GetInstance().SetAmbientLight({0.1f, 0.1f, 0.15f}, 1.0f);

        if (!textSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("TextSystem の初期化に失敗しました");
            return;
        }

        if (!imageSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("ImageSystem の初期化に失敗しました");
            return;
        }

        CreateTextFormats();

        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        // カメラ初期化
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4,
            screenWidth / screenHeight,
            0.1f,
            1000.0f,
            cameraPosition_,
            DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f}
        );
        baseFovY_ = camera_.fovY;

        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>(cfg_CollisionCellSize.Get()).Build();
        ownedEntities_.push_back(collisionSystem);

        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        world.ForEach<StageProgress>([&](Entity e, StageProgress &status) {
            std::string Stagepath = "Assets/StageData/StageCollision/DebugStage" + std::to_string(status.selectStage) + "/room1.csv";
            Entity stageEntity = world.Create().With<StageCreate>(Stagepath).Build();
            ownedEntities_.push_back(stageEntity);
        });

        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        CreateUI(world, screenWidth, screenHeight);

        SetupStage(world, 1);

        DEBUGLOG("GameWithUIScene の初期化が正常に完了しました");
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {

        world.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
            if (input.GetKeyDown(VK_ESCAPE) || input.GetKeyDown('P')) {
                stats.isPaused = !stats.isPaused;
                DEBUGLOG(stats.isPaused ? "ゲームが一時停止されました" : "ゲームが再開されました");
            }

            if (stats.isPaused) {
                deltaTime = 0.0f;
            }
        });

        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.requestAdvance) {
                sp.requestAdvance = false;
                sp.currentStage++;
                DEBUGLOG("ステージが進行しました: " + std::to_string(sp.currentStage));
                SetupStage(world, sp.currentStage);
            }
        });

        world.ForEach<PlayerMovement>([&](Entity, PlayerMovement &pm) {
            if (!pm.input_) {
                pm.input_ = &input;
            }
            if (!pm.gamepad_) {
                pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>();
            }
        });

        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        RenderingSystem::GetInstance().UpdateEmissivePulse(world, deltaTime);

        // 遅延リスポーン処理
        UpdateDelayedRespawn(deltaTime, world);

        // カメラリアクション更新
        UpdateCameraReaction(deltaTime, world);
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        world.Tick(deltaTime);

        if (world.IsAlive(playerEntity_)) {
            CheckTimeLimit(world, playerEntity_, cfg_LimitTime);
        }
    }

    void OnRender(World &world) override {
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (gfx) {
            RenderingSystem::GetInstance().BindLightBuffer(gfx->Ctx(), 1);
        }

        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.Render(world);
        });
    }

    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");

        // グローバルポインタをクリア
        g_GameScene = nullptr;

        RenderingSystem::GetInstance().Shutdown();

        for (const auto &entity : ownedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        textSystem_.Shutdown();
        imageSystem_.Shutdown();
        DEBUGLOG("GameWithUIScene のクリーンアップが完了しました");
    }

    // =========================================
    // カメラリアクション公開API
    // =========================================

    void TriggerCameraShake(float intensity, float duration) {
        reactionType_ = CameraReactionType::Shake;
        shakeIntensity_ = intensity;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f;
    }

    void TriggerCameraImpulse(float dirX, float dirY, float dirZ, float intensity, float duration) {
        reactionType_ = CameraReactionType::Impulse;
        impulseDir_ = {dirX, dirY, dirZ};
        impulseIntensity_ = intensity;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f;
    }

    void TriggerCameraZoom(float zoomAmount, float duration) {
        reactionType_ = CameraReactionType::Zoom;
        zoomAmount_ = zoomAmount;
        reactionTime_ = duration;
        reactionElapsed_ = 0.0f;
    }

    void StopCameraReaction() {
        reactionType_ = CameraReactionType::None;
        camera_.position = cameraPosition_;
        camera_.fovY = baseFovY_;
        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    const Camera& GetCamera() const { return camera_; }

    void SetCameraBasePosition(const DirectX::XMFLOAT3& pos) {
        cameraPosition_ = pos;
    }

    /**
     * @brief 壁衝突時の処理（カメラシェイク＋遅延リスポーン）
     * @param player リスポーンするプレイヤーエンティティ
     */
    void OnWallHit(Entity player) {
        if (pendingRespawn_) return; // 既にリスポーン待ち中

        // プレイヤーの速度を即座に止める
        if (world_ && world_->IsAlive(player)) {
            if (auto *v = world_->TryGet<PlayerVelocity>(player)) {
                v->velocity = {0.0f, 0.0f};
                v->isBoosting = false;
                v->isDecelerating = false;
                v->boostSpeed = 0.0f;
            }
        }

        // カメラシェイク開始
        TriggerCameraShake(cfg_WallHitShakeIntensity.Get(), cfg_WallHitShakeDuration.Get());

        // 遅延リスポーン設定
        pendingRespawn_ = true;
        respawnPlayer_ = player;
        respawnTimer_ = cfg_WallHitRespawnDelay.Get();

        DEBUGLOG("壁に衝突 - カメラシェイク開始、リスポーン待機中");
    }

    // Worldへの参照を設定（OnUpdate内で使用）
    void SetWorldRef(World* w) { world_ = w; }

  private:
    void CreateTextFormats();
    void CreateUI(World &world, float screenWidth, float screenHeight);

    void CreatePlayer(World &world) {
        float s = cfg_PlayerScale;
        Transform transform{{0.0f, 0.0f, cfg_PlayerStartY}, {0.0f, 0.0f, 0.0f}, {s, s, s}};

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

    void CreateStageMap(World &world) {
        world.ForEach<StageCreate>([&](Entity, StageCreate &stagecreate) {
            float tileSize = 1.0f;

            if (stagecreate.stageMap.empty() || stagecreate.stageMap[0].empty()) {
                return;
            }

            float mapWidth = static_cast<float>(stagecreate.stageMap[0].size());
            float mapHeight = static_cast<float>(stagecreate.stageMap.size());

            const int max_x_index = static_cast<int>(stagecreate.stageMap[0].size() - 1);
            const int max_y_index = static_cast<int>(stagecreate.stageMap.size() - 1);

            const float offsetX = (mapWidth * tileSize) * 0.5f - (tileSize * 0.5f);
            const float offsetZ = (mapHeight * tileSize) * 0.5f - (tileSize * 0.5f);

            CreateFloor(world, static_cast<int>(mapWidth), tileSize);

            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {
                    int blockType = stagecreate.stageMap[y][x];

                    float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                    float worldY = 0.0f;
                    float worldZ = offsetZ - (static_cast<float>(y) * tileSize);

                    const DirectX::XMFLOAT3 blockposition = {worldX, worldY, worldZ};

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

                    if (blockType != 0) {
                        switch (blockType) {
                            case 1: CreateStart(world, blockposition); break;
                            case 2: CreateGoal(world, blockposition); break;
                            case 3: CreateWall(world, blockposition); break;
                            case 5: CreateRightDownCorner(world, blockposition); break;
                            case 6: CreateLeftDownCorner(world, blockposition); break;
                            case 7: CreateLeftUpCorner(world, blockposition); break;
                            case 8: CreateRightUpCorner(world, blockposition); break;
                        }
                    }
                }
            }
        });
    }

    void CreateFloor(World &world, int gridSize, float tileSize) {
        if (gridSize <= 0.0f || tileSize <= 0.0f) {
            return;
        }

        const float yOffset = cfg_FloorYOffset;
        const float half = (gridSize * tileSize) * 0.5f;

        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                float x = i * tileSize - half + tileSize * 0.5f;
                float z = j * tileSize - half + tileSize * 0.5f;

                Transform transform{{x, yOffset, z}, {0.0f, 0.0f, 0.0f}, {tileSize, cfg_FloorThickness, tileSize}};
                MeshRenderer renderer;
                renderer.meshType = MeshType::Cube;
                renderer.color = DirectX::XMFLOAT3{cfg_FloorR, cfg_FloorG, cfg_FloorB};

                Entity floor = world.Create()
                                   .With<Transform>(transform)
                                   .With<MeshRenderer>(renderer)
                                   .Build();

                ownedEntities_.push_back(floor);
            }
        }
    }

    void CreateStart(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition;
        diffPosition.x = position.x;
        diffPosition.y = position.y - 1.0f;
        diffPosition.z = position.z;

        Transform t{diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_StartR, cfg_StartG, cfg_StartB};

        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_StartEmissiveR, cfg_StartEmissiveG, cfg_StartEmissiveB},
            cfg_StartEmissiveIntensity};

        EmissivePulse pulse{cfg_StartPulseMin, cfg_StartPulseMax, cfg_StartPulseSpeed};

        PointLight light{
            DirectX::XMFLOAT3{cfg_StartEmissiveR, cfg_StartEmissiveG, cfg_StartEmissiveB},
            cfg_StartEmissiveIntensity,
            cfg_StartLightRange};

        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<EmissiveMaterial>(emissive)
                       .With<EmissivePulse>(pulse)
                       .With<PointLight>(light)
                       .With<StartTag>()
                       .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                       .Build();

        startEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateGoal(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition;
        diffPosition.x = position.x;
        diffPosition.y = position.y - 1.0f;
        diffPosition.z = position.z;

        Transform t{diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_GoalR, cfg_GoalG, cfg_GoalB};

        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR, cfg_GoalEmissiveG, cfg_GoalEmissiveB},
            cfg_GoalEmissiveIntensity};

        EmissivePulse pulse{cfg_GoalPulseMin, cfg_GoalPulseMax, cfg_GoalPulseSpeed};

        PointLight light{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR, cfg_GoalEmissiveG, cfg_GoalEmissiveB},
            cfg_GoalEmissiveIntensity,
            cfg_GoalLightRange};

        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<EmissiveMaterial>(emissive)
                       .With<EmissivePulse>(pulse)
                       .With<PointLight>(light)
                       .With<GoalTag>()
                       .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                       .Build();

        goalEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
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

    void CreateRightDownCorner(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 180.0f}, {1.0f, cfg_WallSize, 1.0f}};
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

    void CreateLeftUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
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

    void CreateRightUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
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

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{cfg_FloorWallR, cfg_FloorWallG, cfg_FloorWallB};

        Entity worldwallEntity = world.Create()
            .With<Transform>(transform)
            .With<MeshRenderer>(renderer)
            .With<WallTag>()
            .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
            .With<FloorWallCollisionHandler>()
            .Build();

        stageOwnedEntities_.push_back(worldwallEntity);
    }

    void CreateDashBord(World& world) {
        Transform transform{{-7.0f,0.0f,0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;

        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f};

        Entity dashBordEntity = world.Create()
                                    .With<Transform>(transform)
                                    .With<MeshRenderer>(renderer)
                                    .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                    .With<GimmickTag>()
                                    .With<DashBordCollisionHandler>()
                                    .Build();

        stageOwnedEntities_.push_back(dashBordEntity);
    }

    void SetupStage(World &world, int stage) {
        for (const auto &entity : stageOwnedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::StageReset);
            }
        }
        stageOwnedEntities_.clear();

        startEntity_ = {};
        goalEntity_ = {};

        CreateStageMap(world);
        CreateDashBord(world);
        BakeStageLighting(world);

        if (world.IsAlive(playerEntity_)) {
            ResetPlayerToStart(world, playerEntity_);
        }
    }

    void BakeStageLighting(World &world) {
        using namespace DirectX;

        XMFLOAT3 ambient{0.15f, 0.15f, 0.2f};
        float ambientIntensity = 1.0f;

        bool hasDir = false;
        XMFLOAT3 dirDir{0.0f, -1.0f, 0.0f};
        XMFLOAT3 dirColor{1.0f, 1.0f, 1.0f};
        world.ForEach<DirectionalLight>([&](Entity, DirectionalLight &dl) {
            dirDir = dl.direction;
            dirColor = XMFLOAT3{dl.color.x, dl.color.y, dl.color.z};
            hasDir = true;
        });

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

            XMFLOAT3 base = mr->color;
            XMFLOAT3 accum = scale3(ambient, ambientIntensity);

            if (hasDir) {
                XMVECTOR n = vUp;
                float ndotl = saturate(DirectX::XMVectorGetX(DirectX::XMVector3Dot(n, XMVectorNegate(vDir))));
                accum = add3(accum, scale3(dirColor, ndotl));
            }

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

            XMFLOAT3 shaded = mul3(base, accum);
            shaded.x = saturate(shaded.x);
            shaded.y = saturate(shaded.y);
            shaded.z = saturate(shaded.z);
            mr->color = shaded;
        }
    }

    // =========================================
    // 遅延リスポーン処理
    // =========================================
    void UpdateDelayedRespawn(float dt, World& world) {
        world_ = &world; // World参照を保持

        if (!pendingRespawn_) return;

        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            // リスポーン実行
            ResetPlayerToStart(world, respawnPlayer_, true);
            pendingRespawn_ = false;
            respawnPlayer_ = {};
            DEBUGLOG("リスポーン完了");
        }
    }

    // =========================================
    // カメラリアクションの内部更新
    // =========================================
    void UpdateCameraReaction(float dt, World& world) {
        DirectX::XMFLOAT3 playerPos = {0.0f, 0.0f, 0.0f};
        if (world.IsAlive(playerEntity_)) {
            if (auto* pt = world.TryGet<Transform>(playerEntity_)) {
                playerPos = pt->position;
            }
        }

        float smooth = cfg_CameraFollowSmooth.Get() * dt;
        smooth = std::min(smooth, 1.0f);
        currentTarget_.x += (playerPos.x - currentTarget_.x) * smooth;
        currentTarget_.y += (playerPos.y - currentTarget_.y) * smooth;
        currentTarget_.z += (playerPos.z - currentTarget_.z) * smooth;

        camera_.target = currentTarget_;

        switch (reactionType_) {
            case CameraReactionType::Shake:
                UpdateShake(dt);
                break;
            case CameraReactionType::Impulse:
                UpdateImpulse(dt);
                break;
            case CameraReactionType::Zoom:
                UpdateZoom(dt);
                break;
            case CameraReactionType::None:
            default:
                camera_.position = cameraPosition_;
                camera_.fovY = baseFovY_;
                break;
        }

        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    void UpdateShake(float dt) {
        reactionElapsed_ += dt;

        if (reactionElapsed_ >= reactionTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.position = cameraPosition_;
            return;
        }

        float decay = cfg_CameraShakeDecay.Get();
        float decayFactor = std::exp(-decay * reactionElapsed_);
        float currentIntensity = shakeIntensity_ * decayFactor;

        float freqX = cfg_CameraShakeFreqX.Get();
        float freqY = cfg_CameraShakeFreqY.Get();
        float freqZ = cfg_CameraShakeFreqZ.Get();
        float randomness = cfg_CameraShakeRandomness.Get();
        float yScale = cfg_CameraShakeYScale.Get();

        float randX = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;
        float randY = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;
        float randZ = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;

        float sx = (std::sin(reactionElapsed_ * freqX) * (1.0f - randomness) + randX) * currentIntensity;
        float sy = (std::cos(reactionElapsed_ * freqY) * (1.0f - randomness) + randY) * currentIntensity * yScale;
        float sz = (std::sin(reactionElapsed_ * freqZ) * (1.0f - randomness) + randZ) * currentIntensity;

        camera_.position = {
            cameraPosition_.x + sx,
            cameraPosition_.y + sy,
            cameraPosition_.z + sz
        };
    }

    void UpdateImpulse(float dt) {
        reactionElapsed_ += dt;

        if (reactionElapsed_ >= reactionTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.position = cameraPosition_;
            return;
        }

        float decay = cfg_CameraImpulseDecay.Get();
        float t = reactionElapsed_ / reactionTime_;
        float bounce = std::cos(t * DirectX::XM_PI * 2.0f) * std::exp(-decay * reactionElapsed_);
        float currentIntensity = impulseIntensity_ * bounce;

        camera_.position = {
            cameraPosition_.x + impulseDir_.x * currentIntensity,
            cameraPosition_.y + impulseDir_.y * currentIntensity,
            cameraPosition_.z + impulseDir_.z * currentIntensity
        };
    }

    void UpdateZoom(float dt) {
        reactionElapsed_ += dt;

        if (reactionElapsed_ >= reactionTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.fovY = baseFovY_;
            return;
        }

        float t = reactionElapsed_ / reactionTime_;
        float zoomCurve = std::sin(t * DirectX::XM_PI);
        camera_.fovY = baseFovY_ - zoomAmount_ * zoomCurve;

        camera_.fovY = std::max(DirectX::XM_PIDIV4 * 0.25f, std::min(DirectX::XM_PIDIV2 * 1.5f, camera_.fovY));
        camera_.position = cameraPosition_;
    }

    // =========================================
    // メンバー変数
    // =========================================
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
    DirectX::XMFLOAT3 cameraPosition_ = { 0.0f, 10.0f, -10.0f };
    DirectX::XMFLOAT3 currentTarget_ = {0.0f, 0.0f, 0.0f};
    Camera camera_{};
    float baseFovY_ = DirectX::XM_PIDIV4;

    // カメラリアクション状態
    CameraReactionType reactionType_ = CameraReactionType::None;
    float reactionTime_ = 0.0f;
    float reactionElapsed_ = 0.0f;

    // シェイク用
    float shakeIntensity_ = 0.0f;

    // 衝撃用
    DirectX::XMFLOAT3 impulseDir_ = {0.0f, 0.0f, 0.0f};
    float impulseIntensity_ = 0.0f;

    // ズーム用
    float zoomAmount_ = 0.0f;

    // 遅延リスポーン用
    bool pendingRespawn_ = false;
    Entity respawnPlayer_{};
    float respawnTimer_ = 0.0f;
    World* world_ = nullptr;
};

// =========================================
// 衝突ハンドラーの実装（GameScene定義後）
// =========================================
inline void WallCollisionHandler::OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {
    if (w.Has<PlayerTag>(other)) {
        DEBUGLOG("壁がプレイヤーと衝突 - カメラシェイク＋遅延リスポーン");
        if (g_GameScene) {
            g_GameScene->OnWallHit(other);
        } else {
            // フォールバック：即座にリスポーン
            ResetPlayerToStart(w, other, true);
        }
    }
}

inline void FloorWallCollisionHandler::OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {
    if (w.Has<PlayerTag>(other)) {
        DEBUGLOG("ステージ壁がプレイヤーと衝突 - カメラシェイク＋遅延リスポーン");
        if (g_GameScene) {
            g_GameScene->OnWallHit(other);
        } else {
            // フォールバック：即座にリスポーン
            ResetPlayerToStart(w, other, true);
        }
    }
}
