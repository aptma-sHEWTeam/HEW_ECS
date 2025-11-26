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

#include "config/ConfigVar.h"
#include "components/GameTags.h"
#include "components/PlayerComponents.h"
#include "components/UIComponents.h"
#include "components/CountUIComponent.h"
#include "components/Rotator.h"
#include "components/Light.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "app/ServiceLocator.h"
#include "SenesUIController.h"
#include "systems/ModelLoadingSystem.h"

inline static ConfigVar<float> cfg_LimitTime{"Game", "LimitTime", 10.0f};

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
            tPlayer->position = {tStart.position.x, 0.0f, tStart.position.z-1.0f};//プレイヤーの生成場所

            if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
                vPlayer->velocity = {0.0f, 0.0f};
            }
        }

        if (resetTimer) {
            w.ForEach<GameStats>([](Entity, GameStats &stats) {
                stats.elapsedTime = 0.0f;
            });
        }

        done = true;
    });
}

inline void CheckTimeLimit(World &w,Entity player, float timeLimitSeconds) {
    w.ForEach<GameStats>([&](Entity e, GameStats &stats) {
        if (stats.elapsedTime >= timeLimitSeconds) {
            DEBUGLOG("時間切れ");
            ResetPlayerToStart(w,player,true);
        }
    });
}

/**
 * @struct PlayerCollisionHandler
 * @brief プレイヤーの衝突イベントを処理
 */
struct PlayerCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        if (w.Has<EnemyTag>(other)) {
            DEBUGLOG("プレイヤーが敵と衝突 - 侵入深度: " + std::to_string(info.penetrationDepth));
            w.ForEach<GameStats>([](Entity, GameStats &stats) { stats.score += 10; });
        }
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
 * @brief 壁の衝突イベントを処理
 */
struct WallCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World& w, Entity self, Entity other, const CollisionInfo& info) override {
        if (w.Has<PlayerTag>(other)) {
            DEBUGLOG("壁がプレイヤーと衝突 - スタート地点へ戻しタイマーをリセット");
            ResetPlayerToStart(w, other, true);
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(WallCollisionHandler)

/**
 * @struct FloorWallColisionHandler
 * @brief ステージの壁の衝突イベントを処理
 */
struct FloorWallCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World& w, Entity self, Entity other, const CollisionInfo& info) override {
         if (w.Has<PlayerTag>(other)) {
            DEBUGLOG("壁がプレイヤーと衝突 - スタート地点へ戻しタイマーをリセット");
             ResetPlayerToStart(w,other,true);
          }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(FloorWallCollisionHandler)

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

    inline static ConfigVar<std::string> cfg_PlayerFBXPass{"Player", "PlayerFBXPass", "Assets/Models/aaa.fbx"};

    inline static ConfigVar<std::string> cfg_StagePath{"Stage", "CSVPath", "Assets/StageData/aaa.csv"};

    inline static ConfigVar<float> cfg_CollisionCellSize{"Game", "CollisionCellSize", 20.0f};


    void OnEnter(World &world) override {
        DEBUGLOG("<<<<< GameScene::OnEnter CALLED! >>>>>");
        DEBUGLOG("GameWithUIScene::OnEnter() 開始");

        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("GfxDevice が見つかりません");
            return;
        }

        if (!textSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("TextSystem の初期化に失敗しました");
            return;
        }

        CreateTextFormats();

        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        Entity gameStats = world.Create().With<GameStats>().Build();
        ownedEntities_.push_back(gameStats);

        Entity stageProgress = world.Create().With<StageProgress>().Build();
        ownedEntities_.push_back(stageProgress);

        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>(cfg_CollisionCellSize.Get()).Build();
        ownedEntities_.push_back(collisionSystem);

        // ModelLoadingSystem を追加して Model -> ModelComponent 変換を有効化
        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        Entity stageEntity_ = world.Create().With<StageCreate>(cfg_StagePath.Get()).Build();
        ownedEntities_.push_back(stageEntity_);

        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        CreateUI(world, screenWidth, screenHeight);
        ShowStateUI(world);
        SetupStage(world, 1);

        DEBUGLOG("GameWithUIScene の初期化が正常に完了しました");
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {

        // ゲームの一時停止と再開
        world.ForEach<GameStats>([&](Entity, GameStats &stats) {
            if (input.GetKeyDown(VK_ESCAPE) || input.GetKeyDown('P')) {
                stats.isPaused = !stats.isPaused;
                DEBUGLOG(stats.isPaused ? "ゲームが一時停止されました" : "ゲームが再開されました");
            }

            if (stats.isPaused) {
                deltaTime = 0.0f;
            }
        });

        // ステージの進行
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.requestAdvance) {
                sp.requestAdvance = false;
                sp.currentStage++;
                DEBUGLOG("ステージが進行しました: " + std::to_string(sp.currentStage));
                SetupStage(world, sp.currentStage);
            }
        });

        // プレイヤーの移動
        world.ForEach<PlayerMovement>([&](Entity, PlayerMovement &pm) {
            if (!pm.input_) {
                pm.input_ = &input;
            }
            if (!pm.gamepad_) {
                pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>();
            }
        });

        // UI インタラクションの設定
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        world.Tick(deltaTime);

        //制限時間が過ぎていたらリセット
        if (world.IsAlive(playerEntity_)) {
            CheckTimeLimit(world, playerEntity_, cfg_LimitTime);
        }
    }

    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");

        for (const auto &entity : ownedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        textSystem_.Shutdown();
        DEBUGLOG("GameWithUIScene のクリーンアップが完了しました");
    }

  private:
    void CreateTextFormats();
    void CreateUI(World &world, float screenWidth, float screenHeight);

    void CreatePlayer(World &world) {
        float s = cfg_PlayerScale;
        Transform transform { {0.0f, 0.0f, cfg_PlayerStartY }, {0.0f, 0.0f, 0.0f}, {s, s, s} };

        Entity player = world.Create()
            .With<Transform>(transform)
            .With<Model>(cfg_PlayerFBXPass)
            .With<PlayerTag>()
            .With<PlayerVelocity>()
            .With<PlayerMovement>()
            .With<PlayerGuide>()
            .With<CollisionBox>(DirectX::XMFLOAT3 { s, cfg_PlayerHeight, s })
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

            // ステージの床を生成
            CreateFloor(world, static_cast<int>(mapWidth), tileSize);

            // ステージマップに基づいてオブジェクトを生成
            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {
                    int blockType = stagecreate.stageMap[y][x];

                    // タイルのワールド座標を計算
                    float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                    float worldY = 0.0f;
                    float worldZ = offsetZ - (static_cast<float>(y) * tileSize);

                    const DirectX::XMFLOAT3 blockposition = { worldX, worldY, worldZ };

                    // ステージの境界には常に壁を生成
                    if (y == 0) { CreatFloorWall(world, { worldX, worldY, worldZ + tileSize }); } // 下
                    if (y == max_y_index) { CreatFloorWall(world, { worldX, worldY, worldZ - tileSize }); } // 上
                    if (x == 0) { CreatFloorWall(world, { worldX - tileSize, worldY, worldZ }); } // 左
                    if (x == max_x_index) { CreatFloorWall(world, { worldX + tileSize, worldY, worldZ }); } // 右

                    // ステージマップに応じたオブジェクトの生成
                    if (blockType != 0) {
                        switch (blockType) {
                            case 1: CreateStart(world, blockposition); break; // スタート地点
                            case 2: CreateGoal(world, blockposition); break; // ゴール地点
                            case 3: CreateWall(world, blockposition); break; // 通常の壁
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

                Transform transform { { x, yOffset, z }, { 0.0f, 0.0f, 0.0f }, { tileSize, cfg_FloorThickness, tileSize } };
                MeshRenderer renderer;
                renderer.meshType = MeshType::Cube;
                renderer.color = DirectX::XMFLOAT3 { cfg_FloorR, cfg_FloorG, cfg_FloorB };

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
        diffPosition.x = position.x ;
        diffPosition.y = position.y - 1.0f;
        diffPosition.z = position.z ;

        Transform t{ diffPosition , {0, 0, 0}, {1, 1, 1}};//スタート地点のBox
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3 { cfg_StartR, cfg_StartG, cfg_StartB };

        Entity e = world.Create()
            .With<Transform>(t)
            .With<MeshRenderer>(r)
            .With<StartTag>()
            .With<CollisionBox>(DirectX::XMFLOAT3 { 1.0f, 2.0f, 1.0f })
            .Build();

        startEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateGoal(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition;
        diffPosition.x = position.x;
        diffPosition.y = position.y - 1.0f;
        diffPosition.z = position.z;
        Transform t{ diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3 { cfg_GoalR, cfg_GoalG, cfg_GoalB };

        Entity e = world.Create()
            .With<Transform>(t)
            .With<MeshRenderer>(r)
            .With<GoalTag>()
            .With<CollisionBox>(DirectX::XMFLOAT3 { 1.0f, 2.0f, 1.0f })
            .Build();

        goalEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3 { cfg_WallR, cfg_WallG, cfg_WallB };

        Entity wallEntity = world.Create()
            .With<Transform>(transform)
            .With<MeshRenderer>(renderer)
            .With<WallTag>()
            .With<CollisionBox>(DirectX::XMFLOAT3 { 1.0f, 2.0f, 1.0f })
            .With<WallCollisionHandler>()
            .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3 { cfg_FloorWallR, cfg_FloorWallG, cfg_FloorWallB };

        Entity worldwallEntity = world.Create()
            .With<Transform>(transform)
            .With<MeshRenderer>(renderer)
            .With<WallTag>()
            .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
            .With<FloorWallCollisionHandler>()
            .Build();

        stageOwnedEntities_.push_back(worldwallEntity);
    }

    void SetupStage(World &world, int stage) {
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

        // プレイヤーをスタート地点にリセット
        if (world.IsAlive(playerEntity_)) {
            ResetPlayerToStart(world, playerEntity_);
        }
    }

    void ShowStateUI(World &world) {
        UITransform CountTransform;
        CountTransform.position = {cfg_UICountPosX, cfg_UICountPosY};
        CountTransform.size = {cfg_UICountW, cfg_UICountH};
        CountTransform.anchor = {0.0f, 0.0f};
        CountTransform.pivot = {0.0f, 0.0f};

        UIText CountText{L"Count:Go"};
        CountText.color = {cfg_UICountR, cfg_UICountG, cfg_UICountB, 1.0f};
        CountText.formatId = "hud";

        Entity CountEntity = world.Create()
                               .With<UITransform>(CountTransform)
                               .With<UIText>(CountText)
                               .Build();
        ownedEntities_.push_back(CountEntity);
       /* stateCountDowndoActive_ = false;
        stateFrameCounter_ = 0;
        stateCountdownJustFinished_ = false;

        if (world.IsAlive(startTextEntity_))
        {

        }*/

    }

    TextSystem textSystem_;
    std::vector<Entity> ownedEntities_;
    std::vector<Entity> stageOwnedEntities_;
    Entity playerEntity_{};
    Entity stageEntity_{};
    Entity startEntity_{};
    Entity wall_{};
    Entity worldwall_{};
    Entity goalEntity_{};
};
