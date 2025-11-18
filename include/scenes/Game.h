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

// プレイヤーをスタート地点へ戻す (必要に応じてタイマーもリセット)
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

            if (auto *vPlayer = w.TryGet<PlayerVelocity>(player)) {
                vPlayer->velocity = {0.0f, 0.0f};
            }
        }

        // タイマーリセット処理
        if (resetTimer) {
            w.ForEach<GameStats>([](Entity, GameStats &stats) {
                stats.elapsedTime = 0.0f;
            });
        }

        done = true;
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
 * @class GameScene
 * @brief 3DゲームとUIを統合したシーン
 */
class GameScene : public IScene {
  public:
    void OnEnter(World &world) override {
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

        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>().Build();
        ownedEntities_.push_back(collisionSystem);

        Entity stageEntity_ = world.Create().With<StageCreate>().Build();
        ownedEntities_.push_back(stageEntity_);

        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        SetupStage(world, 1);
        CreateUI(world, screenWidth, screenHeight);

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
        Transform transform { {0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f} };
        MeshRenderer renderer;
        renderer.meshType = MeshType::Sphere;
        renderer.color = DirectX::XMFLOAT3 { 0.0f, 0.0f, 1.0f };

        Entity player = world.Create()
            .With<Transform>(transform)
            .With<MeshRenderer>(renderer)
            .With<PlayerTag>()
            .With<PlayerVelocity>()
            .With<PlayerMovement>()
            .With<PlayerGuide>()
            .With<CollisionBox>(DirectX::XMFLOAT3 { 0.8f, 2.0f, 0.8f })
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

            const int max_x_index = stagecreate.stageMap[0].size() - 1;
            const int max_y_index = stagecreate.stageMap.size() - 1;

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

        const float yOffset = -2.0f;
        const float half = (gridSize * tileSize) * 0.5f;

        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                float x = i * tileSize - half + tileSize * 0.5f;
                float z = j * tileSize - half + tileSize * 0.5f;

                Transform transform { { x, yOffset, z }, { 0.0f, 0.0f, 0.0f }, { tileSize, 0.2f, tileSize } };
                MeshRenderer renderer;
                renderer.meshType = MeshType::Cube;
                renderer.color = DirectX::XMFLOAT3 { 0.5f, 0.5f, 0.5f };

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

        Transform t{ diffPosition, {0, 0, 0}, {1, 1, 1}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3 { 0.0f, 0.0f, 1.0f };

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
        r.color = DirectX::XMFLOAT3 { 1.0f, 1.0f, 0.0f };

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
        Transform transform { position, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3 { 1.0f, 1.0f, 1.0f };

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
        Transform transform { position, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3 { 0.5f, 0.5f, 0.5f };

        Entity worldwallEntity = world.Create()
            .With<Transform>(transform)
            .With<MeshRenderer>(renderer)
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

    void ShowStateUI() {}

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
