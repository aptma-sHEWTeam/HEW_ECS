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
            w.ForEach<StageProgress>([](Entity e, StageProgress &sp) { sp.requestAdvance = true, sp.currentStage += 1; });
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
#ifdef _DEBUG
        Entity debugRenderer = world.Create().With<CollisionDebugRenderer>().Build(); ownedEntities_.push_back(debugRenderer);
#endif
        world.Create().With<DirectionalLight>();
        int gridSize = 10;//床のサイズ指定
        float tileSize = 1.0f;
        CreateFloor(world, gridSize,tileSize);
        CreatePlayer(world);
        CreateStart(world);
        CreateGoal(world);
        CreateTestEnemy(world);
        CreateWall(world,{3.0f,0.0f,3.0f});
        CreateWall(world, {-1.0f, 0.0f, 3.0f});
        CreateUI(world, screenWidth, screenHeight);
        // ステージ1をセットアップ
        SetupStage(world, 1);
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
    void CreateTextFormats() {
        TextSystem::TextFormat hudFormat;
        hudFormat.fontSize = 24.0f;
        hudFormat.fontFamily = L"メイリオ";
        hudFormat.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        textSystem_.CreateTextFormat("hud", hudFormat);

        TextSystem::TextFormat pauseFormat;
        pauseFormat.fontSize = 72.0f;
        pauseFormat.fontFamily = L"メイリオ";
        pauseFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        pauseFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        textSystem_.CreateTextFormat("pause", pauseFormat);

        TextSystem::TextFormat buttonFormat;
        buttonFormat.fontSize = 20.0f;
        buttonFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        buttonFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        textSystem_.CreateTextFormat("button", buttonFormat);

        TextSystem::TextFormat panelFormat;
        panelFormat.fontSize = 200.0f;
        textSystem_.CreateTextFormat("panel", panelFormat);

        TextSystem::TextFormat titleFormat;
        titleFormat.fontSize = 20.0f;
        titleFormat.fontFamily = L"メイリオ";
        titleFormat.style = DWRITE_FONT_STYLE_ITALIC;
        titleFormat.alignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
        titleFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
        textSystem_.CreateTextFormat("title", titleFormat);

    }

    void CreateUI(World &world, float screenWidth, float screenHeight) {
        Entity canvas = world.Create()
                            .With<UICanvas>()
                            .Build();
        ownedEntities_.push_back(canvas);

        Entity uiRenderSystem = world.Create()
                                    .With<UIRenderSystem>()
                                    .Build();
        auto *renderSys = world.TryGet<UIRenderSystem>(uiRenderSystem);
        if (renderSys) {
            renderSys->SetTextSystem(&textSystem_);
            renderSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiRenderSystem);

        Entity uiInteractionSystem = world.Create()
                                         .With<UIInteractionSystem>()
                                         .Build();
        auto *interactionSys = world.TryGet<UIInteractionSystem>(uiInteractionSystem);
        if (interactionSys) {
            interactionSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiInteractionSystem);

        UITransform scoreTransform;
        scoreTransform.position = {20.0f, 20.0f};
        scoreTransform.size = {300.0f, 40.0f};
        scoreTransform.anchor = {0.0f, 0.0f};
        scoreTransform.pivot = {0.0f, 0.0f};

        UIText scoreText{L"スコア: 0"};
        scoreText.color = {1.0f, 1.0f, 0.0f, 1.0f};
        scoreText.formatId = "hud";

        Entity scoreEntity = world.Create()
                                 .With<UITransform>(scoreTransform)
                                 .With<UIText>(scoreText)
                                 .Build();
        ownedEntities_.push_back(scoreEntity);

        UITransform timeTransform;
        timeTransform.position = {20.0f, 70.0f};
        timeTransform.size = {300.0f, 40.0f};
        timeTransform.anchor = {0.0f, 0.0f};
        timeTransform.pivot = {0.0f, 0.0f};

        UIText timeText{L"時間: 00:00"};
        timeText.color = {1.0f, 1.0f, 1.0f, 1.0f};
        timeText.formatId = "hud";

        Entity timeEntity = world.Create()
                                .With<UITransform>(timeTransform)
                                .With<UIText>(timeText)
                                .Build();
        ownedEntities_.push_back(timeEntity);

        UITransform fpsTransform;
        fpsTransform.position = {-20.0f, 20.0f};
        fpsTransform.size = {200.0f, 40.0f};
        fpsTransform.anchor = {1.0f, 0.0f};
        fpsTransform.pivot = {1.0f, 0.0f};

        UIText fpsText{L"FPS: 0.0"};
        fpsText.color = {0.0f, 1.0f, 0.0f, 1.0f};
        fpsText.formatId = "hud";

        Entity fpsEntity = world.Create()
                               .With<UITransform>(fpsTransform)
                               .With<UIText>(fpsText)
                               .Build();
        ownedEntities_.push_back(fpsEntity);

        UITransform stageTransform;
        stageTransform.position = {150.0f, 120.0f};
        stageTransform.size = {130.0f, 40.0f};
        stageTransform.anchor = {0.0f, 0.0f};
        stageTransform.pivot = {1.0f, 0.0f};

        UIText stageText{L"FLOOR: 1"};
        stageText.color = {1.0f, 0.5f, 0.0f, 1.0f};
        stageText.formatId = "hud";

        Entity stageEntity = world.Create()
                                 .With<UITransform>(stageTransform)
                                 .With<UIText>(stageText)
                                 .Build();
        ownedEntities_.push_back(stageEntity);
        
        UIText titleText[2];

        titleText[0].text     = {L"Fricker Game:"};
        titleText[0].color    = {1.0f, 0.0f, 1.0f, 1.0f};
        titleText[0].formatId = "title";
        float titletextSize0  = 3.9f * sizeof(titleText[0].text);

        titleText[1].text     = {L"Proto Type"};
        titleText[1].color    = {0.7f, 0.0f, 0.7f, 1.0f};
        titleText[1].formatId = "title";

        UITransform titleTransform[2];

        titleTransform[0].position = {800.0f, 60.0f};
        titleTransform[0].size     = {300.0f, 30.0f};
        titleTransform[0].anchor   = {0.0f,    0.0f};
        titleTransform[0].pivot    = {0.0f,    0.0f};

        titleTransform[1].position = {titleTransform[0].position.x + titletextSize0 , titleTransform[0].position.y};
        titleTransform[1].size     = titleTransform[0].size;
        titleTransform[1].anchor   = titleTransform[0].anchor;
        titleTransform[1].pivot    = titleTransform[0].pivot;

        for (int i = 0 ; i < 2; i++)
        {
            Entity titleEntity[2];

                   titleEntity[i] = world.Create()
                                     .With<UITransform>(titleTransform[i])
                                     .With<UIText>(titleText[i])
                                     .Build();
            ownedEntities_.push_back(titleEntity[i]);
        }

        UITransform pauseTransform;
        pauseTransform.position = {0.0f, 0.0f};
        pauseTransform.size = {0.0f, 0.0f};
        pauseTransform.anchor = {0.5f, 0.5f};
        pauseTransform.pivot = {0.5f, 0.5f};

        UIText pauseText{L""};
        pauseText.color = {1.0f, 0.0f, 0.0f, 1.0f};
        pauseText.formatId = "pause";

        Entity pauseEntity = world.Create()
                                 .With<UITransform>(pauseTransform)
                                 .With<UIText>(pauseText)
                                 .Build();
        ownedEntities_.push_back(pauseEntity);

        Entity uiUpdater = world.Create()
                               .With<GameUIUpdater>()
                               .Build();
        auto *updater = world.TryGet<GameUIUpdater>(uiUpdater);
        if (updater) {
            updater->scoreTextEntity_ = scoreEntity;
            updater->timeTextEntity_ = timeEntity;
            updater->fpsTextEntity_ = fpsEntity;
            updater->pauseTextEntity_ = pauseEntity;
            updater->stageTextEntity_ = stageEntity;
        }
        ownedEntities_.push_back(uiUpdater);
    }

    //床の生成　gridSize ×　gridSizeのタイル生成
    void CreateFloor(World &world ,int gridSize, float tileSize ) {

        if (gridSize <= 0.0f || tileSize <= 0.0f)
            return;

        const float yOffset = -2.0f;//高さのオフセット
        const float half = (gridSize * tileSize) * 0.5f;
        
        

        for (int i = 0; i < gridSize; ++i)
        {
            for (int j = 0; j < gridSize; ++j)
            {
                float x = i * tileSize - half + tileSize * 0.5f;
                float z = j * tileSize - half + tileSize * 0.5f;

                Transform transform{
                    {x, yOffset, z},
                    {0.0f, 0.0f, 0.0f}, //回転
                    {tileSize, 0.2f, tileSize},
                };

                MeshRenderer renderer;
                renderer.meshType = MeshType::Cube;
                renderer.color = DirectX::XMFLOAT3{1.0f, 0.0f, 1.0f};

                Entity floor = world.Create()
                                   .With<Transform>(transform)
                                   .With<MeshRenderer>(renderer)
                                   //.With<CollisionBox>(DirectX::XMFLOAT3{20.0f, 0.2f, 20.0f})
                                   .Build();

                ownedEntities_.push_back(floor);

            }

        }
       

    }

    void CreatePlayer(World &world) {
        Transform transform{
            {0.0f, 0.0f, 5.0f},
            {0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f},
        };

        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f};

        Entity player = world.Create()
                            .With<Transform>(transform)
                            .With<MeshRenderer>(renderer)
                            .With<PlayerTag>()
                            .With<PlayerVelocity>()
                            .With<PlayerMovement>()
                            .With<Rotator>(45.0f)
                            .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                            .With<PlayerCollisionHandler>()
                            .Build();

        playerEntity_ = player;
        ownedEntities_.push_back(player);
    }

    void CreateStart(World &world) {
        Transform t{{-3.0f,0.0f,5.0f},{0,0,0},{1,1,1}};

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
        ownedEntities_.push_back(e);
    }

    void CreateGoal(World &world) {
        Transform t{{5.0f,0.0f,5.0f},{0,0,0},{1,1,1}};

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
        ownedEntities_.push_back(e);
    }

    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};
        Entity wall = world.Create().With<Transform>(transform).With<MeshRenderer>(renderer).With<WallTag>().With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f}).With<WallCollisionHandler>().Build();
        ownedEntities_.push_back(wall);
    }

    void CreateTestEnemy(World &world) {
        Transform transform{{1.5f, 0.0f, 5.0f},{0.0f, 0.0f, 0.0f},{1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer; renderer.meshType = MeshType::Sphere; renderer.color = DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f};
        Entity enemy = world.Create().With<Transform>(transform).With<MeshRenderer>(renderer).With<EnemyTag>().With<CollisionSphere>(0.5f).With<EnemyCollisionHandler>().Build();
        ownedEntities_.push_back(enemy);
    }

    void SetupStage(World &world, int stage) {
        float goalX = 5.0f + static_cast<float>(stage - 1) * 2.0f;
        if (world.IsAlive(goalEntity_)) { if (auto* tGoal = world.TryGet<Transform>(goalEntity_)) { tGoal->position = { goalX, 0.0f, 5.0f }; } }
        if (world.IsAlive(startEntity_)) { if (auto* tStart = world.TryGet<Transform>(startEntity_)) { tStart->position = { -3.0f, 0.0f, 5.0f }; } }
        if (world.IsAlive(playerEntity_)) { if (auto* tPlayer = world.TryGet<Transform>(playerEntity_)) { tPlayer->position = { -3.0f, 0.0f, 5.0f }; } }
    }

    TextSystem textSystem_;
    std::vector<Entity> ownedEntities_;
    Entity playerEntity_{};
    Entity startEntity_{};
    Entity goalEntity_{};
};


