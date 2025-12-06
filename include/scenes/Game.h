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
#include <comdef.h>

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
        RenderingSystem::GetInstance().SetAmbientLight({0.08f, 0.08f, 0.08f}, 1.0f);
        // テキスト描画システムの初期化
        try {
            if (!textSystem_.Init(*gfx)) {
                DEBUGLOG_ERROR("TextSystem の初期化に失敗しました");
                return;
            }
        } catch (const _com_error& ex) {
            std::wostringstream woss;
            const wchar_t* wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
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
        } catch (const _com_error& ex) {
            std::wostringstream woss;
            const wchar_t* wmsg = ex.ErrorMessage() ? ex.ErrorMessage() : L"unknown";
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
        int initialStage = 1;
        const int maxStage = GetAvailableStageCount();
        world.ForEach<StageProgress>([&](Entity e, StageProgress &status) {
            int desiredStage = status.selectStage > 0 ? status.selectStage : 1;
            desiredStage = std::min(desiredStage, maxStage);
            if (desiredStage != status.selectStage) {
                DEBUGLOG_WARNING("[StageCreate] 要求ステージ " + std::to_string(status.selectStage) + " をクランプ: " + std::to_string(desiredStage));
            }

            status.selectStage = desiredStage;
            status.currentStage = desiredStage;

            auto stagePath = ResolveStageCsvPath(desiredStage);
            if (!stagePath) {
                DEBUGLOG_ERROR("[StageCreate] ステージ" + std::to_string(desiredStage) + "のCSVが見つかりません。Stage1へフォールバックします");
                stagePath = ResolveStageCsvPath(1);
            }

            if (stagePath) {
                Entity stageEntity = world.Create().With<StageCreate>(*stagePath).Build();
                ownedEntities_.push_back(stageEntity);
            } else {
                DEBUGLOG_ERROR("[StageCreate] 有効なステージCSVが見つからず、ステージ生成をスキップしました");
            }

            initialStage = status.currentStage;
        });

        // 平行光源をエンティティとして生成
        world.Create().With<DirectionalLight>();

        CreatePlayer(world);
        CreateUI(world, screenWidth, screenHeight);

        SetupStage(world, initialStage);

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
        ChargCameraAction(world);
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
                    if (static_cast<bool>(pv->velocity.x + pv->velocity.y)) {
                        float vecX = pv->velocity.x / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;
                        float vecY = pv->velocity.y / (pv->velocity.x + pv->velocity.y) * impulseIntensity_;

                        TriggerCameraImpulse(vecX, 0.0f, vecY, 0.2f, 0.1f);
                    }
                };
            }
        });

    }

    /**
     * @brief 壁衝突時の処理（カメラシェイク＋遅延リスポーン）
     */
    void OnWallHit(Entity player, World &world) {
        if (pendingRespawn_) return;

        if (auto* playerStatus = world.TryGet<PlayerStatus>(playerEntity_))
        {
            playerStatus->isStartAfterWallHit = true;
            DEBUGLOG("isStartAfterWallHitがtrueになりました");
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
                            .With<PlayerStatus>()
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

                const int maxStage = GetAvailableStageCount();
                const int nextStageIndex = std::min(sp.currentStage + 1, maxStage);
                if (nextStageIndex == sp.currentStage) {
                    DEBUGLOG_WARNING("進行可能なステージが存在しません (current=" + std::to_string(sp.currentStage) + ", max=" + std::to_string(maxStage) + ")");
                    return;
                }

                auto nextStagePath = ResolveStageCsvPath(nextStageIndex);
                if (!nextStagePath) {
                    DEBUGLOG_ERROR("[StageCreate] ステージ" + std::to_string(nextStageIndex) + "のCSVが見つからず、進行をキャンセルします");
                    return;
                }

                std::vector<Entity> stageCreateEntities;
                world.ForEach<StageCreate>([&](Entity e, StageCreate &) {
                    stageCreateEntities.push_back(e);
                });
                for (auto e : stageCreateEntities) {
                    if (world.IsAlive(e)) {
                        world.DestroyEntityWithCause(e, World::Cause::StageReset);
                    }
                }

                sp.currentStage = nextStageIndex;
                sp.selectStage = nextStageIndex;

                Entity newStageEntity = world.Create().With<StageCreate>(*nextStagePath).Build();
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

    std::optional<std::string> ResolveSpeedUpCsvPath(const std::string &stageCollisionCsvPath) {
        namespace fs = std::filesystem;
        fs::path collisionPath(stageCollisionCsvPath);
        const fs::path stageDir = collisionPath.parent_path().filename();
        if (stageDir.empty()) {
            return std::nullopt;
        }

        fs::path speedUpPath = fs::path("Assets/StageData/UniqueObj/SpeedUp") / stageDir / collisionPath.filename();
        std::error_code ec;
        if (!fs::exists(speedUpPath, ec) || ec) {
            return std::nullopt;
        }

        return speedUpPath.string();
    }

    std::optional<std::string> ResolveMovingObstacleCsvPath(const std::string &stageCollisionCsvPath) {
        namespace fs = std::filesystem;
        fs::path collisionPath(stageCollisionCsvPath);
        const fs::path stageDir = collisionPath.parent_path().filename();
        if (stageDir.empty()) {
            return std::nullopt;
        }

        fs::path movePath = fs::path("Assets/StageData/UniqueObj/Move") / stageDir / collisionPath.filename();
        std::error_code ec;
        if (!fs::exists(movePath, ec) || ec) {
            return std::nullopt;
        }

        return movePath.string();
    }

    std::vector<std::vector<int>> LoadAngleCsv(const std::string &csvPath) {
        std::vector<std::vector<int>> angles;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG_ERROR("[SpeedUp] 角度CSVが開けません: " + csvPath);
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

    std::vector<MovingObstaclePattern> LoadMovingObstacleCsv(const std::string &csvPath) {
        std::vector<MovingObstaclePattern> patterns;
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            DEBUGLOG_ERROR("[MoveObstacle] CSVが開けません: " + csvPath);
            return patterns;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
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
                p.dirY = vals[1];
                p.waitAtStart = vals[2];
                p.waitAtEnd = vals[3];
                p.travelTime = std::max(vals[4], 0.0001f);
                patterns.push_back(p);
            }
        }

        return patterns;
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
                } else if (blockType >= 50 && blockType < 60) {
                    CreateMovingObstacle(world, position, blockType);
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

    struct MovingObstacle : Behaviour {
        DirectX::XMFLOAT3 startPos{};
        DirectX::XMFLOAT3 endPos{};
        DirectX::XMFLOAT3 delta{};
        DirectX::XMFLOAT3 baseScale{1.0f, 1.0f, 1.0f};
        float waitAtStart = 0.0f;
        float waitAtEnd = 0.0f;
        float travelTime = 1.0f;
        float timer = 0.0f;
        enum class State { WaitStart, MoveForward, WaitEnd, MoveBack } state = State::WaitStart;

        void OnUpdate(World &w, Entity self, float dt) override {
            auto *t = w.TryGet<Transform>(self);
            if (!t) return;

            // スケールが他所で変更されないよう固定
            t->scale = baseScale;

            timer += dt;

            auto lerpVec = [](const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b, float r) {
                return DirectX::XMFLOAT3{
                    a.x + (b.x - a.x) * r,
                    a.y + (b.y - a.y) * r,
                    a.z + (b.z - a.z) * r};
            };

            switch (state) {
                case State::WaitStart:
                    if (timer >= waitAtStart) {
                        timer = 0.0f;
                        state = State::MoveForward;
                    }
                    break;
                case State::MoveForward: {
                    float ratio = std::clamp(timer / std::max(travelTime, 0.0001f), 0.0f, 1.0f);
                    t->position = lerpVec(startPos, endPos, ratio);
                    if (timer >= travelTime) {
                        timer = 0.0f;
                        state = State::WaitEnd;
                        t->position = endPos;
                    }
                    break;
                }
                case State::WaitEnd:
                    if (timer >= waitAtEnd) {
                        timer = 0.0f;
                        state = State::MoveBack;
                    }
                    break;
                case State::MoveBack: {
                    float ratio = std::clamp(timer / std::max(travelTime, 0.0001f), 0.0f, 1.0f);
                    t->position = lerpVec(endPos, startPos, ratio);
                    if (timer >= travelTime) {
                        timer = 0.0f;
                        state = State::WaitStart;
                        t->position = startPos;
                    }
                    break;
                }
            }
        }
    };

    void CreateMovingObstacle(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        MovingObstacle obstacle;
        obstacle.startPos = position;
        obstacle.baseScale = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};

        // blockType 50ベースでパターンを取得
        int patternIndex = blockType - 50;
        world.ForEach<LoadMovingObstacle>([&](Entity, LoadMovingObstacle &data) {
            if (patternIndex >= 0 && patternIndex < static_cast<int>(data.patterns.size())) {
                const auto &p = data.patterns[patternIndex];
                obstacle.delta = DirectX::XMFLOAT3{p.dirX, 0.0f, p.dirY}; // そのままの差分ベクトル
                obstacle.endPos = DirectX::XMFLOAT3{
                    position.x + obstacle.delta.x,
                    position.y + obstacle.delta.y,
                    position.z + obstacle.delta.z};
                obstacle.waitAtStart = p.waitAtStart;
                obstacle.waitAtEnd = p.waitAtEnd;
                obstacle.travelTime = p.travelTime;
            }
        });

        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{1.0f, 0.3f, 0.3f}; // 目立つ赤

        Transform transform{position, {0.0f, 0.0f, 0.0f}, obstacle.baseScale};

        Entity entity = world.Create()
                              .With<Transform>(transform)
                              .With<MeshRenderer>(renderer)
                              .With<WallTag>()
                              .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                              .With<WallCollisionHandler>()
                              .With<MovingObstacle>(obstacle)
                              .Build();

        stageOwnedEntities_.push_back(entity);
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
            const int angleIndex = blockType - 30;
            if (angleIndex >= 0 && angleIndex < static_cast<int>(loadAngle.stageAngle.size())) {
                const auto &row = loadAngle.stageAngle[angleIndex];
                if (!row.empty()) {
                    angle = static_cast<float>(row[0]);
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

        // ステージに紐づく加速角度CSVをロードしてLoadAngleコンポーネントに反映
        world.ForEach<StageCreate>([&](Entity, StageCreate &stagecreate) {
            auto angleCsvPath = ResolveSpeedUpCsvPath(stagecreate.csvPath);
            std::vector<std::vector<int>> angles;
            if (angleCsvPath) {
                angles = LoadAngleCsv(*angleCsvPath);
                if (angles.empty()) {
                    DEBUGLOG_WARNING("[SpeedUp] 角度CSVが空、または読み込みに失敗しました: " + *angleCsvPath);
                }
            } else {
                DEBUGLOG_WARNING("[SpeedUp] 角度CSVパスを解決できません: " + stagecreate.csvPath);
            }

            bool updated = false;
            world.ForEach<LoadAngle>([&](Entity, LoadAngle &loadAngle) {
                loadAngle.stageAngle = angles;
                updated = true;
            });

            if (!updated) {
                LoadAngle loadAngle;
                loadAngle.stageAngle = angles;
                Entity angleEntity = world.Create().With<LoadAngle>(loadAngle).Build();
                stageOwnedEntities_.push_back(angleEntity);
            }

            // 動く障害物CSVもステージごとにロード
            auto moveCsvPath = ResolveMovingObstacleCsvPath(stagecreate.csvPath);
            std::vector<MovingObstaclePattern> movePatterns;
            if (moveCsvPath) {
                movePatterns = LoadMovingObstacleCsv(*moveCsvPath);
                if (movePatterns.empty()) {
                    DEBUGLOG_WARNING("[MoveObstacle] CSVが空、または読み込みに失敗しました: " + *moveCsvPath);
                }
            } else {
                DEBUGLOG_WARNING("[MoveObstacle] CSVパスを解決できません: " + stagecreate.csvPath);
            }

            bool moveUpdated = false;
            world.ForEach<LoadMovingObstacle>([&](Entity, LoadMovingObstacle &loadMove) {
                loadMove.patterns = movePatterns;
                moveUpdated = true;
            });

            if (!moveUpdated) {
                LoadMovingObstacle loadMove;
                loadMove.patterns = movePatterns;
                Entity moveEntity = world.Create().With<LoadMovingObstacle>(loadMove).Build();
                stageOwnedEntities_.push_back(moveEntity);
            }
        });

        CreateStageMap(world);
        BakeStageLighting(world);

        if (world.IsAlive(playerEntity_)) {
            ResetPlayerToStart(world, playerEntity_);
        }
    }

    // =========================================
    // 遅延リスポーン・カメラリアクション更新
    // =========================================

    void UpdateDelayedRespawn(float dt, World &world) 
    {


        if (auto *movement = world.TryGet<PlayerMovement>(playerEntity_))
        {
            movement->isCharging_ = false;
        }
        if (!pendingRespawn_) return;
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            ResetPlayerToStart(world, respawnPlayer_, true);
            pendingRespawn_ = false;
            respawnTimer_ = 0.0f;
        }
        if (auto* playerStatus = world.TryGet<PlayerStatus>(playerEntity_))
        {
            playerStatus->isStartAfterWallHit = false;
            DEBUGLOG("isStartAfterWallHitがfalseになりました " );
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
