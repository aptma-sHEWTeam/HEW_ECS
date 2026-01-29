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
#include <fstream>
#include <optional>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cassert>

// 追加: IScene 定義を利用するためにシーンマネージャのヘッダーをインクルード
#include "scenes/SceneManager.h"

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
#include "components/MeshRenderer.h"
#include "components/ModelComponent.h"
#include "components/Collision.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include "components/TimeScaleComponents.h"
#include "components/EmissiveMaterial.h"
#include "components/EmissivePulse.h"
#include "components/TransformHierarchy.h"
#include "components/PointLight.h"
#include "systems/RenderingSystem.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "graphics/ImageSystem.h"
#include "graphics/Effect.h"
#include "app/ServiceLocator.h"
#include "SenesUIController.h"
#include "systems/ModelLoadingSystem.h"
#include "graphics/TextureManager.h"
#include "graphics/Camera.h"
#include <comdef.h>
#include "animation/Animation.h"
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"
#include "config/ConfigVar.h"
#include "graphics/ModelLoader.h"
#include "components/Animator.h"
#include "graphics/StageSave.h"

#include "graphics/SkyboxSystem.h"
#include "graphics/OmniShadowMap.h"
#include "systems/ShadowRenderSystem.h"

//Config Var
// チャージ中オーバーレイのフェード量（0～1）は Animation.h の cfg_ChargingFade を参照
inline static ConfigVar<float> cfg_GoalDistance{"Distance.Goal", "GoalDistance", 2.0f, "ゴール接近スロー演出を開始する距離"};
inline static ConfigVar<float> cfg_SlowDirection{"Direction.Slow", "SlowDistance", 0.2f, "ゴール接近時に適用するタイムスケール"};
inline static ConfigVar<float> cfg_StickZoomAmount{"Camera.Stick", "StickZoomAmount", 0.07f, "チャージ中のカメラズーム量"};
inline static ConfigVar<float> cfg_StickZoomResponse{"Camera.Stick", "StickZoomResponse", 3.0f, "カメラズーム追従速度"};
inline static ConfigVar<float> cfg_StickZoomTargetRatio{"Camera.Stick", "StickZoomTargetRatio", 0.3f, "プレイヤーへのカメラ寄り具合(0=寄らない,1=完全に寄る)"};
inline static ConfigVar<float> cfg_StickZoomTargetSpeed{"Camera.Stick", "StickZoomTargetSpeed", 2.0f, "カメラターゲット補間速度"};
// 追加: ステージクリア待機時間
inline static ConfigVar<float> cfg_StageClearWait{"UI.StageClear", "WaitSeconds", 2.0f, "ステージクリア表示後にシーン遷移するまでの待機時間"};
inline static ConfigVar<float> cfg_SkyboxSpeed{"Skybox", "Speed", 0.05f, "スカイボックスの回転速度(rad/sec)"};
inline static ConfigVar<std::string> cfg_GameSkyboxModelPath{"Game.Skybox", "ModelPath", "Assets/Textures/Skybox/skybox.fbx", "ゲーム: Skybox モデルパス"};
inline static ConfigVar<std::string> cfg_GameSkyboxTexturePath{"Game.Skybox", "TexturePath", "Assets/Textures/Skybox/Sky_Box.png", "ゲーム: Skybox テクスチャパス"};
inline static ConfigVar<float> cfg_GameSkyboxScale{"Game.Skybox", "Scale", 200.0f, "ゲーム: Skybox スケール"};
//プレイヤーが壁に衝突/タイムアップした時のフェード表示の遅延時間
inline static ConfigVar<float> cfg_DeathAnimationFadeTime{"DeathFadeAnimation", "Time", 1.0f, "壁に衝突/タイムアップ時のフェード表示遅延時間"};

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

        pauseInputLock_ = 0.35f;

        StageSave::Load();
        auto *mgr = ServiceLocator::TryGet<SceneManager>();
        if (!mgr)
            return;

        world.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
            stats.isPaused = false;

            if (stats.isDead &&
                !mgr->IsTransitioning() &&
                !stats.resetDone) {
                stats.elapsedTime = cfg_LimitTime;
                stats.resetDone = true;
            }
            // Ensure entry grace: wait for player to make first move before accepting goal collisions
            stats.waitingForPlayerMove = true;
        });

        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            sp.clearedThisStage = false;
            sp.requestAdvance = false;
            sp.goalTransitioning = false;
            sp.pressedSwitch = false;
            sp.goalUnlocked = false;
            sp.currentRoom = 1;
            sp.hasSwitch = false;
            sp.IsWorldBack = false;
            sp.IsWorldNext = false;
            sp.IsClearBack = false;
        });

        world.ForEach<GoalTag>([](Entity, GoalTag &goal) {
            goal.consumed = false;
        });


        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            DEBUGLOG("[Game::OnEnter] StageProgress enter world=" + std::to_string(sp.worldCount) +
                     " select=" + std::to_string(sp.selectStage) +
                     " current=" + std::to_string(sp.currentStage) +
                     " clearedThisStage(before)=" + std::to_string(sp.clearedThisStage));
            sp.clearedThisStage = false;
            sp.goalTransitioning = false;
            sp.requestAdvance = false;
            sp.pressedSwitch = false;
            sp.goalUnlocked = false;
            g_LastStageProgress = sp;
            DEBUGLOG("[Game::OnEnter] clearedThisStage reset to false, g_Last=" + std::to_string(g_LastStageProgress.clearedThisStage));
        });

        // このシーンインスタンスへのグローバルポインタを設定
        g_GameScene = this;

        // ステージクリア状態の初期化
        stageClearActive_ = false;
        stageClearTimer_ = 0.0f;
        stageClearTextEntity_ = {};
        stageOwnedEntities_.clear();
        goalEffectHandle_ = -1;
        pendingStageAdvance_ = {};
        stageAdvanceTimer_ = 0.0f;
        StopGoalEffect();

        // 以前のゴールのアトラクターが残っていれば削除（再入場時の即クリア防止）
        if (world.IsAlive(playerEntity_)) {
            if (world.Has<GoalAttractor>(playerEntity_)) {
                world.Remove<GoalAttractor>(playerEntity_);
            }
        }

        // 入り直し時に残存ステージエンティティがあれば除去（原点残留や重複生成を防ぐ）
        {
            std::vector<Entity> toDestroy;
            world.ForEach<StageCreate>([&](Entity e, StageCreate &) { toDestroy.push_back(e); });
            world.ForEach<WallTag>([&](Entity e, WallTag &) { toDestroy.push_back(e); });
            world.ForEach<GoalTag>([&](Entity e, GoalTag &) { toDestroy.push_back(e); });
            world.ForEach<StartTag>([&](Entity e, StartTag &) { toDestroy.push_back(e); });
            world.ForEach<GimmickTag>([&](Entity e, GimmickTag &) { toDestroy.push_back(e); });
            if (!toDestroy.empty()) {
                world.DestroyAllEntitiesImmediate(World::Cause::SceneInit);
            }
        }

        // ステージ進行情報が無ければ生成
        bool hasStageProgress = false;
        world.ForEach<StageProgress>([&](Entity, StageProgress &) { hasStageProgress = true; });
        if (!hasStageProgress) {
            world.Create().With<StageProgress>().Build();
        }

        // サービスロケーターからグラフィックスデバイスを取得
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("GfxDevice が見つかりません");
            return;
        }

        // レンダリングシステムの初期化と環境光の設定
        RenderingSystem::GetInstance().Initialize(gfx->Dev());
        DirectX::XMFLOAT3 ambientColor{
            cfg_AmbientR.Get(),
            cfg_AmbientG.Get(),
            cfg_AmbientB.Get()};
        RenderingSystem::GetInstance().SetAmbientLight(ambientColor, cfg_AmbientIntensity.Get());

        // 平行光の生成（有効時のみ）
        if (cfg_DirLightEnabled.Get()) {
            Entity dirLightEntity = world.Create().With<DirectionalLight>().Build();
            ownedEntities_.push_back(dirLightEntity);

            if (auto *dl = world.TryGet<DirectionalLight>(dirLightEntity)) {
                DirectX::XMFLOAT3 dir{cfg_DirLightX.Get(), cfg_DirLightY.Get(), cfg_DirLightZ.Get()};
                DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&dir);
                float lenSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(v));
                if (lenSq > 1e-6f) {
                    v = DirectX::XMVector3Normalize(v);
                    DirectX::XMStoreFloat3(&dir, v);
                } else {
                    dir = {0.0f, -1.0f, 0.0f};
                }
                dl->direction = dir;
                dl->color = DirectX::XMFLOAT4{
                    cfg_DirLightR.Get(),
                    cfg_DirLightG.Get(),
                    cfg_DirLightB.Get(),
                    std::max(0.0f, cfg_DirLightIntensity.Get())};
            }
        }
        // テキスト描画システムの初期化
        try {
            if (!textSystem_.Init(*gfx)) {
                DEBUGLOG_ERROR("TextSystem の初期化に失敗しました");
                return;
            }
        } catch (const _com_error &ex) {
            std::wostringstream woss;
            const wchar_t *wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
            woss << L"TextSystem init _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
            std::wstring w = woss.str();
            std::string n(w.begin(), w.end());
            DEBUGLOG_ERROR(n);
            return;
        }

        // 画像描画システムの初期化
        try {
            if (!imageSystem_.Init(*gfx)) {
                DEBUGLOG_ERROR("ImageSystem の初期化に失敗しました");
                return;
            }
        } catch (const _com_error &ex) {
            std::wostringstream woss;
            const wchar_t *wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
            woss << L"ImageSystem init _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
            std::wstring w = woss.str();
            std::string n(w.begin(), w.end());
            DEBUGLOG_ERROR(n);
            return;
        }

        // 画像描画システムの初期化
        try {
            if (!imageSystem_.Init(*gfx)) {
                DEBUGLOG_ERROR("ImageSystem の初期化に失敗しました");
                return;
            }
        } catch (const _com_error &ex) {
            std::wostringstream woss;
            const wchar_t *wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
            woss << L"ImageSystem init _com_error hr=0x" << std::hex << ex.Error() << L" msg=" << wmsg;
            std::wstring w = woss.str();
            std::string n(w.begin(), w.end());
            DEBUGLOG_ERROR(n);
            return;
        }

        // UI用のテキストフォーマットを作成
        CreateTextFormats();

        // 画面サイズを取得
        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        // カメラを初期化（App設定を優先）
        camera_ = Camera::LookAtLH(
            baseFovY_,
            screenWidth / screenHeight,
            cameraNear_,
            cameraFar_,
            cameraPosition_,
            baseTarget_,
            baseUp_);
        // 初期の補間ターゲットを基準注視点に合わせる
        currentTarget_ = baseTarget_;

        // 衝突検出システムをエンティティとして生成
        Entity collisionSystem = world.Create().With<CollisionDetectionSystem>(cfg_CollisionCellSize.Get()).Build();
        ownedEntities_.push_back(collisionSystem);

        // モデル読み込みシステムをエンティティとして生成
        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        CreateSkybox(world);

#if defined(_DEBUG)
        static bool skyboxScaleTestsRan = false;
        if (!skyboxScaleTestsRan) {
            RunSkyboxScaleTests();
            RunPointLightOffsetTests();
            assert(cfg_LimitTime.Get() > 0.0f);
            skyboxScaleTestsRan = true;
        }
#endif

        // ステージ進行状況に応じてステージデータを読み込む
        int initialStage = 1;
        const int maxStage = GetAvailableStageCount(world);
        world.ForEach<StageProgress>([&](Entity e, StageProgress &status) {
            status.Normalize(maxStage, status.worldCount);
            int desiredStage = status.selectStage > 0 ? status.selectStage : 1;
            desiredStage = std::min(desiredStage, maxStage);
            if (desiredStage != status.selectStage) {
                DEBUGLOG_WARNING("[StageCreate] 要求ステージ " + std::to_string(status.selectStage) + " をクランプ: " + std::to_string(desiredStage));
            }

            status.selectStage = desiredStage;
            status.currentStage = desiredStage;
            status.currentRoom = 1; // ステージ開始時は常にroom1から
            status.clearedThisStage = false;
            status.goalTransitioning = false;
            status.requestAdvance = false;
            status.pressedSwitch = false;
            status.goalUnlocked = false;
            DEBUGLOG("[StageCreate] Load stage world=" + std::to_string(status.worldCount) +
                     " stage=" + std::to_string(status.currentStage) +
                     " room=" + std::to_string(status.currentRoom) +
                     " clearedThisStage=" + std::to_string(status.clearedThisStage));

            auto stagePath = ResolveStageRoomCsvPath(status.worldCount, desiredStage, status.currentRoom);
            if (!stagePath) {
                DEBUGLOG_ERROR("[StageCreate] ステージ" + std::to_string(desiredStage) + " の room" + std::to_string(status.currentRoom) + ".csv が見つかりません。Stage1/room1へフォールバックします");
                status.currentStage = 1;
                status.selectStage = 1;
                status.currentRoom = 1;
                stagePath = ResolveStageRoomCsvPath(1, 1, 1);
            }

            if (stagePath) {
                Entity stageEntity = world.Create().With<StageCreate>(*stagePath).Build();
                ownedEntities_.push_back(stageEntity);
            } else {
                DEBUGLOG_ERROR("[StageCreate] 有効なステージCSVが見つからず、ステージ生成をスキップしました");
            }

            initialStage = status.currentStage;
            g_LastStageProgress = status;
            DEBUGLOG("[StageCreate] g_LastStageProgress synced: clearedThisStage=" + std::to_string(g_LastStageProgress.clearedThisStage));
        });

        // 平行光源をエンティティとして生成
        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        CreateUI(world, screenWidth, screenHeight);

        SetupStage(world, initialStage);
        StartFadeInNormal(world);

        // シャドウマップ初期化
        if (!shadowMap_.Init(gfx->Dev(), 1024)) {
            DEBUGLOG("[ERROR] OmniShadowMap::Init() 失敗");
        }
        if (!shadowSystem_.Initialize(*gfx)) {
            DEBUGLOG("[ERROR] ShadowRenderSystem::Initialize() 失敗");
        }

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
        GamepadSystem *pad = ServiceLocator::TryGet<GamepadSystem>();
        world.ForEach<GameStatus>([&](Entity, GameStatus &stats) {
            auto *v = world.TryGet<PlayerVelocity>(playerEntity_);
            auto *movement = world.TryGet<PlayerMovement>(playerEntity_);

            bool togglePause = input.GetKeyDown('P');
            if (pad && pad->GetButtonDown(GamepadSystem::Button_Y)) {
                togglePause = true;
            }
            if (togglePause) {
                stats.isPaused = !stats.isPaused;
                DEBUGLOG(stats.isPaused ? "ゲームが一時停止されました" : "ゲームが再開されました");
            }
            if (stats.isPaused && pad && pad->GetButtonDown(GamepadSystem::Button_B)) {
                stats.isPaused = false;
                DEBUGLOG("ゲームが再開されました");
            }
        });

        // Debug cheatを無効化（誤爆による即クリア防止）

        // ステージクリア待機中の処理
        if (stageClearActive_) {
            DEBUGLOG("[Game::OnUpdate] stageClearActive is TRUE! timer=" + std::to_string(stageClearTimer_));
            stageClearTimer_ += deltaTime;
            if (auto *txt = world.TryGet<UIText>(stageClearTextEntity_)) {
                //txt->text = L"ステージクリア！";
            }
            // 一定時間経過でクリア動画へ
            if (stageClearTimer_ >= cfg_StageClearWait.Get()) {
               
                if (auto *mgr = ServiceLocator::TryGet<SceneManager>()) {
                    mgr->ChangeScene("ClearVideo", world);
                    return;
                }
            }
        }

        // 目標(ゴール)接近時のスロー演出用タイムスケール
        float timeScale = 1.0f;
        float GoalDistance = cfg_GoalDistance;
        float SlowDirection = cfg_SlowDirection;
        bool goalSequenceActive = false;
        if (world.IsAlive(playerEntity_)) {
            goalSequenceActive = world.Has<GoalAttractor>(playerEntity_);
        }
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.goalTransitioning) {
                goalSequenceActive = true;
            }
        });
        if (goalSequenceActive && world.IsAlive(playerEntity_) && world.IsAlive(goalEntity_)) {
            auto *tPlayer = world.TryGet<Transform>(playerEntity_);
            auto *tGoal = world.TryGet<Transform>(goalEntity_);
            if (tPlayer && tGoal) {
                const float dx = tPlayer->position.x - tGoal->position.x;
                const float dz = tPlayer->position.z - tGoal->position.z;
                const float dist = std::sqrt(dx * dx + dz * dz);
                const float slowThreshold = GoalDistance;        // ゴールに近づいたとみなす距離
                if (dist <= slowThreshold && !pendingRespawn_) { // 死亡(リスポーン待機)中はスローにしない
                    timeScale = SlowDirection;                   // スロー演出
                }
            }
        }
        SetGlobalTimeScale(timeScale);

        // ステージ進行リクエストの処理
        HandleStageAdvance(world);
        UpdateStageTransition(world, deltaTime * timeScale);
        UpdateGoalEffectTransform(world);

        // 入力システムの参照を設定
        SetupInputReferences(world, input);

        // 発光マテリアルのパルス効果を更新
        RenderingSystem::GetInstance().UpdateEmissivePulse(world, deltaTime * timeScale);

        // 遅延リスポーンのタイマーを更新
        UpdateDelayedRespawn(deltaTime * timeScale, world);
        UpdateChargeOverlay(world, deltaTime * timeScale);
        UpdateDeathFade(world, deltaTime * timeScale);

        // カメラリアクションを更新
        UpdateCameraReaction(deltaTime * timeScale, world);
        UpdateSkyboxRotation(deltaTime);
        UpdateSkyboxTransform(world);
        UpdateSkyboxTexture(world);
        ChargCameraAction(world);
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        // 時間切れチェック
            if (world.IsAlive(playerEntity_)) {
                CheckTimeLimit(world, playerEntity_,cfg_LimitTime);
            }
        EffekseerManager::GetInstance().Update();


        //ゴールの見た目変更
        UpdateGoal(world);

    }

    /**
     * @brief 毎フレーム呼び出される描画処理
     * @param world ECSワ عالمへの参照
     */
    void OnRender(World &world) override {
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx)
            return;

        // ライティング情報の更新
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        // シャドウレンダリング
        int shadowIdx = RenderingSystem::GetInstance().GetShadowLightIndex();
        try {
            auto &renderer = ServiceLocator::Get<RenderSystem>(); // RenderSystemを取得

            if (shadowIdx >= 0) {
                PointLightGPU pLight = RenderingSystem::GetInstance().GetLightGPU(shadowIdx);
                shadowSystem_.RenderShadows(*gfx, world, pLight.position, pLight.range, shadowMap_);
                gfx->RestoreBackBuffer();
                renderer.SetShadowMap(shadowMap_.GetSRV());
            } else {
                renderer.SetShadowMap(nullptr);
            }

            // メインレンダリング (不透明オブジェクト)
            renderer.Render(world, camera_);

        } catch (...) {
            DEBUGLOG_ERROR("Failed to get RenderSystem from ServiceLocator");
        }

        EffekseerManager::GetInstance().Draw(camera_);

        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.Render(world);
        });

        SOUND_SYS.PlayBGM(cfg_GameMP3Pass);
    }

    /**
     * @brief シーン終了時に呼び出されるクリーンアップ処理
     * @param world ECSワールドへの参照
     */
    void OnExit(World &world) override {
        DEBUGLOG("GameWithUIScene::OnExit() 開始");

        g_GameScene = nullptr;
        RenderingSystem::GetInstance().Shutdown();
        StopGoalEffect();
        EffekseerManager::GetInstance().StopEffect(); // エフェクトを全停止

        // ステージ生成物を優先的に破棄（シーン間で漏れないようにする）
        for (const auto &entity : stageOwnedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);
            }
        }
        stageOwnedEntities_.clear();

        // 念のため、ステージタグを持つものも一括で破棄
        std::vector<Entity> stageTagged;
        world.ForEach<WallTag>([&](Entity e, WallTag &) { stageTagged.push_back(e); });
        world.ForEach<GoalTag>([&](Entity e, GoalTag &) { stageTagged.push_back(e); });
        world.ForEach<StartTag>([&](Entity e, StartTag &) { stageTagged.push_back(e); });
        world.ForEach<GimmickTag>([&](Entity e, GimmickTag &) { stageTagged.push_back(e); });
        for (auto e : stageTagged) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }

        // 残存のStageCreateがあれば破棄（次シーンで二重生成を防ぐ）
        std::vector<Entity> stageCreates;
        world.ForEach<StageCreate>([&](Entity e, StageCreate &) { stageCreates.push_back(e); });
        for (auto e : stageCreates) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }

        std::vector<StageProgress> savedStageProgress;
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            savedStageProgress.push_back(sp);
        });

        const int maxStage = GetAvailableStageCount(world);

        world.DestroyAllEntitiesImmediate(World::Cause::SceneUnload);

        if (savedStageProgress.empty()) {
            DEBUGLOG("[Game::OnExit] savedStageProgress empty, using g_LastStageProgress world=" +
                     std::to_string(g_LastStageProgress.worldCount) +
                     " select=" + std::to_string(g_LastStageProgress.selectStage));
            savedStageProgress.push_back(g_LastStageProgress);
        }
        for (auto &sp : savedStageProgress) {
            sp.Normalize(maxStage, sp.worldCount);
            sp.currentRoom = 1;
            sp.clearedThisStage = false;
            sp.requestAdvance = false;
            sp.goalTransitioning = false;
            sp.pressedSwitch = false;
            sp.goalUnlocked = false;
            sp.hasSwitch = false;
            sp.IsWorldBack = false;
            sp.IsWorldNext = false;
            sp.IsClearBack = false;
            DEBUGLOG("[Game::OnExit] restore StageProgress world=" + std::to_string(sp.worldCount) +
                     " select=" + std::to_string(sp.selectStage) +
                     " current=" + std::to_string(sp.currentStage));
        }

        for (const auto &sp : savedStageProgress) {
            world.Create().With<StageProgress>(sp).Build();
        }

        for (const auto &entity : ownedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        shadowSystem_.Shutdown();
        shadowMap_.Shutdown();
        skybox_.Shutdown();

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
        shakeIntensity_ = intensity;
        shakeTime_ = duration;
        shakeElapsed_ = 0.0f;
        shakeActive_ = true;
    }

    /**
     * @brief カメラインパルス（衝撃）を開始する
     */
    void TriggerCameraImpulse(float dirX, float dirY, float dirZ, float intensity, float duration) {
        impulseDir_ = {dirX, dirY, dirZ};
        impulseIntensity_ = intensity;
        impulseTime_ = duration * 1.5f;
        impulseElapsed_ = 0.0f;
        impulseActive_ = true;
    }
    
    /**
     * @brief カメラズームを開始する
     */
    void TriggerCameraZoom(float zoomAmount, float duration) {
        zoomAmount_ = zoomAmount;
        zoomTime_ = duration;
        zoomElapsed_ = 0.0f;
        zoomActive_ = true;
    }

    /**
     * @brief 現在のカメラリアクションをすべて停止
     */
    void StopCameraReaction() {
        shakeActive_ = false;
        impulseActive_ = false;
        zoomActive_ = false;
        shakeElapsed_ = shakeTime_ = 0.0f;
        impulseElapsed_ = impulseTime_ = 0.0f;
        zoomElapsed_ = zoomTime_ = 0.0f;
        stickZoomTarget_ = 0.0f;
        stickZoomCurrent_ = 0.0f;
        camera_.position = cameraPosition_;
        camera_.target = baseTarget_;
        camera_.fovY = baseFovY_;
        camera_.up = baseUp_;
        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    // チャージ開始/解放演出

    /**
     * @brief チャージ状態を即座にリセット（死亡・ゴール時用）
     * @details 灰色フェード、カメラズーム、寄り率をすべてリセット
     */
    void ResetChargeState() {
        SetStickZoomActive(false);
        chargeOverlayCurrent_ = 0.0f;
        chargeOverlayTarget_ = 0.0f;
        chargeOverlayVisible_ = false;
        stickZoomRatioCurrent_ = 0.0f;
    }

    void OnChargeStart(World &world) {
        // Disabled gray fade overlay on charge start
        // (previously set chargeOverlayTarget_ and chargeOverlayVisible_)
        SetStickZoomActive(true);
        PlayPlayerAnimation(world, AnimationConfig::Clips::PlayerCharge, true);

        SOUND_SYS.PlaySE(cfg_DriftMP3Pass.Get(),false);
    }
    void OnChargeRelease(World &world, float chargeAmount01) {
        SetStickZoomActive(false);
        // Disabled gray fade overlay on charge release
        // (previously adjusted chargeOverlayCurrent_/Target_/Visible here)
        float impulse = std::clamp(chargeAmount01, 0.15f, 1.0f) * 0.12f;
        TriggerCameraShake(0.03f + impulse, 0.25f);
        PlayPlayerAnimation(world, AnimationConfig::Clips::PlayerChargeOut, false);

        SOUND_SYS.PlaySE(cfg_Fire1MP3Pass.Get(),true);
    }

    void UpdateDeathFade(World &world, float dt /*dt*/) {

        if (isDeathFadePending_) {
            deathFadeTimer_ -= dt;
            if (deathFadeTimer_ <= 0.0f) {
                StartDeathFadeOut(world);
                isDeathFadePending_ = false;
            }
        }

        if (!world.IsAlive(deathFadeAnimationEntity_))
            return;

        auto *img = world.TryGet<UIImage>(deathFadeAnimationEntity_);
        auto *anim = world.TryGet<SpriteSheetAnimation>(deathFadeAnimationEntity_);

        if (deathFadeVisible_) {
            if (img)
                img->opacity = 1.0f;
            if (anim && anim->isFinished && !anim->isPlaying) {
                deathFadeVisible_ = false;
                if (img)
                    img->opacity = 0.0f;
            }
            SOUND_SYS.StopSE(cfg_DriftMP3Pass);
        } else {
            if (img)
                img->opacity = 0.0f;
        }
    }

    void UpdateGoal(World &world) {
        bool pressed = false;
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) { pressed = sp.goalUnlocked; });

        if (world.IsAlive(goalEntity_)) {
            if (auto *emissive = world.TryGet<EmissiveMaterial>(goalEntity_)) {
                if (pressed) {
                    emissive->emissiveColor = {1.0f, 0.0f, 0.0f};
                } else {
                    emissive->emissiveColor = {0.0f, 1.0f, 1.0f};
                }
            }
        }
    }

    static constexpr bool IsGoalCheatCombo(
        bool ctrlHeld,
        bool zHeld,
        bool xHeld,
        bool cHeld,
        bool vHeld,
        bool ctrlDown,
        bool zDown,
        bool xDown,
        bool cDown,
        bool vDown) {
        const bool comboHeld = ctrlHeld && zHeld && xHeld && cHeld && vHeld;
        if (!comboHeld) {
            return false;
        }
        return ctrlDown || zDown || xDown || cDown || vDown;
    }

    /** @brief Detect goal cheat key combo. */
    bool IsGoalCheatTriggered(const InputSystem &input) const {
        return IsGoalCheatCombo(
            input.GetKey(VK_CONTROL),
            input.GetKey('Z'),
            input.GetKey('X'),
            input.GetKey('C'),
            input.GetKey('V'),
            input.GetKeyDown(VK_CONTROL),
            input.GetKeyDown('Z'),
            input.GetKeyDown('X'),
            input.GetKeyDown('C'),
            input.GetKeyDown('V'));
    }

    void TriggerGoalCheat(World &world) {
        if (stageClearActive_ || pendingStageAdvance_.active) {
            return;
        }

        bool isGoalTransitioning = false;
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.goalTransitioning || sp.requestAdvance) {
                isGoalTransitioning = true;
            }
        });
        if (isGoalTransitioning) {
            return;
        }

        if (!world.IsAlive(playerEntity_)) {
            DEBUGLOG_WARNING("Goal cheat ignored: player not alive");
            return;
        }

        DirectX::XMFLOAT3 goalCenter{};
        bool hasGoalTarget = false;
        if (world.IsAlive(goalEntity_)) {
            if (auto *tGoal = world.TryGet<Transform>(goalEntity_)) {
                goalCenter = ResolvePlacementCenter(world, goalEntity_, *tGoal);
                hasGoalTarget = true;
                if (auto *goalTag = world.TryGet<GoalTag>(goalEntity_)) {
                    goalTag->consumed = true;
                }
            }
        }

        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            sp.goalTransitioning = true;
        });

        GameScene_ResetChargeState();

        if (auto *v = world.TryGet<PlayerVelocity>(playerEntity_)) {
            v->velocity = {0.0f, 0.0f};
            v->isBoosting = false;
            v->isDecelerating = false;
            v->boostSpeed = 0.0f;
            v->boostDir = {0.0f, 0.0f};
        }

        if (hasGoalTarget) {
            if (world.Has<GoalAttractor>(playerEntity_)) {
                if (auto *attractor = world.TryGet<GoalAttractor>(playerEntity_)) {
                    attractor->target = goalCenter;
                    attractor->elapsed = 0.0f;
                }
            } else {
                GoalAttractor attract;
                attract.target = goalCenter;
                world.Add<GoalAttractor>(playerEntity_, attract);
            }
            DEBUGLOG("Goal cheat: started goal transition");
            return;
        }

        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            if (sp.clearedThisStage) {
                return;
            }
            sp.clearedThisStage = true;
            StageSave::MarkStageCleared(sp.currentStage);
            sp.requestAdvance = true;
        });
        world.ForEach<GameStatus>([](Entity, GameStatus &stats) {
            stats.elapsedTime = cfg_LimitTime;
            stats.timerRunning = false;
            stats.waitingForPlayerMove = true;
        });
        DEBUGLOG_WARNING("Goal cheat: goal entity missing, advanced stage directly");
    }

    /** @brief Get camera reference. */
    const Camera &GetCamera() const {
        return camera_;
    }

    /** @brief カメラの基準位置を設定 */
    void SetCameraBasePosition(const DirectX::XMFLOAT3 &pos) {
        cameraPosition_ = pos;
    }
    /** @brief カメラの基準設定を一括で指定（位置・注視点・Up・FOV・Near/Far） */
    void ConfigureBaseCamera(const DirectX::XMFLOAT3 &pos, const DirectX::XMFLOAT3 &target, const DirectX::XMFLOAT3 &up, float fovRad, float nearZ, float farZ) {
        cameraPosition_ = pos;
        baseTarget_ = target;
        baseUp_ = up;
        baseFovY_ = fovRad;
        cameraNear_ = nearZ;
        cameraFar_ = farZ;
    }

    /** @brief プレイヤーを弾いたときの画面の揺れ */
    void ChargCameraAction(World &world) {
        world.ForEach<PlayerMovement>([&](Entity e, PlayerMovement &player) {
            float gx = player.gamepad_->GetLeftStickX();
            float gy = player.gamepad_->GetLeftStickY();
            float mag = std::sqrt(gx * gx + gy * gy);

            if (mag > PlayerConstants::EPSILON) {
                player.lastStickDir_.x = -(gx / mag);
                player.lastStickDir_.y = -(gy / mag);
            }

            const float releaseThreshold = cfg_ReleaseThreshold;
            bool chargingNowLocal = (mag > releaseThreshold);

            bool releasedSys = player.gamepad_->IsLeftStickReleased();
            bool releasedLocal = (player.wasCharging_ && !chargingNowLocal);

            if (releasedSys || releasedLocal) {
                if (auto *pv = world.TryGet<PlayerVelocity>(playerEntity_)) {
                    const float vx = pv->velocity.x;
                    const float vy = pv->velocity.y;
                    const float len = std::hypot(vx, vy);
                    if (len > 1e-5f) {
                        const float dirX = vx / len;
                        const float dirY = vy / len;
                        TriggerCameraImpulse(dirX, 0.0f, dirY, 0.2f, 0.1f);
                        if (static_cast<bool>(pv->velocity.x + pv->velocity.y)) {
                            float vecX = pv->velocity.x / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                            float vecY = pv->velocity.y / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;

                            TriggerCameraImpulse(vecX, 0.0f, vecY, 0.1f, 0.07f);
                        }
                    };
                }
            }
        });
    }

    /**
     * @brief 壁衝突時の処理（カメラシェイク＋遅延リスポーン）
     */
    void OnWallHit(Entity player, World &world) {
        if (pendingRespawn_)
            return;
        ResetChargeState();

        // 再生用フェードアニメーションを開始
        deathFadeTimer_ = cfg_DeathAnimationFadeTime;
        isDeathFadePending_ = true;

        PlayPlayerAnimation(world, AnimationConfig::Clips::PlayerDeath, false);

        if (auto *playerStatus = world.TryGet<PlayerStatus>(playerEntity_)) {
            UpdateWallHitState(*playerStatus, PlayerStatus::WallHitState::Shaking, "WallHit");
        }

        if (auto *pv = world.TryGet<PlayerVelocity>(playerEntity_)) {
            if (static_cast<bool>(pv->velocity.x + pv->velocity.y)) {
                float vecX = pv->velocity.x / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                float vecY = pv->velocity.y / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                TriggerCameraImpulse(vecX, 0.0f, vecY, 0.2f, 0.04f);
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
        respawnTimer_ = cfg_WallHitRespawnDelay.Get() + deathFadeTimer_;

        DEBUGLOG("壁に衝突 - カメラシェイク開始、リスポーン待機中");
    }

    /** @brief タイムアップ時の処理（フェード＋遅延リスポーン） */
    void OnTimeUp(Entity player, World &world) {
        if (pendingRespawn_)
            return;
        ResetChargeState();

        deathFadeTimer_ = cfg_DeathAnimationFadeTime;
        isDeathFadePending_ = true;

        PlayPlayerAnimation(world, AnimationConfig::Clips::PlayerTimeup, false);

        if (auto *playerStatus = world.TryGet<PlayerStatus>(playerEntity_)) {
            UpdateWallHitState(*playerStatus, PlayerStatus::WallHitState::Shaking, "TimeUp");
        }

        World *targetWorld = world_ ? world_ : &world;
        if (targetWorld && targetWorld->IsAlive(player)) {
            if (auto *v = targetWorld->TryGet<PlayerVelocity>(player)) {
                v->velocity = {0.0f, 0.0f};
                v->isBoosting = false;
                v->isDecelerating = false;
                v->boostSpeed = 0.0f;
            }
        }

        SOUND_SYS.PlaySE(cfg_DeathMP3Pass.Get(),false);
        SOUND_SYS.StopSE(cfg_DriftMP3Pass.Get());

        pendingRespawn_ = true;
        respawnPlayer_ = player;
        respawnTimer_ = cfg_WallHitRespawnDelay.Get() + deathFadeTimer_;

        DEBUGLOG("時間切れ - フェード演出開始、リスポーン待機中");
    }

    /** @brief ワールドへのポインタを設定 */
    void SetWorldRef(World *w) {
        world_ = w;
    }

    /** @brief リスポーン待機中かを取得 */
    bool IsRespawnPending() const {
        return pendingRespawn_;
    }

  private:
    static float GetCameraYawDeg(const Camera &cam) {
        using namespace DirectX;
        const XMVECTOR pos = XMLoadFloat3(&cam.position);
        const XMVECTOR target = XMLoadFloat3(&cam.target);
        XMVECTOR dir = XMVectorSubtract(target, pos);
        dir = XMVector3Normalize(dir);

        XMFLOAT3 d{};
        XMStoreFloat3(&d, dir);
        const float yawRad = atan2f(d.x, d.z);
        return XMConvertToDegrees(yawRad);
    }

    static float SkyboxRotationToDegrees(float rotationRad) {
        return DirectX::XMConvertToDegrees(rotationRad);
    }

    static float BuildSkyboxYawDeg(const Camera &cam, float rotationRad) {
        return GetCameraYawDeg(cam) + SkyboxRotationToDegrees(rotationRad);
    }
    struct StageAdvanceInfo {
        bool active = false;
        bool stageBuilt = false;
        bool stageResetPending = false;
        int stage = 0;
        int nextRoom = 0;
        std::string nextRoomPath;
    };

    SkyboxSystem skybox_;
    OmniShadowMap shadowMap_;
    ShadowRenderSystem shadowSystem_;

    static constexpr float WALL_MESH_Y_OFFSET = 2.3f;
    static constexpr float WALL_COLLISION_CENTER_OFFSET = 1.8f;

    // =========================================
    // 初期化ヘルパーメソッド
    // =========================================

    bool PlayPlayerAnimation(World &world, const std::string &clipName, bool loop = false) {
        if (!world.IsAlive(playerEntity_))
            return false;
        return AnimationTools::Play(world, playerEntity_, clipName, loop);
    }

    void CreateTextFormats();
    void CreateUI(World &world, float screenWidth, float screenHeight);

    void CreatePlayer(World &world) {
        if (world.IsAlive(playerEntity_)) {
            if (world.Has<GoalAttractor>(playerEntity_)) {
                world.Remove<GoalAttractor>(playerEntity_);
                DEBUGLOG("[CreatePlayer] Removed GoalAttractor from existing player");
            }
        }

        float s = cfg_PlayerScale;
        Transform transform{{0.0f, cfg_PlayerStartY, 0.0f}, {0.0f, 0.0f, 0.0f}, {s, s, s}};

        // モデル＆アニメーションをConfig経由でロード
        const std::string modelPath = AnimationConfig::Paths::PlayerModel;
        std::vector<std::string> animPaths = {
            AnimationConfig::Paths::PlayerAnimFry,
            AnimationConfig::Paths::PlayerAnimCharge,
            AnimationConfig::Paths::PlayerAnimChargeIn,
            AnimationConfig::Paths::PlayerAnimChargeOut,
            AnimationConfig::Paths::PlayerAnimDeath,
            AnimationConfig::Paths::PlayerAnimTimeup,
        };
        std::vector<std::string> animAliases = {
            AnimationConfig::Clips::PlayerIdle,
            AnimationConfig::Clips::PlayerCharge,
            AnimationConfig::Clips::PlayerChargeIn,
            AnimationConfig::Clips::PlayerChargeOut,
            AnimationConfig::Clips::PlayerDeath,
            AnimationConfig::Clips::PlayerTimeup,
        };

        std::vector<ModelPrefabNode> nodes = ModelLoader::LoadModel(modelPath);
        auto clips = AnimationTools::LoadClipsFromFiles(animPaths, {AnimationConfig::Paths::PlayerAnimFallback}, animAliases);

        // メッシュノードを探す
        ModelPrefabNode *targetNode = nullptr;
        for (auto &node : nodes) {
            if (node.hasMesh) {
                targetNode = &node;
                break;
            }
        }
        if (!targetNode && !nodes.empty())
            targetNode = &nodes[0]; // メッシュなくてもルートを使う

        // エンティティ作成
        Entity player = world.CreateEntity();

        world.Add<Transform>(player, transform);
        world.Add<TransformHierarchy>(player);

        // ModelComponent (Mesh & Skeleton)
        int targetIndex = -1;
        if (targetNode) {
            targetIndex = static_cast<int>(targetNode - &nodes[0]);
        }
        if (targetNode && targetNode->hasMesh) {
            world.Add<ModelComponent>(player, targetNode->component);

            std::string defaultClip = AnimationConfig::Clips::PlayerDefault;
            if (targetNode->component.isSkinned) {
                const bool defaultExists = std::any_of(clips.begin(), clips.end(), [&](const auto &c) { return c.name == defaultClip; });
                if (!defaultExists && !clips.empty()) {
                    defaultClip = clips.front().name;
                }
                const bool hasClips = AnimationTools::InitAnimator(world, player, clips, defaultClip);
                if (!hasClips) {
                    DEBUGLOG_WARNING("No animation clips loaded for player.");
                } else {
                    DEBUGLOG("Playing animation: " + defaultClip);
                }
            }

            // 追加ノードを生成（ローカルTRSと親子関係を維持）
            std::vector<Entity> created(nodes.size());
            if (targetIndex >= 0 && static_cast<size_t>(targetIndex) < created.size()) {
                created[targetIndex] = player;
            }
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (static_cast<int>(i) == targetIndex) continue;
                const auto &node = nodes[i];
                Entity child = world.CreateEntity();
                world.Add<Transform>(child, Transform{node.translation, node.rotationDeg, node.scale});
                world.Add<TransformHierarchy>(child);
                if (node.hasMesh && node.component.indexCount > 0) {
                    world.Add<ModelComponent>(child, node.component);
                    if (node.component.isSkinned && !clips.empty()) {
                        AnimationTools::InitAnimator(world, child, clips, defaultClip);
                    }
                }
                created[i] = child;
            }

            for (size_t i = 0; i < nodes.size(); ++i) {
                Entity child = (static_cast<int>(i) == targetIndex) ? player : created[i];
                if (!world.IsAlive(child)) continue;
                int pIdx = nodes[i].parentIndex;
                Entity parent = player;
                if (pIdx >= 0 && static_cast<size_t>(pIdx) < created.size()) {
                    parent = (pIdx == targetIndex) ? player : created[pIdx];
                }
                auto *ch = world.TryGet<TransformHierarchy>(child);
                auto *ph = world.TryGet<TransformHierarchy>(parent);
                if (ch && ph && child != parent) {
                    ch->SetParent(parent);
                    ph->AddChild(child);
                }
            }
        } else {
            world.Add<Model>(player, modelPath);
        }

        // Player固有コンポーネント
        world.Add<PlayerTag>(player);
        world.Add<PlayerVelocity>(player);
        world.Add<PlayerMovement>(player);
        world.Add<PlayerStatus>(player);
        world.Add<PlayerGuide>(player);
        world.Add<CollisionSphere>(player, 0.4f);
        world.Add<PlayerCollisionHandler>(player);

        playerEntity_ = player;
        ownedEntities_.push_back(player);
    }

    // =========================================
    // 更新ヘルパーメソッド
    // =========================================

    void HandleStageAdvance(World &world) {
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            DEBUGLOG("[HandleStageAdvance] requestAdvance=" + std::to_string(sp.requestAdvance) +
                     " clearedThisStage=" + std::to_string(sp.clearedThisStage) +
                     " goalTransitioning=" + std::to_string(sp.goalTransitioning));
            if (sp.requestAdvance) {
                sp.requestAdvance = false;

                // 同一ステージ内で次のroomへ
                const int nextRoomIndex = sp.currentRoom + 1;
                auto nextRoomPath = ResolveStageRoomCsvPath(sp.worldCount, sp.currentStage, nextRoomIndex);
                if (!nextRoomPath) {
                    DEBUGLOG_WARNING("[StageCreate] Stage" + std::to_string(sp.currentStage) + "/room" + std::to_string(nextRoomIndex) + ".csv が見つかりません。ステージクリア扱いにします");

                    DEBUGLOG("[HandleStageAdvance] stageClearActive set to true! currentStage=" + std::to_string(sp.currentStage) +
                             " currentRoom=" + std::to_string(sp.currentRoom) +
                             " nextRoom=" + std::to_string(nextRoomIndex));
                    stageClearActive_ = true;
                    stageClearTimer_ = 0.0f;

                    world.ForEach<GameStatus>([](Entity, GameStatus &stats) {
                        stats.timerRunning = false;
                    });

                    if (world.IsAlive(stageClearTextEntity_)) {
                        if (auto *txt = world.TryGet<UIText>(stageClearTextEntity_)) {
                        }
                    }
                    ClearGoalTransitionFlag(world);
                    return;
                }

                if (pendingStageAdvance_.active) {
                    DEBUGLOG_WARNING("[StageCreate] 既に遷移中のため新規リクエストを破棄しました");
                    return;
                }

                pendingStageAdvance_.active = true;
                pendingStageAdvance_.stageBuilt = false;
                pendingStageAdvance_.stage = sp.currentStage;
                pendingStageAdvance_.nextRoom = nextRoomIndex;
                pendingStageAdvance_.nextRoomPath = *nextRoomPath;
                stageAdvanceTimer_ = 0.0f;
                pendingStageAdvance_.stageResetPending = false;

                StartFadeOutNormal(world);
                DEBUGLOG("同一ステージ内で次のルームへ進行(フェード演出開始): Stage" + std::to_string(sp.currentStage) + ", room" + std::to_string(pendingStageAdvance_.nextRoom));
            }
        });
    }

    void UpdateStageTransition(World &world, float dt) {
        if (!pendingStageAdvance_.active)
            return;

        stageAdvanceTimer_ += dt;
        auto *anim = world.TryGet<SpriteSheetAnimation>(fadeAnimationEntity_);
        const float fadeDuration = GetFadeDurationSeconds(world, fadeAnimationEntity_);
        const float waitDuration = (fadeDuration > 0.0f) ? fadeDuration : 1.0f;

        const bool fadeOutFinished = anim && !anim->isPlaying && anim->isFinished && anim->playbackDirection >= 0;
        if (!pendingStageAdvance_.stageBuilt) {
            if (fadeOutFinished || stageAdvanceTimer_ >= waitDuration) {
                stageAdvanceTimer_ = 0.0f;

                if (!pendingStageAdvance_.stageResetPending) {
                    std::vector<Entity> stageCreateEntities;
                    world.ForEach<StageCreate>([&](Entity e, StageCreate &) { stageCreateEntities.push_back(e); });
                    for (auto e : stageCreateEntities) {
                        if (world.IsAlive(e)) {
                            world.DestroyEntityWithCause(e, World::Cause::StageReset);
                        }
                    }
                    world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
                        sp.currentRoom = pendingStageAdvance_.nextRoom;
                    });
                    pendingStageAdvance_.stageResetPending = true;
                    return;
                }

                SOUND_SYS.PlaySE(cfg_SpeedUpMP3Pass.Get(),false);

                Entity newStageEntity = world.Create().With<StageCreate>(pendingStageAdvance_.nextRoomPath).Build();
                ownedEntities_.push_back(newStageEntity);

                DEBUGLOG("同一ステージ内で次のルームへ進行: Stage" + std::to_string(pendingStageAdvance_.stage) + ", room" + std::to_string(pendingStageAdvance_.nextRoom));
                SetupStage(world, pendingStageAdvance_.stage);
                pendingStageAdvance_.stageResetPending = false;
                pendingStageAdvance_.stageBuilt = true;
                StartFadeInNormal(world);
            }
            return;
        }

        const bool fadeInFinished = anim && !anim->isPlaying && anim->isFinished && anim->playbackDirection < 0;
        if (fadeInFinished || stageAdvanceTimer_ >= waitDuration) {
            pendingStageAdvance_ = {};
            stageAdvanceTimer_ = 0.0f;
            ClearGoalTransitionFlag(world);
        }
    }

    void StartFadeOutNormal(World &world) {
        StartSpriteFade(world, fadeAnimationEntity_, 1, false);
    }
    void StartFadeInNormal(World &world) {
        StartSpriteFade(world, fadeAnimationEntity_, -1, false);
    }
    void StartDeathFadeOut(World &world) {
        deathFadeVisible_ = true;
        StartSpriteFade(world, deathFadeAnimationEntity_, 1, true);
    }

    void StartSpriteFade(World &world, Entity target, int direction, bool forceOpaque) {
        if (!world.IsAlive(target))
            return;
        AnimationTools::PlaySpriteSheet(world, target, direction, /*loop*/ false, /*reset*/ true);
        if (auto *img = world.TryGet<UIImage>(target)) {
            img->opacity = 1.0f;
        }
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(target)) {
            anim->isFinished = false;
        }
    }

    float GetFadeDurationSeconds(World &world, Entity target) const {
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(target)) {
            return AnimationTools::DurationSeconds(*anim);
        }
        return 0.0f;
    }

    void SetupInputReferences(World &world, InputSystem &input) {
        world.ForEach<PlayerMovement>([&](Entity, PlayerMovement &pm) {
            if (!pm.input_)
                pm.input_ = &input;
            if (!pm.gamepad_)
                pm.gamepad_ = &ServiceLocator::Get<GamepadSystem>();
        });

        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_)
                sys.input_ = &input;
        });
    }

    std::optional<std::string> ResolveSpeedUpCsvPath(World &world, const std::string &stageCollisionCsvPath) {
        DEBUGLOG("ResolveSpeedUpCsvPath");
        namespace fs = std::filesystem;
        fs::path worldcount = ("World");
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            worldcount += (std::to_string(sp.worldCount));
        });

        fs::path collisionPath(stageCollisionCsvPath);

        const fs::path stageDir = collisionPath.parent_path().filename();
        if (stageDir.empty()) {
            DEBUGLOG("パスが空です:" + stageCollisionCsvPath);
            return std::nullopt;
        }

        fs::path speedUpPath = fs::path("Assets/StageData") / worldcount / ("UniqueObj/SpeedUp") / stageDir / collisionPath.filename();
        std::error_code ec;
        if (!fs::exists(speedUpPath, ec) || ec) {
            DEBUGLOG(speedUpPath.string());
            return std::nullopt;
        }

        return speedUpPath.string();
    }

    std::optional<std::string> ResolveMovingObstacleCsvPath(World& world,const std::string &stageCollisionCsvPath) {
        namespace fs = std::filesystem;
        fs::path collisionPath(stageCollisionCsvPath);

        fs::path worldcount = ("World");
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            worldcount += (std::to_string(sp.worldCount));
        });

        const fs::path stageDir = collisionPath.parent_path().filename();
        if (stageDir.empty()) {
            return std::nullopt;
        }

        fs::path movePath = fs::path("Assets/StageData") / worldcount / ("UniqueObj/Move") / stageDir / collisionPath.filename();
        std::error_code ec;
        if (!fs::exists(movePath, ec) || ec) {
            return std::nullopt;
        }

        return movePath.string();
    }

    std::optional<std::string> ResolveLimitTimePath(World &world, const std::string &limitTimeCsvPath) {
        namespace fs = std::filesystem;
        fs::path timePath(limitTimeCsvPath);

        fs::path worldcount = ("World");
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            worldcount += (std::to_string(sp.worldCount));
        });

        fs::path stagecount = ("stage");
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            stagecount += (std::to_string(sp.currentStage));    
        });
        stagecount += (".csv");

       /* const fs::path stageDir = timePath.parent_path().filename();
        if (stageDir.empty()) {
            return std::nullopt;
        }*/

        fs::path limitTimePath = fs::path("Assets/StageData") / worldcount / ("StageTime") / stagecount;
        std::error_code ec;
        if (!fs::exists(limitTimePath, ec) || ec) {
            return std::nullopt;
        }

        return limitTimePath.string();
    }

    std::vector<std::vector<int>> LoadAngleCsv(const std::string &csvPath) {
        std::vector<std::vector<int>> angles;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG("[SpeedUp] 角度CSVが開けません(仕様によりスキップ): " + csvPath);
            return angles;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            std::vector<int> row;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ',')) {
                try {
                    row.push_back(std::stoi(cell));
                } catch (const std::exception &ex) {
                    DEBUGLOG_WARNING(std::string("[SpeedUp] CSVパース失敗: ") + cell + " (" + ex.what() + ")");
                }
            }
            if (!row.empty()) {
                angles.push_back(row);
            }
        }

        return angles;
    }

    std::vector<std::vector<int>> LoadGoalAngleCsv(const std::string &csvPath) {
        std::vector<std::vector<int>> goalangles;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG("[Goal] 角度CSVが開けません(仕様によりスキップ): " + csvPath);
            return goalangles;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            std::vector<int> row;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ',')) {
                try {
                    row.push_back(std::stoi(cell));
                } catch (const std::exception &ex) {
                    DEBUGLOG_WARNING(std::string("[Goal] CSVパース失敗: ") + cell + " (" + ex.what() + ")");
                }
            }
            if (!row.empty()) {
                goalangles.push_back(row);
            }
        }

        return goalangles;
    }

    std::vector<MovingObstaclePattern> LoadMovingObstacleCsv(const std::string &csvPath) {
        std::vector<MovingObstaclePattern> patterns;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG("[MoveObstacle] CSVが開けません(仕様によりスキップ): " + csvPath);
            return patterns;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;
            std::stringstream ss(line);
            std::string cell;
            std::array<float, 5> vals{};
            int idx = 0;
            while (std::getline(ss, cell, ',') && idx < 5) {
                try {
                    vals[idx] = std::stof(cell);
                } catch (const std::exception &ex) {
                    DEBUGLOG_WARNING(std::string("[MoveObstacle] CSVパース失敗: ") + cell + " (" + ex.what() + ")");
                }
                ++idx;
            }
            if (idx >= 5) {
                MovingObstaclePattern p;
                p.dirX = vals[0];
                p.dirY = -vals[1];
                p.waitAtStart = vals[2];
                p.waitAtEnd = vals[3];
                p.travelTime = std::max(vals[4], 0.0001f);
                patterns.push_back(p);
            }
        }

        return patterns;
    }

    std::vector<std::vector<int>> LoadTimeCsv(const std::string &csvPath) {
        std::vector<std::vector<int>> limitTime;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG("[Time] 角度CSVが開けません(仕様によりスキップ): " + csvPath);
            return limitTime;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            std::vector<int> row;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ',')) {
                try {
                    row.push_back(std::stoi(cell));
                } catch (const std::exception &ex) {
                    DEBUGLOG_WARNING(std::string("[Time] CSVパース失敗: ") + cell + " (" + ex.what() + ")");
                }
            }
            if (!row.empty()) {
                limitTime.push_back(row);
            }
        }

        return limitTime;
    }

    // =========================================
    // ステージ生成メソッド
    // =========================================

    void ApplyDefaultPointLightParams(PointLight &light) const {
        light.SetAttenuation(cfg_PointLightConst.Get(), cfg_PointLightLinear.Get(), cfg_PointLightQuadratic.Get());
    }

    void CreateStageMap(World &world, const StageCreate &stagecreate, int stagenumber) {
        float tileSize = 1.0f;
        bool switchFound = false;

        if (stagecreate.stageMap.empty() || stagecreate.stageMap[0].empty()) {
            return;
        }

        float mapWidth = static_cast<float>(stagecreate.stageMap[0].size());
        float mapHeight = static_cast<float>(stagecreate.stageMap.size());

        const int max_x_index = static_cast<int>(stagecreate.stageMap[0].size() - 1);
        const int max_y_index = static_cast<int>(stagecreate.stageMap.size() - 1);

        const float offsetX = (mapWidth * tileSize) * 0.5f - (tileSize * 0.5f);
        const float offsetZ = (mapHeight * tileSize) * 0.5f - (tileSize * 0.5f);

        for (int y = 0; y < static_cast<int>(stagecreate.stageMap.size()); ++y) {
            for (int x = 0; x < static_cast<int>(stagecreate.stageMap[y].size()); ++x) {
                int blockType = stagecreate.stageMap[y][x];

                float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                float worldY = 0.0f;
                float worldZ = offsetZ - (static_cast<float>(y) * tileSize);

                const DirectX::XMFLOAT3 blockposition = {worldX, worldY, worldZ};

                CreateFloor(world, blockposition);

                // ステージ境界壁
                if (y == 0)
                    CreatFloorWall(world, {worldX, worldY, worldZ + tileSize});
                if (y == max_y_index)
                    CreatFloorWall(world, {worldX, worldY, worldZ - tileSize});
                if (x == 0)
                    CreatFloorWall(world, {worldX - tileSize, worldY, worldZ});
                if (x == max_x_index)
                    CreatFloorWall(world, {worldX + tileSize, worldY, worldZ});
                if (x == 0 && y == 0)
                    CreatFloorWall(world, {worldX - tileSize, worldY, worldZ + tileSize});
                if (x == max_x_index && y == 0)
                    CreatFloorWall(world, {worldX + tileSize, worldY, worldZ + tileSize});
                if (x == 0 && y == max_y_index)
                    CreatFloorWall(world, {worldX - tileSize, worldY, worldZ - tileSize});
                if (x == max_x_index && y == max_y_index)
                    CreatFloorWall(world, {worldX + tileSize, worldY, worldZ - tileSize});

                if (blockType != 0) {
                    if (blockType == 64 && !switchFound)
                        switchFound = true;
                    CreateBlockByType(world, blockposition, blockType, stagenumber);
                }
            }
        }

        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            sp.hasSwitch = switchFound;
            sp.goalUnlocked = !switchFound;
        });

        BakeStageLights(world, stagecreate.stageMap, tileSize);
    }

    void BakeStageLights(World &world, const std::vector<std::vector<int>> &stageMap, float tileSize) {
        // 既存のポイントライトに対して、ステージスケールに応じた減衰・レンジを適用するだけ（新規ライトは生成しない）
        if (stageMap.empty() || stageMap[0].empty())
            return;

        const float mapWidth = static_cast<float>(stageMap[0].size());
        const float mapHeight = static_cast<float>(stageMap.size());
        const float offsetX = (mapWidth * tileSize) * 0.5f - (tileSize * 0.5f);
        const float offsetZ = (mapHeight * tileSize) * 0.5f - (tileSize * 0.5f);

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        auto isWalkable = [](int block) {
            // 0:空き, 1:Start, 2:Goal, 10-19:移動物, 30-39:ダッシュ板, 50+オブジェクトなどは許容
            if (block == 0 || block == 1 || block == 2)
                return true;
            if (block >= 10 && block < 20)
                return true;
            if (block >= 30 && block < 40)
                return true;
            if (block >= 50)
                return true;
            return false; // 3系などの壁
        };

        for (int y = 0; y < static_cast<int>(stageMap.size()); ++y) {
            for (int x = 0; x < static_cast<int>(stageMap[y].size()); ++x) {
                if (!isWalkable(stageMap[y][x]))
                    continue;
                float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                float worldZ = offsetZ - (static_cast<float>(y) * tileSize);
                minX = std::min(minX, worldX);
                maxX = std::max(maxX, worldX);
                minZ = std::min(minZ, worldZ);
                maxZ = std::max(maxZ, worldZ);
            }
        }

        if (minX > maxX || minZ > maxZ)
            return;

        const float diag = std::sqrt((maxX - minX) * (maxX - minX) + (maxZ - minZ) * (maxZ - minZ));
        const float targetRange = std::max(cfg_PointLightRange.Get(), 0.3f * diag);

        world.ForEach<PointLight>([&](Entity, PointLight &pl) {
            ApplyDefaultPointLightParams(pl);
            pl.range = targetRange;
        });
    }

    void CreateBlockByType(World &world, const DirectX::XMFLOAT3 &position, int blockType, int stagenumber) {
        float lightangle = 0;
        DirectX::XMFLOAT3 lightpos = {0.0f, -2.0f, 0.0f};
        switch (blockType) {
            case 1:
                CreateStart(world, position);
                break;
            case 2:
                CreateGoal(world, position, stagenumber);
                break;
            case 3:
                CreateWall(world, position);
                break;
            case 5:
                CreateRightDownCorner(world, position);
                break;
            case 6:
                CreateLeftDownCorner(world, position);
                break;
            case 7:
                CreateLeftUpCorner(world, position);
                break;
            case 8:
                CreateRightUpCorner(world, position);
                break;
            case 54:
                CreateObjectC(world, position, blockType);
                break;
            case 60: //右向き
                lightpos.x += 0.0f;
                CreateWallLight(world, position, lightangle, lightpos);
                CreateWall(world, position);
                break;
            case 61: //上向き
                lightpos.z += 0.0f;
                lightangle = 270.0f;
                CreateWallLight(world, position, lightangle, lightpos);
                CreateWall(world, position);
                break;
            case 62: //左向き
                lightpos.x += 0.0f;
                lightangle = 180.0f;
                CreateWallLight(world, position, lightangle, lightpos);
                CreateWall(world, position);
                break;
            case 63: //下向き
                lightpos.z += 0.0f;
                lightangle = 90.0f;
                CreateWallLight(world, position, lightangle, lightpos);
                CreateWall(world, position);
                break;
            case 64:
                CreateGoalSwitch(world, position, blockType);
            default:
                if (blockType >= 10 && blockType < 20) {
                    CreateMovingObstacle(world, position, blockType);
                } else if (blockType >= 30 && blockType < 40) {
                    CreateDashBoard(world, position, blockType);
                } else if (blockType == 50 || blockType == 51) {
                    CreateObjectA(world, position, blockType);
                } else if (blockType == 52 || blockType == 53) {
                    CreateObjectB(world, position, blockType);
                }
                break;
        }
    }

    void CreateFloor(World &world, const DirectX::XMFLOAT3 &position) {
        // 各マスにフロアFBXをそのまま配置する（スケールは1x1x1、Yは設定値でオフセット）
        DirectX::XMFLOAT3 floorPos = {position.x, position.y + cfg_FloorYOffset, position.z};
        Transform transform{{floorPos}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};

        Entity floor = world.Create()
                           .With<Transform>(transform)
                           .With<Model>(cfg_FloorFBXPass)
                           .With<StageElementTag>()
                           .With<StaticCollider>()
                           .Build();

        // ステージ切り替え時に破棄されるよう、ステージ所有リストへ登録
        stageOwnedEntities_.push_back(floor);
    }

    void CreateStart(World &world, const DirectX::XMFLOAT3 &position) {
        if (world.IsAlive(startEntity_))
            return;

        // 2x2マスの中心に合わせる (左上マス中心から X+0.5, Z-0.5)
        DirectX::XMFLOAT3 diffPosition = {position.x + 0.5f, position.y - 1.5f, position.z - 0.5f};

        Transform t{diffPosition, {0, 0, 0}, {2.0f, 0.5f, 2.0f}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_StartR.Get(), cfg_StartG.Get(), cfg_StartB.Get()};

        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_StartEmissiveR.Get(), cfg_StartEmissiveG.Get(), cfg_StartEmissiveB.Get()},
            cfg_StartEmissiveIntensity.Get()};
        EmissivePulse pulse{cfg_StartPulseMin.Get(), cfg_StartPulseMax.Get(), cfg_StartPulseSpeed.Get()};
        PointLight light{
            DirectX::XMFLOAT3{cfg_StartEmissiveR.Get(), cfg_StartEmissiveG.Get(), cfg_StartEmissiveB.Get()},
            cfg_StartEmissiveIntensity.Get(),
            cfg_StartLightRange.Get()};
        ApplyDefaultPointLightParams(light);

        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<StartTag>()
                       .With<StageElementTag>()
                       .With<CollisionBox>(DirectX::XMFLOAT3{2.0f, 4.0f, 2.0f})
                       .With<StaticCollider>()
                       .Build();

        startEntity_ = e;
        stageOwnedEntities_.push_back(e);
    }

    void CreateGoal(World &world, const DirectX::XMFLOAT3 &position, int currentstage) {
        if (world.IsAlive(goalEntity_))
            return;

        int stageIndex = currentstage - 1;
        if (stageIndex < 0)
            stageIndex = 0;

        // 現在のルーム番号を取得（1-based -> 0-based）
        int roomIndex = 0;
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            roomIndex = sp.currentRoom - 1;
        });
        if (roomIndex < 0)
            roomIndex = 0;

        float angle = 0.0f;
        world.ForEach<LoadGoalAngle>([&](Entity, LoadGoalAngle &data) {
            if (stageIndex < static_cast<int>(data.goalAngle.size())) {
                const auto &row = data.goalAngle[stageIndex];
                if (roomIndex < static_cast<int>(row.size())) {
                    angle = static_cast<float>(row[roomIndex]);
                } else if (!row.empty()) {
                    angle = static_cast<float>(row[0]);
                }
            } else if (!data.goalAngle.empty() && !data.goalAngle[0].empty()) {
                angle = static_cast<float>(data.goalAngle[0][0]);
            }
        });

        // 2x2マスの中心に合わせる (左上マス中心から X+0.5, Z-0.5)
        DirectX::XMFLOAT3 diffPosition = {position.x + 0.5f, position.y - 1.5f, position.z - 0.5f};

        Transform t{diffPosition, {0, angle, 0}, {2.0f, 0.5f, 2.0f}};

        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = DirectX::XMFLOAT3{cfg_GoalR.Get(), cfg_GoalG.Get(), cfg_GoalB.Get()};

        EmissiveMaterial emissive{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR.Get(), cfg_GoalEmissiveG.Get(), cfg_GoalEmissiveB.Get()},
            cfg_GoalEmissiveIntensity.Get()};
        EmissivePulse pulse{cfg_GoalPulseMin.Get(), cfg_GoalPulseMax.Get(), cfg_GoalPulseSpeed.Get()};
        PointLight light{
            DirectX::XMFLOAT3{cfg_GoalEmissiveR.Get(), cfg_GoalEmissiveG.Get(), cfg_GoalEmissiveB.Get()},
            cfg_GoalEmissiveIntensity.Get(),
            cfg_GoalLightRange.Get()};
        ApplyDefaultPointLightParams(light);
        light.offset = {0.0f, std::max(0.0f, cfg_PointLightYOffset.Get()), 0.0f};

        StopGoalEffect();

        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<EmissiveMaterial>(emissive)
                       .With<EmissivePulse>(pulse)
                       .With<PointLight>(light)
                       .With<GoalTag>()
                       .With<StageElementTag>()
                       .With<CollisionBox>(DirectX::XMFLOAT3{2.0f, 4.0f, 2.0f})
                       .With<GoalCollisionHandler>()
                       .With<StaticCollider>()
                       .Build();

        goalEntity_ = e;
        stageOwnedEntities_.push_back(e);
        auto goalHandle = EffekseerManager::GetInstance().PlayEffectSafe("Goal", diffPosition, {1.0f, 1.0f, 1.0f}, true);
        goalEffectHandle_ = goalHandle.value_or(-1);
    }

    void CreateGoalSwitch(World &world, const DirectX::XMFLOAT3 &position, int currentstage) {
        Transform t{{position.x, position.y - 1.0f, position.z}, {0, 0, 0}, {1.0f, 0.5f, 1.0f}};
        MeshRenderer r;
        r.meshType = MeshType::Cube;
        r.color = {1.0f, 0.0f, 0.0f};

        Entity e = world.Create()
                       .With<Transform>(t)
                       .With<MeshRenderer>(r)
                       .With<CollisionBox>(DirectX::XMFLOAT3{1.5f, 2.0f, 1.5f})
                       .With<SwitchCollisionHandler>()
                       .With<StaticCollider>()
                       .With<StageElementTag>()
                       .With<SwitchTag>()
                       .Build();

        stageOwnedEntities_.push_back(e);
    }

    void StopGoalEffect() {
        auto &efk = EffekseerManager::GetInstance();
        if (goalEffectHandle_ != -1) {
            efk.StopEffectHandle(goalEffectHandle_);
            goalEffectHandle_ = -1;
        }
        efk.StopEffect("Goal");
    }

    void UpdateGoalEffectTransform(World &world) {
        if (goalEffectHandle_ == -1)
            return;
        if (!world.IsAlive(goalEntity_))
            return;
        if (auto *t = world.TryGet<Transform>(goalEntity_)) {
            auto &efk = EffekseerManager::GetInstance();
            efk.SetEffectPosition(goalEffectHandle_, t->position);
            efk.SetEffectRotation(goalEffectHandle_, t->rotation);
            DirectX::XMFLOAT3 scaled = {t->scale.x * 0.5f, t->scale.y * 0.5f, t->scale.z * 0.5f};
            efk.SetEffectScale(goalEffectHandle_, scaled);
        }
    }

    struct GoalCollisionHandler {
        void OnHit(World &world, Entity self, Entity other) {
            if (!world.Has<PlayerTag>(other))
                return;

        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            if (sp.clearedThisStage)
                return;
            sp.clearedThisStage = true;
            StageSave::MarkStageCleared(sp.currentStage);
        });
        }
    };

    void CreateGoalDoor(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 a = {};
    }

    void CreateWall(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        Transform transform{diffPosition, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_WallFBXPass)
                                .With<StageElementTag>()
                                .With<WallTag>()
                                .With<CollisionBox>(
                                    DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f},
                                    DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                .With<WallCollisionHandler>()
                                .With<StaticCollider>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateWallLight(World &world, const DirectX::XMFLOAT3 &position, float angle, DirectX::XMFLOAT3 subpos) {
        DirectX::XMFLOAT3 diffPosition = {position.x + (subpos.x), position.y + (subpos.y), position.z + (subpos.z)};
        Transform transform{diffPosition, {0.0f, angle, 0.0f}, {1.0f, 1.0f, 1.0f}};

        PointLight wallLight;
        wallLight.color = {1.0f, 0.7f, 0.5f};
        ApplyDefaultPointLightParams(wallLight);
        wallLight.range = 1.0f;
        wallLight.intensity = 1.0f;
        wallLight.constantAttenuation = 0.1f;
        wallLight.offset = {0.0f, std::max(0.0f, cfg_PointLightYOffset.Get()), 0.0f};

        Entity walllightEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<Model>(cfg_WallLightFBXPass)
                                     .With<StageElementTag>()
                                     .With<PointLight>(wallLight)
                                     .With<WallLightTag>()
                                     .Build();

        stageOwnedEntities_.push_back(walllightEntity);
    }

    void CreateRightDownCorner(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        // メッシュ描画用Transform
        Transform transform{diffPosition, {0.0f, 270.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        // メッシュ描画用Entity
        Entity meshEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_HalfWallFBXPass)
                                .With<StageElementTag>()
                                .Build();
        stageOwnedEntities_.push_back(meshEntity);

        // コライダー用Transform（斜辺に沿って45°回転）
        Transform colliderTransform{diffPosition, {0.0f, 315.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        Entity wallEntity = world.Create()
                                .With<Transform>(colliderTransform)
                                .With<WallTag>()
                                .With<CollisionBox>(
                                    DirectX::XMFLOAT3{1.414f, 2.0f, 0.1f},
                                    DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                .With<WallCollisionHandler>()
                                .With<StaticCollider>()
                                .Build();
        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateLeftDownCorner(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        // メッシュ描画用Transform
        Transform transform{diffPosition, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        // メッシュ描画用Entity
        Entity meshEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_HalfWallFBXPass)
                                .With<StageElementTag>()
                                .Build();
        stageOwnedEntities_.push_back(meshEntity);

        // コライダー用Transform（斜辺に沿って45°回転）
        Transform colliderTransform{diffPosition, {0.0f, 45.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        Entity wallEntity = world.Create()
                                .With<Transform>(colliderTransform)
                                .With<WallTag>()
                                .With<CollisionBox>(
                                    DirectX::XMFLOAT3{1.414f, 2.0f, 0.1f},
                                    DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                .With<WallCollisionHandler>()
                                .With<StaticCollider>()
                                .Build();
        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateLeftUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        // メッシュ描画用Transform
        Transform transform{diffPosition, {0.0f, 90.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        // メッシュ描画用Entity
        Entity meshEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_HalfWallFBXPass)
                                .With<StageElementTag>()
                                .Build();
        stageOwnedEntities_.push_back(meshEntity);

        // コライダー用Transform（斜辺に沿って45°回転）
        Transform colliderTransform{diffPosition, {0.0f, 135.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        Entity wallEntity = world.Create()
                                .With<Transform>(colliderTransform)
                                .With<WallTag>()
                                .With<CollisionBox>(
                                    DirectX::XMFLOAT3{1.414f, 2.0f, 0.1f},
                                    DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                .With<WallCollisionHandler>()
                                .With<StaticCollider>()
                                .Build();
        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateRightUpCorner(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        // メッシュ描画用Transform
        Transform transform{diffPosition, {0.0f, 180.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        // メッシュ描画用Entity
        Entity meshEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_HalfWallFBXPass)
                                .With<StageElementTag>()
                                .Build();
        stageOwnedEntities_.push_back(meshEntity);

        // コライダー用Transform（斜辺に沿って45°回転）
        Transform colliderTransform{diffPosition, {0.0f, 225.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        Entity wallEntity = world.Create()
                                .With<Transform>(colliderTransform)
                                .With<WallTag>()
                                .With<CollisionBox>(
                                    DirectX::XMFLOAT3{1.414f, 2.0f, 0.1f},
                                    DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                .With<WallCollisionHandler>()
                                .With<StaticCollider>()
                                .Build();
        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateMovingObstacle(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y + cfg_FloorYOffset + 1.0f, position.z};
        MovingObstacle obstacle;
        obstacle.startPos = diffPosition;
        obstacle.baseScale = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};

        auto resolvePatternIndex = [](int type) -> std::optional<int> {
            if (type >= 10 && type < 20)
                return type - 10; // CSVのインデックス+10がID
            return std::nullopt;
        };

        const auto patternIndex = resolvePatternIndex(blockType);
        world.ForEach<LoadMovingObstacle>([&](Entity, LoadMovingObstacle &data) {
            if (!patternIndex) {
                DEBUGLOG_WARNING("[MoveObstacle] 未対応のブロックIDです: " + std::to_string(blockType));
                return;
            }

            if (*patternIndex >= 0 && *patternIndex < static_cast<int>(data.patterns.size())) {
                const auto &p = data.patterns[*patternIndex];
                obstacle.delta = DirectX::XMFLOAT3{p.dirX, 0.0f, -p.dirY}; // ステージ座標のYはワールドZと逆向き
                obstacle.endPos = DirectX::XMFLOAT3{
                    diffPosition.x + obstacle.delta.x,
                    diffPosition.y + obstacle.delta.y,
                    diffPosition.z + obstacle.delta.z};
                obstacle.waitAtStart = p.waitAtStart;
                obstacle.waitAtEnd = p.waitAtEnd;
                obstacle.travelTime = p.travelTime;
            } else {
                DEBUGLOG_WARNING("[MoveObstacle] パターンが見つかりません index=" + std::to_string(*patternIndex));
            }
        });

        Transform transform{diffPosition, {0.0f, 0.0f, 0.0f}, obstacle.baseScale};

        Entity entity = world.Create()
                            .With<Transform>(transform)
                            .With<Model>(cfg_MovingObstacleFBXPass)
                            .With<StageElementTag>()
                            .With<WallTag>()
                            .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                            .With<WallCollisionHandler>()
                            .With<MovingObstacle>(obstacle)
                            .Build();

        stageOwnedEntities_.push_back(entity);
    }

    void CreateObjectA(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity ObjectAEntity = world.Create()
                                   .With<Transform>(transform)
                                   .With<Model>(cfg_AObstacleFBXPass)
                                   .With<MeshRenderer>(renderer)
                                   .With<StageElementTag>()
                                   .With<WallTag>()
                                   .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                   .With<FloorWallCollisionHandler>()
                                   .With<StaticCollider>()
                                   .Build();

        stageOwnedEntities_.push_back(ObjectAEntity);
    }

    void CreateObjectB(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity ObjectAEntity = world.Create()
                                   .With<Transform>(transform)
                                   .With<Model>(cfg_BObstacleFBXPass)
                                   .With<MeshRenderer>(renderer)
                                   .With<StageElementTag>()
                                   .With<WallTag>()
                                   .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                   .With<FloorWallCollisionHandler>()
                                   .With<StaticCollider>()
                                   .Build();

        stageOwnedEntities_.push_back(ObjectAEntity);
    }

    void CreateObjectC(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity ObjectAEntity = world.Create()
                                   .With<Transform>(transform)
                                   .With<Model>(cfg_CObstacleFBXPass)
                                   .With<MeshRenderer>(renderer)
                                   .With<StageElementTag>()
                                   .With<WallTag>()
                                   .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                   .With<FloorWallCollisionHandler>()
                                   .With<StaticCollider>()
                                   .Build();

        stageOwnedEntities_.push_back(ObjectAEntity);
    }

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - WALL_MESH_Y_OFFSET, position.z};
        Transform transform{diffPosition, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        Entity worldwallEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<Model>(cfg_WallFBXPass)
                                     .With<WallTag>()
                                     .With<StageElementTag>()
                                     .With<CollisionBox>(
                                         DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f},
                                         DirectX::XMFLOAT3{0.0f, WALL_COLLISION_CENTER_OFFSET, 0.0f})
                                     .With<FloorWallCollisionHandler>()
                                     .With<StaticCollider>()
                                     .Build();

        stageOwnedEntities_.push_back(worldwallEntity);
    }

    void CreateDashBoard(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        DashBoardStatus status;
        status.blockID = blockType;
        float csvAngleDeg = 0.0f;

        world.ForEach<LoadAngle>([&](Entity, LoadAngle &loadAngle) {
            const int angleIndex = blockType - 30;
            if (angleIndex >= 0 && angleIndex < static_cast<int>(loadAngle.stageAngle.size())) {
                const auto &row = loadAngle.stageAngle[angleIndex];
                if (!row.empty()) {
                    csvAngleDeg = static_cast<float>(row[0]);
                }
            }
        });

        // 角度を正規化（-360～360 -> 0～360）し、ゲームロジック用の加速度角度はCSV仕様そのままを採用
        while (csvAngleDeg < 0.0f)
            csvAngleDeg += 360.0f;
        while (csvAngleDeg >= 360.0f)
            csvAngleDeg -= 360.0f;

        // 見た目補正: FBXのデフォルト向きが+90度ずれているため、モデルの回転のみ+90度補正
        const float visualYawDeg = -csvAngleDeg + 90.0f; // 180度反転で見た目を加速方向に合わせる

        DirectX::XMFLOAT3 adjustedPos = position;
        adjustedPos.y -= 1.5f;
        Transform transform{{adjustedPos}, {0.0f, visualYawDeg, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f};

        // プレイヤーへの影響角度はCSVそのまま（見た目補正は加えない）
        status.accelAngle = csvAngleDeg;

        //加速板のエフェクト常時出力
        EffekseerManager::GetInstance().PlayEffectSafe("DashBoard", transform.position, {1.0f, 1.0f, 1.0f}, true);

        Entity dashBoardEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<Model>(cfg_DashBoardFBXPass)
                                     .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                     .With<GimmickTag>()
                                     .With<StageElementTag>()
                                     .With<DashBordCollisionHandler>()
                                     .With<DashBoardStatus>(status)
                                     .With<StaticCollider>()
                                     .Build();

        stageOwnedEntities_.push_back(dashBoardEntity);
    }

    void BakeStageLighting(World & /*world*/) {
        // Deprecated placeholder（現状は CreateStageMap 内で BakeStageLights を実行）
    }

    void CollectDescendants(World &world, Entity parent, std::vector<Entity> &outList) {
        auto *h = world.TryGet<TransformHierarchy>(parent);
        if (!h)
            return;

        for (auto child : h->GetChildren()) {
            if (world.IsAlive(child)) {
                outList.push_back(child);
                CollectDescendants(world, child, outList);
            }
        }
    }

    void SetupStage(World &world, int stage) {
        DEBUGLOG("SetupStage: destroying stageOwnedEntities_ count=" + std::to_string(stageOwnedEntities_.size()));
        // 既存のステージ所有エンティティを破棄
        for (const auto &entity : stageOwnedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::StageReset);
            }
        }
        stageOwnedEntities_.clear();
        StopGoalEffect();

        // StageElementTag を使ってステージ固有エンティティを一網打尽にする（子孫も含む）
        std::vector<Entity> toDestroy;
        world.ForEach<StageElementTag>([&](Entity e, StageElementTag &) {
            toDestroy.push_back(e);
            CollectDescendants(world, e, toDestroy);
        });
        DEBUGLOG("SetupStage: StageElementTag candidates=" + std::to_string(toDestroy.size()));
        for (auto e : toDestroy) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::StageReset);
            }
        }

        // 破棄を即時反映（次のステージ生成と重ならないように）
        world.FlushDestroyEndOfFrame();
        DEBUGLOG("SetupStage: after pre-stage flush alive=" + std::to_string(world.GetAliveCount()));
        DiagnoseStageLeak(world, toDestroy.size());

        startEntity_ = {};
        goalEntity_ = {};
        goalEffectHandle_ = -1;

        stageClearActive_ = false;
        stageClearTimer_ = 0.0f;

        world.ForEach<StageProgress>([](Entity, StageProgress &sp) {
            sp.requestAdvance = false;
            sp.goalTransitioning = false;
            sp.clearedThisStage = false;
            sp.pressedSwitch = false;
            sp.goalUnlocked = false;
            DEBUGLOG("[SetupStage] StageProgress reset: clearedThisStage=" + std::to_string(sp.clearedThisStage) +
                     " goalTransitioning=" + std::to_string(sp.goalTransitioning) +
                     " pressedSwitch=" + std::to_string(sp.pressedSwitch));
        });

        // エフェクトを全停止（前のステージのエフェクトが残らないように）
        EffekseerManager::GetInstance().StopEffect();

        // プレイヤーからGoalAttractorを削除（前のステージの状態が残らないように）
        if (world.IsAlive(playerEntity_)) {
            if (world.Has<GoalAttractor>(playerEntity_)) {
                world.Remove<GoalAttractor>(playerEntity_);
                DEBUGLOG("[SetupStage] GoalAttractor removed from player");
            }
        }

        // 最新のStageCreateのみを利用し、それ以外は破棄して重複生成を防ぐ
        Entity activeStageCreate{};
        StageCreate *activeStagePtr = nullptr;
        std::vector<Entity> duplicateStageCreates;
        world.ForEach<StageCreate>([&](Entity e, StageCreate &sc) {
            activeStageCreate = e;
            activeStagePtr = &sc;
            // ループ終端で最後に見つかったものを採用。途中で見つかったものは後で破棄。
            duplicateStageCreates.push_back(e);
        });
        DEBUGLOG("SetupStage: StageCreate candidates=" + std::to_string(duplicateStageCreates.size()));
        if (!duplicateStageCreates.empty()) {
            // keep last
            duplicateStageCreates.pop_back();
        }
        for (auto e : duplicateStageCreates) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::StageReset);
            }
        }
        if (!activeStagePtr) {
            DEBUGLOG_ERROR("[StageCreate] 有効な StageCreate が存在しません。ステージ生成をスキップします");
            return;
        }
        world.FlushDestroyEndOfFrame();
        DEBUGLOG("SetupStage: stage cleanup complete, alive=" + std::to_string(world.GetAliveCount()));

        // ステージに紐づく加速角度CSVをロードしてLoadAngleコンポーネントに反映
        {
            auto angleCsvPath = ResolveSpeedUpCsvPath(world, activeStagePtr->csvPath);
            std::vector<std::vector<int>> angles;
            if (angleCsvPath) {
                angles = LoadAngleCsv(*angleCsvPath);
                if (angles.empty()) {
                    DEBUGLOG("[SpeedUp] 角度CSVが空、または読み込みに失敗しました(仕様によりスキップ): " + *angleCsvPath);
                }
            } else {
                DEBUGLOG("[SpeedUp] 角度CSVパスを解決できません(仕様によりスキップ): " + activeStagePtr->csvPath);
            }

            bool updated = false;
            world.ForEach<LoadAngle>([&](Entity, LoadAngle &loadAngle) {
                loadAngle.stageAngle = angles;
                updated = true;
            });

            if (!updated) {
                LoadAngle loadAngle;
                loadAngle.stageAngle = angles;
                Entity angleEntity = world.Create()
                                         .With<LoadAngle>(loadAngle)
                                         .With<StageElementTag>()
                                         .Build();
                stageOwnedEntities_.push_back(angleEntity);
            }

            // 動く障害物CSVもステージごとにロード
            auto moveCsvPath = ResolveMovingObstacleCsvPath(world, activeStagePtr->csvPath);
            std::vector<MovingObstaclePattern> movePatterns;
            if (moveCsvPath) {
                movePatterns = LoadMovingObstacleCsv(*moveCsvPath);
                if (movePatterns.empty()) {
                    DEBUGLOG("[MoveObstacle] CSVが空、または読み込みに失敗しました(仕様によりスキップ): " + *moveCsvPath);
                }
            } else {
                DEBUGLOG("[MoveObstacle] CSVパスを解決できません(仕様によりスキップ): " + activeStagePtr->csvPath);
            }

            bool moveUpdated = false;
            world.ForEach<LoadMovingObstacle>([&](Entity, LoadMovingObstacle &loadMove) {
                loadMove.patterns = movePatterns;
                moveUpdated = true;
            });

            if (!moveUpdated) {
                LoadMovingObstacle loadMove;
                loadMove.patterns = movePatterns;
                Entity moveEntity = world.Create()
                                        .With<LoadMovingObstacle>(loadMove)
                                        .With<StageElementTag>()
                                        .Build();
                stageOwnedEntities_.push_back(moveEntity);
            }

            auto timeCsvPath = ResolveLimitTimePath(world, activeStagePtr->csvPath);
            std::vector<vector<int>> stagetime;
            float roomtime = 0.0f;

            if (timeCsvPath) {
                stagetime = LoadTimeCsv(*timeCsvPath);

                if (stagetime.empty()) {
                    DEBUGLOG("[Time] 制限時間CSVが空、または読み込みに失敗しました(仕様によりスキップ): " + *timeCsvPath);
                }
            } else {
                DEBUGLOG("[Time] 制限時間CSVパスを解決できません(仕様によりスキップ): " + activeStagePtr->csvPath);
            }
            
            world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
                cfg_LimitTime = static_cast<float>(stagetime[0][sp.currentRoom - 1]);
                world.ForEach<GameStatus>([&](Entity, GameStatus &gs) {
                    gs.elapsedTime = cfg_LimitTime;
                });
                });
        }

        CreateStageMap(world, *activeStagePtr, stage);
        BakeStageLighting(world);

        if (world.IsAlive(playerEntity_)) {
            ResetPlayerToStart(world, playerEntity_);
        }
    }

    void ClearGoalTransitionFlag(World &world) {
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            sp.goalTransitioning = false;
        });
    }

    void DiagnoseStageLeak(World &world, size_t stageElementCount) {
        const size_t totalAlive = world.GetAliveCount();
        if (totalAlive <= stageElementCount + 200) {
            return;
        }
        size_t sampleCount = 0;
        size_t untrackedMeshes = 0;
        std::vector<uint32_t> sampleIds;
        world.ForEach<MeshRenderer>([&](Entity e, MeshRenderer &) {
            if (world.Has<StageElementTag>(e))
                return;
            if (world.Has<PlayerTag>(e))
                return;
            if (world.Has<UIText>(e))
                return;
            if (world.Has<GoalAttractor>(e))
                return;
            ++untrackedMeshes;
            if (sampleCount < 5) {
                sampleIds.push_back(e.id);
                std::string comps = "Components: MeshRenderer ";
                if (world.Has<Model>(e))
                    comps += "Model ";
                if (world.Has<PointLight>(e))
                    comps += "PointLight ";
                if (world.Has<CollisionBox>(e))
                    comps += "ColBox ";
                if (world.Has<UIImage>(e))
                    comps += "UIImage ";
                if (world.Has<Animator>(e))
                    comps += "Animator ";
                if (auto *t = world.TryGet<Transform>(e)) {
                    comps += "Pos(" + std::to_string(t->position.x) + "," + std::to_string(t->position.y) + ") ";
                }
                DEBUGLOG("Leak Candidate ID=" + std::to_string(e.id) + ": " + comps);
                ++sampleCount;
            }
        });
        if (untrackedMeshes > 0) {
            std::stringstream ss;
            ss << "Stage leak warning: totalAlive=" << totalAlive
               << ", stageElements=" << stageElementCount
               << ", untrackedMeshRenderer=" << untrackedMeshes;
            if (!sampleIds.empty()) {
                ss << ", sampleIDs=" << sampleIds[0];
                for (size_t i = 1; i < sampleIds.size(); ++i) {
                    ss << "," << sampleIds[i];
                }
            }
            DEBUGLOG(ss.str());
        }
    }

    static const char *WallHitStateToString(PlayerStatus::WallHitState state) {
        switch (state) {
            case PlayerStatus::WallHitState::Idle:
                return "Idle";
            case PlayerStatus::WallHitState::Shaking:
                return "Shaking";
            case PlayerStatus::WallHitState::RespawnWait:
                return "RespawnWait";
            default:
                return "Unknown";
        }
    }

    void UpdateWallHitState(PlayerStatus &status, PlayerStatus::WallHitState newState, const char *reason) {
        if (status.wallHitState == newState)
            return;
        status.wallHitState = newState;
        DEBUGLOG(std::string("WallHitState -> ") + WallHitStateToString(newState) + " (" + reason + ")");
    }

    // =========================================
    // 遅延リスポーン・カメラリアクション更新
    // =========================================

    void UpdateDelayedRespawn(float dt, World &world) {

        if (auto *movement = world.TryGet<PlayerMovement>(playerEntity_)) {
            movement->isCharging_ = false;
        }
        if (!pendingRespawn_)
            return;
        // グローバルにも反映して他コンポーネントから参照可能に
        g_respawnPending = true;
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            ResetPlayerToStart(world, respawnPlayer_, true);
            StartFadeInNormal(world);
            deathFadeVisible_ = false;
            if (auto *playerStatus = world.TryGet<PlayerStatus>(playerEntity_)) {
                UpdateWallHitState(*playerStatus, PlayerStatus::WallHitState::Idle, "RespawnComplete");
            }
            pendingRespawn_ = false;
            g_respawnPending = false;
            respawnTimer_ = 0.0f;
        }
        if (auto *playerStatus = world.TryGet<PlayerStatus>(playerEntity_)) {
            if (playerStatus->wallHitState == PlayerStatus::WallHitState::Shaking) {
                UpdateWallHitState(*playerStatus, PlayerStatus::WallHitState::RespawnWait, "RespawnPending");
            }
        }
    }

    static DirectX::XMFLOAT3 SmoothStickTarget(
        const DirectX::XMFLOAT3 &current,
        const DirectX::XMFLOAT3 &desired,
        float dt,
        float response,
        float maxSpeed,
        float snapDistance = 1e-3f) {
        const float safeDt = std::max(0.0f, dt);
        const float safeResponse = std::max(0.0f, response);
        const float safeSpeed = std::max(0.0f, maxSpeed);
        const float lerpFactor = 1.0f - std::exp(-safeResponse * safeDt);

        DirectX::XMFLOAT3 delta{
            desired.x - current.x,
            desired.y - current.y,
            desired.z - current.z};
        DirectX::XMFLOAT3 step{
            delta.x * lerpFactor,
            delta.y * lerpFactor,
            delta.z * lerpFactor};

        const float stepLen = std::sqrt(step.x * step.x + step.y * step.y + step.z * step.z);
        const float maxStep = safeSpeed * safeDt;
        if (stepLen > maxStep && stepLen > 1e-6f && maxStep > 0.0f) {
            const float scale = maxStep / stepLen;
            step.x *= scale;
            step.y *= scale;
            step.z *= scale;
        }

        DirectX::XMFLOAT3 next{
            current.x + step.x,
            current.y + step.y,
            current.z + step.z};

        const float remainingX = desired.x - next.x;
        const float remainingY = desired.y - next.y;
        const float remainingZ = desired.z - next.z;
        const float remainingLen = std::sqrt(remainingX * remainingX + remainingY * remainingY + remainingZ * remainingZ);
        if (remainingLen <= snapDistance)
            return desired;

        return next;
    }

    static bool IsSkyboxTexturePathValid(const std::string &path) {
        return !path.empty();
    }

    static float SanitizeSkyboxScale(float scale) {
        return std::max(scale, 0.1f);
    }

#if defined(_DEBUG)
    void RunStickZoomSmoothingTests() {
        {
            const DirectX::XMFLOAT3 current{0.0f, 0.0f, 0.0f};
            const DirectX::XMFLOAT3 desired{10.0f, 0.0f, 0.0f};
            const DirectX::XMFLOAT3 next = SmoothStickTarget(current, desired, 1.0f, 3.0f, 1.0f);
            const float stepLen = std::sqrt(
                (next.x - current.x) * (next.x - current.x) +
                (next.y - current.y) * (next.y - current.y) +
                (next.z - current.z) * (next.z - current.z));
            assert(stepLen <= 1.0f + 1e-4f);
        }
        {
            const DirectX::XMFLOAT3 current{0.0f, 0.0f, 0.0f};
            const DirectX::XMFLOAT3 desired{1.0f, 1.0f, 0.0f};
            const DirectX::XMFLOAT3 next = SmoothStickTarget(current, desired, 0.25f, 3.0f, 10.0f);
            assert(next.x > 0.0f && next.y > 0.0f);
            assert(next.x < desired.x && next.y < desired.y);
        }
        {
            const DirectX::XMFLOAT3 current{0.5f, -0.5f, 0.0f};
            const DirectX::XMFLOAT3 desired{0.5f, -0.5f, 0.0f};
            const DirectX::XMFLOAT3 next = SmoothStickTarget(current, desired, 0.16f, 3.0f, 2.0f);
            assert(std::abs(next.x - current.x) < 1e-6f);
            assert(std::abs(next.y - current.y) < 1e-6f);
        }
    }

    void RunSkyboxScaleTests() {
        assert(!IsSkyboxTexturePathValid(""));
        assert(IsSkyboxTexturePathValid("Assets/Textures/Skybox/Sky_Box.png"));
        assert(SanitizeSkyboxScale(1.0f) == 1.0f);
        assert(SanitizeSkyboxScale(0.0f) == 0.1f);
        assert(SanitizeSkyboxScale(-2.0f) == 0.1f);
        assert(std::abs(SkyboxRotationToDegrees(DirectX::XM_PI) - 180.0f) < 1e-3f);
        Camera testCam{};
        testCam.position = {0.0f, 0.0f, 0.0f};
        testCam.target = {1.0f, 0.0f, 0.0f};
        assert(std::abs(BuildSkyboxYawDeg(testCam, DirectX::XM_PIDIV2) - 180.0f) < 1e-3f);
    }

    void RunPointLightOffsetTests() {
        PointLight light;
        light.offset = {0.0f, 1.0f, 0.0f};
        DirectX::XMFLOAT3 base{1.0f, 2.0f, 3.0f};
        DirectX::XMFLOAT3 pos = ApplyPointLightOffset(base, light);
        assert(std::abs(pos.x - 1.0f) < 1e-6f);
        assert(std::abs(pos.y - 3.0f) < 1e-6f);
        assert(std::abs(pos.z - 3.0f) < 1e-6f);
    }
#endif

    void UpdateCameraReaction(float dt, World &world) {
#if defined(_DEBUG)
        static bool stickZoomTestsExecuted = false;
        if (!stickZoomTestsExecuted) {
            RunStickZoomSmoothingTests();
            stickZoomTestsExecuted = true;
        }
#endif
        DirectX::XMFLOAT3 posOffset{0.0f, 0.0f, 0.0f};
        DirectX::XMFLOAT3 targetOffset{0.0f, 0.0f, 0.0f};
        float fovDelta = 0.0f;
        DirectX::XMFLOAT3 upVec = baseUp_;

        if (shakeActive_)
            UpdateShake(dt, posOffset, targetOffset, upVec);
        if (impulseActive_)
            UpdateImpulse(dt, posOffset, targetOffset);
        if (zoomActive_)
            UpdateZoom(dt, fovDelta);
        float stickZoomDelta = UpdateStickZoom(dt);

        // スティックズーム時のターゲット補間（ズーム追応と同じレートで滑らかに寄せる）
        DirectX::XMFLOAT3 effectiveTarget = baseTarget_;
        // 計算のための現在の補間注視点を保持する（worldが生きている間は更新）
        if (world.IsAlive(playerEntity_)) {
            if (auto *pTransform = world.TryGet<Transform>(playerEntity_)) {
                // 目標寄り率を計算（ズーム量 * TargetRatio）
                float zoomRatio = std::abs(stickZoomCurrent_) / std::max(0.01f, std::abs(cfg_StickZoomAmount.Get()));
                zoomRatio = std::clamp(zoomRatio, 0.0f, 1.0f);
                float targetRatio = zoomRatio * cfg_StickZoomTargetRatio.Get();

                DirectX::XMFLOAT3 desiredTarget{
                    baseTarget_.x + (pTransform->position.x - baseTarget_.x) * targetRatio,
                    baseTarget_.y + (pTransform->position.y - baseTarget_.y) * targetRatio,
                    baseTarget_.z + (pTransform->position.z - baseTarget_.z) * targetRatio};

                const float response = cfg_StickZoomResponse.Get();
                const float moveSpeed = cfg_StickZoomTargetSpeed.Get(); // world units per second cap
                currentTarget_ = SmoothStickTarget(currentTarget_, desiredTarget, dt, response, moveSpeed, 1e-3f);
            } else {
                // プレイヤーがいない場合は基準注視点へ戻す
                DirectX::XMFLOAT3 desiredTarget = baseTarget_;
                const float response = cfg_StickZoomResponse.Get();
                const float moveSpeed = cfg_StickZoomTargetSpeed.Get();
                DirectX::XMFLOAT3 blendedTarget = desiredTarget;
                blendedTarget.y = currentTarget_.y + (desiredTarget.y - currentTarget_.y) * currentVerticalBlend_;
                currentTarget_ = SmoothStickTarget(currentTarget_, blendedTarget, dt, response, moveSpeed, 1e-3f);
                float verticalBlendSpeed = std::max(0.5f, cfg_StickZoomTargetSpeed.Get());
                currentVerticalBlend_ += verticalBlendSpeed * dt;
                if (currentVerticalBlend_ > 1.0f)
                    currentVerticalBlend_ = 1.0f;
            }
        } else {
            // プレイヤーがいない場合は基準注視点へ戻す
            DirectX::XMFLOAT3 desiredTarget = baseTarget_;
            const float response = cfg_StickZoomResponse.Get();
            const float moveSpeed = cfg_StickZoomTargetSpeed.Get();
            DirectX::XMFLOAT3 blendedTarget = desiredTarget;
            blendedTarget.y = currentTarget_.y + (desiredTarget.y - currentTarget_.y) * currentVerticalBlend_;
            currentTarget_ = SmoothStickTarget(currentTarget_, blendedTarget, dt, response, moveSpeed, 1e-3f);
            float verticalBlendSpeed = std::max(0.5f, cfg_StickZoomTargetSpeed.Get());
            currentVerticalBlend_ += verticalBlendSpeed * dt;
            if (currentVerticalBlend_ > 1.0f)
                currentVerticalBlend_ = 1.0f;
        }

        // Use the smoothed currentTarget_ as the effective target to apply
        effectiveTarget = currentTarget_;

        camera_.position = {cameraPosition_.x + posOffset.x, cameraPosition_.y + posOffset.y, cameraPosition_.z + posOffset.z};
        camera_.target = {effectiveTarget.x + targetOffset.x, effectiveTarget.y + targetOffset.y, effectiveTarget.z + targetOffset.z};
        camera_.fovY = std::clamp(baseFovY_ + fovDelta + stickZoomDelta, DirectX::XM_PIDIV4 * 0.25f, DirectX::XM_PIDIV2 * 1.5f);
        camera_.up = upVec;

        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    void CreateSkybox(World &world) {
        const std::string modelPath = cfg_GameSkyboxModelPath.Get();
        if (modelPath.empty()) {
            DEBUGLOG_ERROR("[GameScene] Skybox model path is empty");
            return;
        }
        const float scale = SanitizeSkyboxScale(cfg_GameSkyboxScale.Get());
        const float yawDeg = BuildSkyboxYawDeg(camera_, skyboxRotation_);
        Transform transform{camera_.position, {0.0f, yawDeg, 0.0f}, {scale, scale, scale}};

        skyboxEntity_ = world.Create()
                            .With<Transform>(transform)
                            .With<Model>(modelPath)
                            .Build();
        ownedEntities_.push_back(skyboxEntity_);
        skyboxTextureApplied_ = false;
    }

    void UpdateSkyboxTransform(World &world) {
        if (!world.IsAlive(skyboxEntity_)) {
            return;
        }
        if (auto *t = world.TryGet<Transform>(skyboxEntity_)) {
            const float scale = SanitizeSkyboxScale(cfg_GameSkyboxScale.Get());
            t->position = camera_.position;
            t->rotation = {0.0f, SkyboxRotationToDegrees(0.0f), 0.0f};
            t->scale = {scale, scale, scale};
        }
    }

    void UpdateSkyboxRotation(float dt) {
        (void)dt;
    }

    bool EnsureSkyboxTextureLoaded() {
        if (skyboxTexture_ != TextureManager::INVALID_TEXTURE) {
            return true;
        }
        const std::string texturePath = cfg_GameSkyboxTexturePath.Get();
        if (!IsSkyboxTexturePathValid(texturePath)) {
            DEBUGLOG_ERROR("[GameScene] Skybox texture path is empty");
            return false;
        }
        std::error_code ec;
        if (!std::filesystem::exists(texturePath, ec) || ec) {
            DEBUGLOG_ERROR("[GameScene] Skybox texture not found: " + texturePath);
            return false;
        }
        auto &texMgr = ServiceLocator::Get<TextureManager>();
        skyboxTexture_ = texMgr.LoadFromFile(texturePath.c_str());
        if (skyboxTexture_ == TextureManager::INVALID_TEXTURE) {
            DEBUGLOG_ERROR("[GameScene] Failed to load skybox texture: " + texturePath);
            return false;
        }
        return true;
    }

    bool ApplySkyboxTextureRecursive(World &world, Entity entity) {
        bool applied = false;
        if (auto *mr = world.TryGet<MeshRenderer>(entity)) {
            mr->texture = skyboxTexture_;
            mr->useLighting = 0.0f;
            applied = true;
        }
        if (auto *mc = world.TryGet<ModelComponent>(entity)) {
            mc->texture = skyboxTexture_;
            mc->useLighting = 0.0f;
            applied = true;
        }
        if (auto *hier = world.TryGet<TransformHierarchy>(entity)) {
            for (const auto &child : hier->GetChildren()) {
                if (world.IsAlive(child)) {
                    applied |= ApplySkyboxTextureRecursive(world, child);
                }
            }
        }
        return applied;
    }

    void UpdateSkyboxTexture(World &world) {
        if (skyboxTextureApplied_) {
            return;
        }
        if (!world.IsAlive(skyboxEntity_)) {
            return;
        }
        if (!EnsureSkyboxTextureLoaded()) {
            return;
        }
        if (ApplySkyboxTextureRecursive(world, skyboxEntity_)) {
            skyboxTextureApplied_ = true;
        }
    }

    void UpdateChargeOverlay(World &world, float dt) {
        if (!world.IsAlive(chargeOverlayEntity_))
            return;
        const float lerpRate = 1.0f - std::exp(-6.0f * std::max(0.0f, dt));
        chargeOverlayCurrent_ = chargeOverlayCurrent_ + (chargeOverlayTarget_ - chargeOverlayCurrent_) * lerpRate;
        chargeOverlayCurrent_ = std::clamp(chargeOverlayCurrent_, 0.0f, 1.0f);
        if (auto *img = world.TryGet<UIImage>(chargeOverlayEntity_)) {
            img->opacity = chargeOverlayVisible_ ? chargeOverlayCurrent_ : 0.0f;
        }
        if (chargeOverlayVisible_ && chargeOverlayCurrent_ <= 0.01f && chargeOverlayTarget_ <= 0.01f) {
            chargeOverlayVisible_ = false;
        }
    }

    void UpdateShake(float dt, DirectX::XMFLOAT3 &posOffset, DirectX::XMFLOAT3 &targetOffset, DirectX::XMFLOAT3 &upVec) {
        shakeElapsed_ += dt;

        if (shakeElapsed_ >= shakeTime_) {
            shakeActive_ = false;
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

        // 平行移動ではなく微小回転で画面を揺らす
        // ベースのforwardを cameraPosition -> baseTarget の方向で計算（正規化）
        DirectX::XMVECTOR camPosV = DirectX::XMLoadFloat3(&cameraPosition_);
        DirectX::XMVECTOR tgtV = DirectX::XMLoadFloat3(&baseTarget_);
        DirectX::XMVECTOR forwardVec = DirectX::XMVectorSubtract(tgtV, camPosV);
        float fwdLenSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(forwardVec));
        DirectX::XMFLOAT3 baseForward{};
        if (fwdLenSq > 1e-6f) {
            forwardVec = DirectX::XMVector3Normalize(forwardVec);
            DirectX::XMStoreFloat3(&baseForward, forwardVec);
        } else {
            baseForward = {0.0f, 0.0f, 1.0f};
            forwardVec = DirectX::XMLoadFloat3(&baseForward);
            forwardVec = DirectX::XMVector3Normalize(forwardVec);
        }

        DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(sy * 0.1f, sx * 0.1f, sz * 0.05f);
        DirectX::XMVECTOR rotatedForward = DirectX::XMVector3TransformNormal(forwardVec, rot);
        DirectX::XMVECTOR rotatedUp = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&baseUp_), rot);

        DirectX::XMFLOAT3 newForward{};
        DirectX::XMFLOAT3 newUp{};
        DirectX::XMStoreFloat3(&newForward, rotatedForward);
        DirectX::XMStoreFloat3(&newUp, rotatedUp);

        targetOffset = {newForward.x * 0.1f - baseForward.x,
                        newForward.y * 0.1f - baseForward.y,
                        newForward.z * 0.1f - baseForward.z};
        upVec = newUp;
        // 位置は固定し、違和感の少ない視線揺らぎのみ適用
        posOffset = {0.0f, 0.0f, 0.0f};
    }

    void UpdateImpulse(float dt, DirectX::XMFLOAT3 &posOffset, DirectX::XMFLOAT3 &targetOffset) {
        impulseElapsed_ += dt;

        if (impulseElapsed_ >= impulseTime_) {
            impulseActive_ = false;
            return;
        }

        float decay = cfg_CameraImpulseDecay.Get();
        DirectX::XMFLOAT3 dir = impulseDir_;

        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 1e-6f) {
            dir.x /= len;
            dir.y /= len;
            dir.z /= len;
        }

        float t = impulseElapsed_ / std::max(1e-6f, impulseTime_);
        // なめらかな押し出し: 前半で優しく押し出し、後半はゆるやかに減衰して基準位置へ戻す
        float easeOut = 1.0f - std::cos(std::clamp(t, 0.0f, 1.0f) * DirectX::XM_PIDIV2); // 0→1 のソフトカーブ
        float fall = std::exp(-decay * impulseElapsed_);
        float currentIntensity = impulseIntensity_ * easeOut * fall;

        const float dx = dir.x * currentIntensity;
        const float dy = dir.y * currentIntensity;
        const float dz = dir.z * currentIntensity;

        posOffset.x += dx;
        posOffset.y += dy;
        posOffset.z += dz;
        targetOffset = {dx, dy, dz};
    }

    void UpdateZoom(float dt, float &fovDelta) {
        zoomElapsed_ += dt;

        if (zoomElapsed_ >= zoomTime_) {
            zoomActive_ = false;
            return;
        }

        float t = zoomElapsed_ / std::max(1e-6f, zoomTime_);
        float zoomCurve = std::sin(t * DirectX::XM_PI);
        fovDelta = -zoomAmount_ * zoomCurve;
    }

    void SetStickZoomActive(bool active) {
        // ズームインにするため負の値にする (FOVを狭める)
        stickZoomTarget_ = active ? -std::abs(cfg_StickZoomAmount.Get()) : 0.0f;
    }

    float UpdateStickZoom(float dt) {
        const float response = std::max(0.0f, cfg_StickZoomResponse.Get());
        const float safeDt = std::max(dt, 0.0f);
        const float lerpFactor = 1.0f - std::exp(-response * safeDt);
        stickZoomCurrent_ += (stickZoomTarget_ - stickZoomCurrent_) * lerpFactor;
        if (std::abs(stickZoomCurrent_) <= 1e-4f && std::abs(stickZoomTarget_) <= 1e-4f) {
            stickZoomCurrent_ = 0.0f;
        }
        return stickZoomCurrent_;
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
    int goalEffectHandle_ = -1;
    Entity gimmickEntity_{};
    Entity skyboxEntity_{};
    Entity fadeAnimationEntity_{};
    Entity deathFadeAnimationEntity_{};
    float deathFadeTimer_ = 0.0f;
    float startFadeTimer_ = 0.0f;

    bool isDeathFadePending_ = false;
    bool isStartFadePending_ = true;
    Entity chargeOverlayEntity_{};
    // 追加: ステージクリア用テキストエンティティと状態
    Entity stageClearTextEntity_{};
    bool stageClearActive_ = false;
    float stageClearTimer_ = 0.0f;

    float pauseInputLock_ = 0.0f;

    DirectX::XMFLOAT3 cameraPosition_ = {0.0f, 30.0f, -7.0f};
    DirectX::XMFLOAT3 currentTarget_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 lastDesiredTarget_ = {0.0f, 0.0f, 0.0f};
    float currentVerticalBlend_ = 1.0f; // 0..1, controls how much of Y delta is applied (starts full)
    Camera camera_{};
    float baseFovY_ = DirectX::XM_PIDIV4;
    float cameraNear_ = 0.1f;
    float cameraFar_ = 10000.0f;
    DirectX::XMFLOAT3 baseUp_ = {0.0f, 1.0f, 0.0f};

    // シェイク用
    bool shakeActive_ = false;
    float shakeIntensity_ = 0.0f;
    float shakeTime_ = 0.0f;
    float shakeElapsed_ = 0.0f;

    // 衝撃用
    bool impulseActive_ = false;
    DirectX::XMFLOAT3 impulseDir_ = {0.0f, 0.0f, 0.0f};
    float impulseIntensity_ = 0.0f;
    float impulseTime_ = 0.0f;
    float impulseElapsed_ = 0.0f;

    // ズーム用
    bool zoomActive_ = false;
    float zoomAmount_ = 0.0f;
    float zoomTime_ = 0.0f;
    float zoomElapsed_ = 0.0f;

    // 遅延リスポーン用
    bool pendingRespawn_ = false;
    Entity respawnPlayer_{};
    float respawnTimer_ = 0.0f;
    World *world_ = nullptr;
    StageAdvanceInfo pendingStageAdvance_{};
    float stageAdvanceTimer_ = 0.0f;
    float chargeOverlayCurrent_ = 0.0f;
    float chargeOverlayTarget_ = 0.0f;
    bool chargeOverlayVisible_ = false;
    bool deathFadeVisible_ = false;
    bool startFadeVisible_ = false;
    float stickZoomTarget_ = 0.0f;
    float stickZoomCurrent_ = 0.0f;
    float stickZoomRatioCurrent_ = 0.0f; ///< スティックズーム時の現在の寄り率（0〜1）
    TextureManager::TextureHandle skyboxTexture_ = TextureManager::INVALID_TEXTURE;
    bool skyboxTextureApplied_ = false;

    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
    float skyboxRotation_ = 0.0f;
};

// =========================================
// 衝突ハンドラーの実装（GameScene定義後）
// =========================================

// GameScene への依存を持たない UI トリガーのラッパー（定義後ならメンバー呼び出し可）
inline void GameScene_OnChargeStart(World &w) {
    if (g_GameScene) {
        g_GameScene->OnChargeStart(w);
    }
}
inline void GameScene_OnChargeRelease(World &w, float chargeAmount01) {
    if (g_GameScene) {
        g_GameScene->OnChargeRelease(w, chargeAmount01);
    }
}
inline void GameScene_OnTimeUp(World &w, Entity player) {
    if (g_GameScene) {
        g_GameScene->OnTimeUp(player, w);
    } else {
        SOUND_SYS.PlaySE(cfg_DeathMP3Pass,false);
        SOUND_SYS.StopSE(cfg_DriftMP3Pass);
        ResetPlayerToStart(w, player, true);
    }
}
inline void GameScene_ResetChargeState() {
    if (g_GameScene) {
        g_GameScene->ResetChargeState();
    }
}

inline void WallCollisionHandler::OnCollisionEnter(World &w, Entity self, Entity other, const CollisionInfo &info) {
    if (w.Has<PlayerTag>(other)) {
        // ゴール演出中は壁判定を無効化
        bool isGoalTransition = false;

        auto *tEffect = w.TryGet<PlayerMovement>(other);
        tEffect->SwitchEffect(w, other, PlayerMovement::EffectState::Idle);

        w.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.goalTransitioning)
                isGoalTransition = true;
        });
        if (isGoalTransition)
            return;

        // 床としての接触（法線が上向き）ならダメージ処理を行わない
        if (info.normal.y > 0.5f)
            return;

        SOUND_SYS.PlaySE(cfg_CollideMP3Pass.Get(),false);

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
        // ゴール演出中は壁判定を無効化
        bool isGoalTransition = false;
        w.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            if (sp.goalTransitioning)
                isGoalTransition = true;
        });
        if (isGoalTransition)
            return;

        // 床としての接触なら無視
        if (info.normal.y > 0.5f)
            return;

        SOUND_SYS.PlaySE(cfg_CollideMP3Pass.Get(),false);

        DEBUGLOG("ステージ壁がプレイヤーと衝突 - カメラシェイク＋遅延リスポーン");
        if (g_GameScene) {
            g_GameScene->OnWallHit(other, w);
        } else {
            ResetPlayerToStart(w, other, true);
        }
    }
}

