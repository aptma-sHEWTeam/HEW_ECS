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
#include <cmath>

// リファクタリング: 分離されたヘッダーをインクルード
#include "scenes/CameraReaction.h"
#include "scenes/StageConfig.h"
#include "scenes/CollisionHandlers.h"

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

/**
 * @class GameScene
 * @brief 3DゲームとUIを統合したシーン
 */
class GameScene : public IScene {
  public:
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

        // カメラを初期化
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4,
            screenWidth / screenHeight,
            0.1f,
            1000.0f,
            cameraPosition_,
            baseTarget_,
            DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f}
        );
        baseFovY_ = camera_.fovY;

        // 衝突検出システムをエンティティとして生成
        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>(cfg_CollisionCellSize.Get()).Build();
        ownedEntities_.push_back(collisionSystem);

        // モデル読み込みシステムをエンティティとして生成
        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        // ステージ進行状況に応じてステージデータを読み込む
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
        HandleStageAdvance(world);

        // 入力システムの参照を設定
        SetupInputReferences(world, input);

        // 発光マテリアルのパルス効果を更新
        RenderingSystem::GetInstance().UpdateEmissivePulse(world, deltaTime);

        // 遅延リスポーンのタイマーを更新
        UpdateDelayedRespawn(deltaTime, world);

        // カメラリアクションを更新
        UpdateCameraReaction(deltaTime, world);
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        // ECSワールドのTickを進める
        world.Tick(deltaTime);

        // 時間切れチェック
        if (world.IsAlive(playerEntity_)) {
            CheckTimeLimit(world, playerEntity_, cfg_LimitTime);
        }
    }

    /**
     * @brief 毎フレーム呼び出される描画処理
     * @param world ECSワールドへの参照
     */
    void OnRender(World &world) override {
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (gfx) {
            RenderingSystem::GetInstance().BindLightBuffer(gfx->Ctx(), 1);
        }

        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.Render(world);
        });
    }

    /**
     * @brief シーン終了時に呼び出されるクリーンアップ処理
     * @param world ECSワールドへの参照
     */
    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");

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

    /**
     * @brief カメラシェイク（揺れ）を開始する
     * @param intensity 揺れの強さ
     * @param duration 持続時間（秒）
     */
    void TriggerCameraShake(float intensity, float duration) {
        reactionType_ = CameraReactionType::Shake;
        shakeIntensity_ = intensity;
        shakeTime_ = duration;
        shakeElapsed_ = 0.0f;
    }

    /**
     * @brief カメラインパルス（衝撃）を開始する
     */
    void TriggerCameraImpulse(float dirX, float dirY, float dirZ, float intensity, float duration) {
        reactionType_ = CameraReactionType::Impulse;
        impulseDir_ = {dirX, dirY, dirZ};
        impulseIntensity_ = intensity;
        impulseTime_ = duration * 1.5f;
        impulseElapsed_ = 0.0f;
    }

    /**
     * @brief カメラズームを開始する
     */
    void TriggerCameraZoom(float zoomAmount, float duration) {
        reactionType_ = CameraReactionType::Zoom;
        zoomAmount_ = zoomAmount;
        zoomTime_ = duration;
        zoomElapsed_ = 0.0f;
    }

    /**
     * @brief 現在のカメラリアクションをすべて停止
     */
    void StopCameraReaction() {
        reactionType_ = CameraReactionType::None;
        camera_.position = cameraPosition_;
        camera_.fovY = baseFovY_;
        shakeElapsed_ = shakeTime_ = 0.0f;
        impulseElapsed_ = impulseTime_ = 0.0f;
        zoomElapsed_ = zoomTime_ = 0.0f;
        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    /** @brief カメラオブジェクトへのconst参照を取得 */
    const Camera& GetCamera() const { return camera_; }

    /** @brief カメラの基準位置を設定 */
    void SetCameraBasePosition(const DirectX::XMFLOAT3& pos) { cameraPosition_ = pos; }

    /**
     * @brief 壁衝突時の処理（カメラシェイク＋遅延リスポーン）
     */
    void OnWallHit(Entity player, World &world) {
        if (pendingRespawn_) return;

        if (auto *pv = world.TryGet<PlayerVelocity>(playerEntity_)) {
            if (static_cast<bool>(pv->velocity.x + pv->velocity.y)) {
                float vecX = pv->velocity.x / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                float vecY = pv->velocity.y / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                TriggerCameraImpulse(vecX, 0.0f, vecY, 0.2f, 0.1f);
            }
        }

        if (world_ && world_->IsAlive(player)) {
            if (auto *v = world_->TryGet<PlayerVelocity>(player)) {
                v->velocity = {0.0f, 0.0f};
                v->isBoosting = false;
                v->isDecelerating = false;
                v->boostSpeed = 0.0f;
            }
        }

        pendingRespawn_ = true;
        respawnPlayer_ = player;
        respawnTimer_ = cfg_WallHitRespawnDelay.Get();

        DEBUGLOG("壁に衝突 - カメラシェイク開始、リスポーン待機中");
    }

    /** @brief ワールドへのポインタを設定 */
    void SetWorldRef(World* w) { world_ = w; }

  private:
    // =========================================
    // 初期化ヘルパーメソッド
    // =========================================
    
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

    // =========================================
    // 更新ヘルパーメソッド
    // =========================================

    void HandleStageAdvance(World &world) {
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.requestAdvance) {
                sp.requestAdvance = false;

                std::vector<Entity> stageCreateEntities;
                world.ForEach<StageCreate>([&](Entity e, StageCreate &) {
                    stageCreateEntities.push_back(e);
                });
                for (auto e : stageCreateEntities) {
                    if (world.IsAlive(e)) {
                        world.DestroyEntityWithCause(e, World::Cause::StageReset);
                    }
                }

                sp.currentStage++;

                std::string nextStagePath = "Assets/StageData/StageCollision/DebugStage" + std::to_string(sp.currentStage) + "/room1.csv";
                Entity newStageEntity = world.Create().With<StageCreate>(nextStagePath).Build();
                ownedEntities_.push_back(newStageEntity);

                DEBUGLOG("ステージが進行しました: " + std::to_string(sp.currentStage));
                SetupStage(world, sp.currentStage);
            }
        });
    }

    void SetupInputReferences(World &world, InputSystem &input) {
        world.ForEach<PlayerMovement>([&](Entity, PlayerMovement &pm) {
            if (!pm.input_) pm.input_ = &input;
            if (!pm.gamepad_) pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>();
        });

        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) sys.input_ = &input;
        });
    }

    // =========================================
    // ステージ生成メソッド
    // =========================================

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

            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {
                    int blockType = stagecreate.stageMap[y][x];

                    float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                    float worldY = 0.0f;
                    float worldZ = offsetZ - (static_cast<float>(y) * tileSize);

                    const DirectX::XMFLOAT3 blockposition = {worldX, worldY, worldZ};

                    CreateFloor(world, blockposition);

                    // ステージ境界壁
                    if (y == 0) CreatFloorWall(world, {worldX, worldY, worldZ + tileSize});
                    if (y == max_y_index) CreatFloorWall(world, {worldX, worldY, worldZ - tileSize});
                    if (x == 0) CreatFloorWall(world, {worldX - tileSize, worldY, worldZ});
                    if (x == max_x_index) CreatFloorWall(world, {worldX + tileSize, worldY, worldZ});

                    if (blockType != 0) {
                        CreateBlockByType(world, blockposition, blockType);
                    }
                }
            }
        });
    }

    void CreateBlockByType(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        switch (blockType) {
            case 1: CreateStart(world, position); break;
            case 2: CreateGoal(world, position); break;
            case 3: CreateWall(world, position); break;
            case 5: CreateRightDownCorner(world, position); break;
            case 6: CreateLeftDownCorner(world, position); break;
            case 7: CreateLeftUpCorner(world, position); break;
            case 8: CreateRightUpCorner(world, position); break;
            default:
                if (blockType >= 10 && blockType < 20) {
                    CreateMoveWall(world, position, blockType);
                } else if (blockType >= 30 && blockType < 40) {
                    CreateDashBoard(world, position, blockType);
                }
                break;
        }
    }

    void CreateFloor(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 floorPos = {position.x, position.y - 1, position.z};
        Transform transform{{floorPos}, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_FloorThickness, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity floor = world.Create()
                           .With<Transform>(transform)
                           .With<Model>(cfg_FloorFBXPass)
                           .With<MeshRenderer>(renderer)
                           .Build();

        ownedEntities_.push_back(floor);
    }

    void CreateStart(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - 1.0f, position.z};
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
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - 1.0f, position.z};
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

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_WallFBXPass)
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

    void CreateDashBoard(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
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

    void BakeStageLighting(World & /*world*/) {
        // プレースホルダー
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
        BakeStageLighting(world);

        if (world.IsAlive(playerEntity_)) {
            ResetPlayerToStart(world, playerEntity_);
        }
    }

    // =========================================
    // 遅延リスポーン・カメラリアクション更新
    // =========================================

    void UpdateDelayedRespawn(float dt, World &world) {
        if (!pendingRespawn_) return;
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            ResetPlayerToStart(world, respawnPlayer_, true);
            pendingRespawn_ = false;
            respawnTimer_ = 0.0f;
        }
    }

    void UpdateCameraReaction(float dt, World & /*world*/) {
        switch (reactionType_) {
            case CameraReactionType::Shake: UpdateShake(dt); break;
            case CameraReactionType::Impulse: UpdateImpulse(dt); break;
            case CameraReactionType::Zoom: UpdateZoom(dt); break;
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
        shakeElapsed_ += dt;

        if (shakeElapsed_ >= shakeTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.position = cameraPosition_;
            return;
        }

        float decay = cfg_CameraShakeDecay.Get();
        float decayFactor = std::exp(-decay * shakeElapsed_);
        float currentIntensity = shakeIntensity_ * decayFactor;

        float freqX = cfg_CameraShakeFreqX.Get();
        float freqY = cfg_CameraShakeFreqY.Get();
        float freqZ = cfg_CameraShakeFreqZ.Get();
        float randomness = cfg_CameraShakeRandomness.Get();
        float yScale = cfg_CameraShakeYScale.Get();

        float randX = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;
        float randY = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;
        float randZ = (static_cast<float>(std::rand() % 100) / 100.0f - 0.5f) * 2.0f * randomness;

        float sx = (std::sin(shakeElapsed_ * freqX) * (1.0f - randomness) + randX) * currentIntensity;
        float sy = (std::cos(shakeElapsed_ * freqY) * (1.0f - randomness) + randY) * currentIntensity * yScale;
        float sz = (std::sin(shakeElapsed_ * freqZ) * (1.0f - randomness) + randZ) * currentIntensity;

        camera_.position = {cameraPosition_.x + sx, cameraPosition_.y + sy, cameraPosition_.z + sz};
    }

    void UpdateImpulse(float dt) {
        impulseElapsed_ += dt;

        if (impulseElapsed_ >= impulseTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.position = cameraPosition_;
            camera_.target = baseTarget_;
            return;
        }

        float decay = cfg_CameraImpulseDecay.Get();
        DirectX::XMFLOAT3 dir = impulseDir_;

        if (impulseElapsed_ >= impulseTime_ * (2.0f / 3.0f)) {
            dir.x = -dir.x;
            dir.y = -dir.y;
            dir.z = -dir.z;
        }

        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 1e-6f) {
            dir.x /= len;
            dir.y /= len;
            dir.z /= len;
        }

        float t = impulseElapsed_ / std::max(1e-6f, impulseTime_);
        float rise = std::min(1.0f, t * 2.0f);
        float fall = std::exp(-decay * impulseElapsed_);
        float currentIntensity = impulseIntensity_ * rise * fall;

        if (impulseElapsed_ >= impulseTime_ * (2.0f / 3.0f)) {
            currentIntensity *= 2.0f;
        }

        const float dx = dir.x * currentIntensity;
        const float dy = dir.y * currentIntensity;
        const float dz = dir.z * currentIntensity;

        camera_.position = {cameraPosition_.x + dx, cameraPosition_.y + dy, cameraPosition_.z + dz};
        camera_.target = {baseTarget_.x + dx, baseTarget_.y + dy, baseTarget_.z + dz};
    }

    void UpdateZoom(float dt) {
        zoomElapsed_ += dt;

        if (zoomElapsed_ >= zoomTime_) {
            reactionType_ = CameraReactionType::None;
            camera_.fovY = baseFovY_;
            return;
        }

        float t = zoomElapsed_ / std::max(1e-6f, zoomTime_);
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
    DirectX::XMFLOAT3 cameraPosition_ = {0.0f, 30.0f, -7.0f};
    DirectX::XMFLOAT3 currentTarget_ = {0.0f, 0.0f, 0.0f};
    Camera camera_{};
    float baseFovY_ = DirectX::XM_PIDIV4;

    // カメラリアクション状態
    CameraReactionType reactionType_ = CameraReactionType::None;
    float reactionTime_ = 0.0f;
    float reactionElapsed_ = 0.0f;

    // シェイク用
    float shakeIntensity_ = 0.0f;
    float shakeTime_ = 0.0f;
    float shakeElapsed_ = 0.0f;

    // 衝撃用
    DirectX::XMFLOAT3 impulseDir_ = {0.0f, 0.0f, 0.0f};
    float impulseIntensity_ = 0.0f;
    float impulseTime_ = 0.0f;
    float impulseElapsed_ = 0.0f;

    // ズーム用
    float zoomAmount_ = 0.0f;
    float zoomTime_ = 0.0f;
    float zoomElapsed_ = 0.0f;

    // 遅延リスポーン用
    bool pendingRespawn_ = false;
    Entity respawnPlayer_{};
    float respawnTimer_ = 0.0f;
    World *world_ = nullptr;

    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
};

// =========================================
// 衝突ハンドラーの実装（GameScene定義後）
// =========================================
inline void WallCollisionHandler::OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {
    if (w.Has<PlayerTag>(other)) {
        DEBUGLOG("壁がプレイヤーと衝突 - カメラシェイク＋遅延リスポーン");
        if (g_GameScene) {
            g_GameScene->OnWallHit(other, w);
        } else {
            ResetPlayerToStart(w, other, true);
        }
    }
}

inline void FloorWallCollisionHandler::OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {
    if (w.Has<PlayerTag>(other)) {
        DEBUGLOG("ステージ壁がプレイヤーと衝突 - カメラシェイク＋遅延リスポーン");
        if (g_GameScene) {
            g_GameScene->OnWallHit(other, w);
        } else {
            ResetPlayerToStart(w, other, true);
        }
    }
}
