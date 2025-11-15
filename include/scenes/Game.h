/**
 * @file Game.h
 * @brief UIを統合したゲームシーン
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include "components/GameTags.h"
#include "components/PlayerComponents.h"
#include "components/MeshRenderer.h"
#include "components/Collision.h"
#include "components/UIComponents.h"
#include "input/InputSystem.h"
#include "input/GamepadSystem.h"
#include "components/Rotator.h"
#include "components/Light.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "graphics/GfxDevice.h"
#include "app/ServiceLocator.h"
#include "SenesUIController.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include <sstream>
#include <iomanip>

/**
 * @struct PlayerCollisionHandler
 * @brief プレイヤーの衝突イベントを処理
 */
struct PlayerCollisionHandler : ICollisionHandler {
    void OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) override {
        if (w.Has<EnemyTag>(other)) {
            DEBUGLOG("プレイヤーが敵と衝突 - 侵入深度: " + std::to_string(info.penetrationDepth));
            w.ForEach<GameStats>([](Entity e, GameStats &stats) { stats.score += 10; });
        }
        if (w.Has<GoalTag>(other)) {
            w.ForEach<StageProgress>([](Entity e, StageProgress &sp) { sp.requestAdvance = true; });
            DEBUGLOG("プレイヤーがゴールに到達");
        }
    }
    void OnCollisionStay(World &w, Entity self, Entity other, const CollisionInfo &info) override {}
    void OnCollisionExit(World &w, Entity self, Entity other) override {}
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
        // 壁(self)にプレイヤー(other)が衝突したときにログ出力
        if (w.Has<PlayerTag>(other)) {
            DEBUGLOG("壁がプレイヤーと衝突");
            //プレイヤーが壁に当たったら止まる
            auto &PlayerVelo = w.Get<PlayerVelocity>(other);
            PlayerVelo.velocity = {0.0f, 0.0f};
        }
    }
};
REGISTER_COLLISION_HANDLER_TYPE(WallCollisionHandler)

/**
 * @class GameScene
 * @brief 3DゲームとUIを統合したシーン
 */
class GameScene : public IScene {
  public:
    void OnEnter(World &world) override {
        DEBUGLOG("GameWithUIScene::OnEnter() 開始");
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) { DEBUGLOG_ERROR("GfxDevice が見つかりません"); return; }
        if (!textSystem_.Init(*gfx)) { DEBUGLOG_ERROR("TextSystem の初期化に失敗しました"); return; }
        CreateTextFormats();
        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());
        Entity gameStats = world.Create().With<GameStats>().Build(); ownedEntities_.push_back(gameStats);
        Entity stageProgress = world.Create().With<StageProgress>().Build(); ownedEntities_.push_back(stageProgress);
        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>().Build(); ownedEntities_.push_back(collisionSystem);
        Entity stageEntity_ = world.Create().With<StageCreate>().Build();ownedEntities_.push_back(stageEntity_);

        world.Create().With<DirectionalLight>();
        CreatePlayer(world);
        // ステージ1をセットアップ
        SetupStage(world, 1);
        CreateUI(world, screenWidth, screenHeight);
        DEBUGLOG("GameWithUIScene の初期化が正常に完了しました");
    }
    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        world.ForEach<GameStats>([&](Entity e, GameStats &stats) {
            if (input.GetKeyDown(VK_ESCAPE) || input.GetKeyDown('P')) { stats.isPaused = !stats.isPaused; DEBUGLOG(stats.isPaused ? "ゲームが一時停止されました" : "ゲームが再開されました"); }
            if (stats.isPaused) { deltaTime = 0.0f; }
        });
        world.ForEach<StageProgress>([&](Entity e, StageProgress &sp){
            if (sp.requestAdvance){
                sp.requestAdvance = false;
                sp.currentStage++;
                DEBUGLOG("ステージが進行しました: " + std::to_string(sp.currentStage));
                SetupStage(world, sp.currentStage);
            }
        });
        world.ForEach<PlayerMovement>([&](Entity e, PlayerMovement &pm) { if (!pm.input_) { pm.input_ = &input; } if (!pm.gamepad_) { pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>(); } });
        world.ForEach<UIInteractionSystem>([&](Entity e, UIInteractionSystem &sys) { if (!sys.input_) { sys.input_ = &input; } });
        world.Tick(deltaTime);
    }
    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");
        for (const auto &entity : ownedEntities_) { if (world.IsAlive(entity)) { world.DestroyEntityWithCause(entity, World::Cause::SceneUnload); } }
        ownedEntities_.clear();
        textSystem_.Shutdown();
        DEBUGLOG("GameWithUIScene のクリーンアップが完了しました");
    }
  private:
    void CreateTextFormats();

    void CreateUI(World &world, float screenWidth, float screenHeight);

    void CreatePlayer(World &world) {
        Transform transform{
            {0.0f, 0.0f, 5.0f},
            {0.0f, 0.0f, 0.0f},
            {0.8f, 0.8f, 0.8f},
        };

        MeshRenderer renderer;
        renderer.meshType = MeshType::Sphere;
        renderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f};

        Entity player = world.Create()
                            .With<Transform>(transform)
                            .With<MeshRenderer>(renderer)
                            .With<PlayerTag>()
                            .With<PlayerVelocity>()
                            .With<PlayerMovement>()
                            .With<CollisionBox>(DirectX::XMFLOAT3{0.8f, 2.0f, 0.8f})
                            .With<PlayerCollisionHandler>()
                            .Build();

        playerEntity_ = player;
        ownedEntities_.push_back(player);
    }

    void CreateStageMap(World &world) {
        world.ForEach<StageCreate>([&](Entity stageEntity, StageCreate &stagecreate) {
            float tileSize = 1.0f;
        

            //マップサイズの取得
            if (stagecreate.stageMap.empty() || stagecreate.stageMap[0].empty())
                return;
            float mapWidth = static_cast<float>(stagecreate.stageMap[0].size());
            float mapHeight = static_cast<float>(stagecreate.stageMap.size());

            //マップのインデックスの範囲の最大を取得する
            const int max_x_index = stagecreate.stageMap[0].size() - 1; 
            const int max_y_index = stagecreate.stageMap.size() - 1; 

            //オフセット計算
            const float offsetX = (mapWidth * tileSize) * 0.5f - (tileSize * 0.5f);
            const float offsetZ = (mapHeight * tileSize) * 0.5f - (tileSize * 0.5f);

            //床の生成
            CreateFloor(world, static_cast<int>(mapWidth), tileSize);

            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {
                    //CSVファイル内の値の読み込み
                    int blockType = stagecreate.stageMap[y][x];

                    float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                    float worldY = 0.0f;
                    float worldZ = offsetZ - (static_cast<float>(y) * tileSize);

                    const DirectX::XMFLOAT3 blockposition = {worldX, worldY, worldZ};

                       //ステージの外周に壁を生成する
                    if (y == 0) {               //上端
                        CreatFloorWall(world, {worldX,worldY,worldZ + tileSize});
                    }
                    if (y == max_y_index) {     //下端
                        CreatFloorWall(world, {worldX, worldY, worldZ - tileSize});
                    }      
                    if (x == 0) {               //左端
                        CreatFloorWall(world, {worldX - tileSize, worldY, worldZ});
                    }
                    if (x == max_x_index) {     //右端
                        CreatFloorWall(world, {worldX + tileSize, worldY, worldZ});
                    }

                    if (blockType != 0) {
                        switch (blockType) {
                            case 1:
                                CreateStart(world, blockposition);
                                break;
                            case 2:
                                CreateGoal(world, blockposition);
                                break;
                            case 3:
                                CreateWall(world, blockposition);
                                break;
                        }
                    }
                }
            }
        });
    }

 
    void CreateFloor(World &world, int gridSize, float tileSize) {

        if (gridSize <= 0.0f || tileSize <= 0.0f)
            return;

        const float yOffset = -2.0f; //高さのオフセット
        const float half = (gridSize * tileSize) * 0.5f;

        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                float x = i * tileSize - half + tileSize * 0.5f;
                float z = j * tileSize - half + tileSize * 0.5f;

                Transform transform{
                    {x, yOffset, z},
                    {0.0f, 0.0f, 0.0f}, //回転
                    {tileSize, 0.2f, tileSize},
                };

                MeshRenderer renderer;
                renderer.meshType = MeshType::Cube;
                renderer.color = DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}; //床の色

                Entity floor = world.Create()
                                   .With<Transform>(transform)
                                   .With<MeshRenderer>(renderer)
                                   //.With<CollisionBox>(DirectX::XMFLOAT3{20.0f, 0.2f, 20.0f})
                                   .Build();

                ownedEntities_.push_back(floor);
            }
        }
    }

    void CreateStart(World &world, const DirectX::XMFLOAT3 &position) {
        Transform t{position,{0,0,0},{1,1,1}};

        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{0.0f,0.0f,1.0f};

        Entity e = world.Create()
                      .With<Transform>(t)
                      .With<MeshRenderer>(r)
                      .With<StartTag>()
                      .With<CollisionBox>(DirectX::XMFLOAT3{1.0f,2.0f,1.0f})
                      .Build();

        startEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateGoal(World &world, const DirectX::XMFLOAT3 &position) {
        Transform t{position,{0,0,0},{1,1,1}};

        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{1.0f,1.0f,0.0f};

        Entity e = world.Create()
                      .With<Transform>(t)
                      .With<MeshRenderer>(r)
                      .With<GoalTag>()
                      .With<CollisionBox>(DirectX::XMFLOAT3{1.0f,2.0f,1.0f})
                      .Build();

        goalEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};
        Entity wall = world.Create().With<Transform>(transform).With<MeshRenderer>(renderer).With<WallTag>().With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f}).With<WallCollisionHandler>().Build();
        stageOwnedEntities_.push_back(wall);
    }

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f};
        Entity worldwall = world.Create().With<Transform>(transform).With<MeshRenderer>(renderer); 
        stageOwnedEntities_.push_back(wall);
    }

    void SetupStage(World &world, int stage) {
        //既存のステージ要素をリセット
        for (const auto &entity : stageOwnedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);            
            }
        }
        stageOwnedEntities_.clear();
        startEntity_ = {};
        goalEntity_ = {};

        CreateStageMap(world);

        //プレイヤーの位置をリセット
        if (world.IsAlive(playerEntity_) && world.IsAlive(startEntity_)) {
            auto *tPlayer = world.TryGet<Transform>(playerEntity_);
            auto *tStart = world.TryGet<Transform>(startEntity_);

            if (tPlayer && tStart) {
                tPlayer->position = {tStart->position.x,0.0f,tStart->position.z};

                //プレイヤーの速度リセット
                if (auto *vPlayer = world.TryGet<PlayerVelocity>(playerEntity_)) {
                    vPlayer->velocity = {0.0f,0.0f};
                }


            }
        }
    
    }

    //プレイヤーのスタート合図
    void ShowStateUI()
    {
        //優先事項：UIの表示
        //プレイヤーの停止（後でいい）
        //カウントスタート321
        //プレイヤーの解放
    }

    TextSystem textSystem_;
    std::vector<Entity> ownedEntities_;
    std::vector<Entity> stageOwnedEntities_;
    Entity playerEntity_{};
    Entity stageEntity{};
    Entity startEntity_{};
    Entity wall{};
    Entity worldwall{};
    Entity goalEntity_{};
};
