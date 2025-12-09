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
#include "animation/Animation.h"
#include "config/ConfigVar.h"

//Config Var
inline static ConfigVar<float> cfg_ChargingFade{"Animation.Fade", "ChargingFade", 0.4f};
inline static ConfigVar<float> cfg_GoalDistance{"Distance.Goal",  "GoalDistance", 2.0f};
inline static ConfigVar<float> cfg_SlowDirection{"Direction.Slow","SlowDistance", 0.2f};

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
        DirectX::XMFLOAT3 ambientColor{
            cfg_AmbientR.Get(),
            cfg_AmbientG.Get(),
            cfg_AmbientB.Get()
        };
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
                    std::max(0.0f, cfg_DirLightIntensity.Get())
                };
            }
        }
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

        // カメラを初期化（App設定を優先）
        camera_ = Camera::LookAtLH(
            baseFovY_,
            screenWidth / screenHeight,
            cameraNear_,
            cameraFar_,
            cameraPosition_,
            baseTarget_,
            baseUp_
        );

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
            status.currentRoom = 1; // ステージ開始時は常にroom1から

            auto stagePath = ResolveStageRoomCsvPath(desiredStage, status.currentRoom);
            if (!stagePath) {
                DEBUGLOG_ERROR("[StageCreate] ステージ" + std::to_string(desiredStage) + " の room" + std::to_string(status.currentRoom) + ".csv が見つかりません。Stage1/room1へフォールバックします");
                status.currentStage = 1;
                status.selectStage = 1;
                status.currentRoom = 1;
                stagePath = ResolveStageRoomCsvPath(1, 1);
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

        // 目標(ゴール)接近時のスロー演出用タイムスケール
        float timeScale = 1.0f;
        float GoalDistance = cfg_GoalDistance;
        float SlowDirection = cfg_SlowDirection;
        if (world.IsAlive(playerEntity_) && world.IsAlive(goalEntity_)) {
            auto *tPlayer = world.TryGet<Transform>(playerEntity_);
            auto *tGoal = world.TryGet<Transform>(goalEntity_);
            if (tPlayer && tGoal) {
                const float dx = tPlayer->position.x - tGoal->position.x;
                const float dz = tPlayer->position.z - tGoal->position.z;
                const float dist = std::sqrt(dx * dx + dz * dz);
                const float slowThreshold = GoalDistance; // ゴールに近づいたとみなす距離
                if (dist <= slowThreshold) {
                    timeScale = SlowDirection; // スロー演出
                }
            }
        }

        // ステージ進行リクエストの処理
        HandleStageAdvance(world);
        UpdateStageTransition(world, deltaTime * timeScale);

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
        ChargCameraAction(world);
        RenderingSystem::GetInstance().UpdateLights(world, camera_.position);

        // ECSワールドのTickを進める
        world.Tick(deltaTime * timeScale);

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
        camera_.position = cameraPosition_;
        camera_.target = baseTarget_;
        camera_.fovY = baseFovY_;
        camera_.up = baseUp_;
        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    // チャージ開始/解放演出
    void OnChargeStart(World &world) {
        float ChargingFade = cfg_ChargingFade;
        chargeOverlayTarget_ = ChargingFade;
        chargeOverlayVisible_ = true;
        TriggerCameraZoom(-0.12f, 0.25f);
    }
    void OnChargeRelease(World &world, float chargeAmount01) {
        chargeOverlayCurrent_ = std::max(chargeOverlayCurrent_, 0.55f);
        chargeOverlayTarget_ = 0.0f;
        chargeOverlayVisible_ = true;
        float impulse = std::clamp(chargeAmount01, 0.15f, 1.0f) * 0.12f;
        TriggerCameraShake(0.03f + impulse, 0.25f);
    }
    
    void UpdateDeathFade(World &world, float /*dt*/) {
        if (!world.IsAlive(deathFadeAnimationEntity_)) return;
        auto *img = world.TryGet<UIImage>(deathFadeAnimationEntity_);
        auto *anim = world.TryGet<SpriteSheetAnimation>(deathFadeAnimationEntity_);
            
        if (deathFadeVisible_) {
            if (img) img->opacity = 0.1f;
            if (anim && anim->isFinished && !anim->isPlaying) {
                deathFadeVisible_ = false;
                if (img) img->opacity = 0.0f;
            }
        } else {
            if (img) img->opacity = 0.0f;
        }
    }

    /** @brief カメラオブジェクトへのconst参照を取得 */
    const Camera& GetCamera() const { return camera_; }

    /** @brief カメラの基準位置を設定 */
    void SetCameraBasePosition(const DirectX::XMFLOAT3& pos) { cameraPosition_ = pos; }
    /** @brief カメラの基準設定を一括で指定（位置・注視点・Up・FOV・Near/Far） */
    void ConfigureBaseCamera(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up, float fovRad, float nearZ, float farZ) {
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
        if (pendingRespawn_) return;

        // 再生用フェードアニメーションを開始
        StartDeathFadeOut(world);

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

    /** @brief リスポーン待機中かを取得 */
    bool IsRespawnPending() const { return pendingRespawn_; }

  private:
    struct StageAdvanceInfo {
        bool active = false;
        bool stageBuilt = false;
        int stage = 0;
        int nextRoom = 0;
        std::string nextRoomPath;
    };

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

                // 同一ステージ内で次のroomへ
                const int nextRoomIndex = sp.currentRoom + 1;
                auto nextRoomPath = ResolveStageRoomCsvPath(sp.currentStage, nextRoomIndex);
                if (!nextRoomPath) {
                    DEBUGLOG_WARNING("[StageCreate] Stage" + std::to_string(sp.currentStage) + "/room" + std::to_string(nextRoomIndex) + ".csv が見つかりません。進行をキャンセルします");
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

                StartFadeOutNormal(world);
                DEBUGLOG("同一ステージ内で次のルームへ進行(フェード演出開始): Stage" + std::to_string(sp.currentStage) + ", room" + std::to_string(pendingStageAdvance_.nextRoom));
            }
        });
    }

    void UpdateStageTransition(World &world, float dt) {
        if (!pendingStageAdvance_.active) return;

        stageAdvanceTimer_ += dt;
        auto *anim = world.TryGet<SpriteSheetAnimation>(fadeAnimationEntity_);
        const float fadeDuration = GetFadeDurationSeconds(world, fadeAnimationEntity_);
        const float waitDuration = (fadeDuration > 0.0f) ? fadeDuration : 1.0f;

        const bool fadeOutFinished = anim && !anim->isPlaying && anim->isFinished && anim->playbackDirection >= 0;
        if (!pendingStageAdvance_.stageBuilt) {
            if (fadeOutFinished || stageAdvanceTimer_ >= waitDuration) {
                stageAdvanceTimer_ = 0.0f;

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

                Entity newStageEntity = world.Create().With<StageCreate>(pendingStageAdvance_.nextRoomPath).Build();
                ownedEntities_.push_back(newStageEntity);

                DEBUGLOG("同一ステージ内で次のルームへ進行: Stage" + std::to_string(pendingStageAdvance_.stage) + ", room" + std::to_string(pendingStageAdvance_.nextRoom));
                SetupStage(world, pendingStageAdvance_.stage);
                pendingStageAdvance_.stageBuilt = true;
                StartFadeInNormal(world);
            }
            return;
        }

        const bool fadeInFinished = anim && !anim->isPlaying && anim->isFinished && anim->playbackDirection < 0;
        if (fadeInFinished || stageAdvanceTimer_ >= waitDuration) {
            pendingStageAdvance_ = {};
            stageAdvanceTimer_ = 0.0f;
        }
    }

    void StartFadeOutNormal(World &world) { StartSpriteFade(world, fadeAnimationEntity_, 1, false); }
    void StartFadeInNormal(World &world) { StartSpriteFade(world, fadeAnimationEntity_, -1, false); }
    void StartDeathFadeOut(World &world) { deathFadeVisible_ = true; StartSpriteFade(world, deathFadeAnimationEntity_, 1, true); }

    void StartSpriteFade(World &world, Entity target, int direction, bool forceOpaque) {
        if (!world.IsAlive(target)) return;
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(target)) {
            anim->isLooping = false;
            anim->StartAnimation(direction);
            ApplyFadeFrame(world, target, *anim);
            if (forceOpaque) {
                if (auto *img = world.TryGet<UIImage>(target)) {
                    img->opacity = 1.0f;
                }
            }
        }
        // 対象が死亡フェード以外の場合、不要時は透明化
        if (!forceOpaque) {
            if (auto *img = world.TryGet<UIImage>(target)) {
                img->opacity = 1.0f;
            }
        }
    }

    float GetFadeDurationSeconds(World &world, Entity target) const {
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(target)) {
            const int count = std::max(anim->frameCount, 0);
            return anim->frameTime * static_cast<float>(count);
        }
        return 0.0f;
    }

    void ApplyFadeFrame(World &world, Entity target, SpriteSheetAnimation &anim) {
        if (anim.uv.size() != static_cast<size_t>(anim.frameCount)) {
            anim.UpdateUV();
        }

        if (auto *img = world.TryGet<UIImage>(target)) {
            const int frameIndex = std::clamp(anim.currentFrame, 0, std::max(0, anim.frameCount - 1));
            if (!anim.uv.empty()) {
                img->uvRect = anim.uv[frameIndex];
            }
        }
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
                p.dirY = -vals[1];
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

    void ApplyDefaultPointLightParams(PointLight &light) const {
        light.SetAttenuation(cfg_PointLightConst.Get(), cfg_PointLightLinear.Get(), cfg_PointLightQuadratic.Get());
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

            BakeStageLights(world, stagecreate.stageMap, tileSize);
        });
    }

    void BakeStageLights(World &world, const std::vector<std::vector<int>> &stageMap, float tileSize) {
        // 既存のポイントライトに対して、ステージスケールに応じた減衰・レンジを適用するだけ（新規ライトは生成しない）
        if (stageMap.empty() || stageMap[0].empty()) return;

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
            if (block == 0 || block == 1 || block == 2) return true;
            if (block >= 10 && block < 20) return true;
            if (block >= 30 && block < 40) return true;
            if (block >= 50) return true;
            return false; // 3系などの壁
        };

        for (int y = 0; y < static_cast<int>(stageMap.size()); ++y) {
            for (int x = 0; x < static_cast<int>(stageMap[y].size()); ++x) {
                if (!isWalkable(stageMap[y][x])) continue;
                float worldX = (static_cast<float>(x) * tileSize) - offsetX;
                float worldZ = offsetZ - (static_cast<float>(y) * tileSize);
                minX = std::min(minX, worldX);
                maxX = std::max(maxX, worldX);
                minZ = std::min(minZ, worldZ);
                maxZ = std::max(maxZ, worldZ);
            }
        }

        if (minX > maxX || minZ > maxZ) return;

        const float diag = std::sqrt((maxX - minX) * (maxX - minX) + (maxZ - minZ) * (maxZ - minZ));
        const float targetRange = std::max(cfg_PointLightRange.Get(), 0.3f * diag);

        world.ForEach<PointLight>([&](Entity, PointLight &pl) {
            ApplyDefaultPointLightParams(pl);
            pl.range = targetRange;
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
                    CreateMovingObstacle(world, position, blockType);
                } else if (blockType >= 30 && blockType < 40) {
                    CreateDashBoard(world, position, blockType);
                } else if (blockType == 50 || blockType == 51) {
                    CreateObjectA(world, position, blockType);
                } else if (blockType == 52 || blockType == 53) {
                    CreateObjectB(world, position, blockType);
                }
                break;
            case 54: CreateObjectC(world, position, blockType);break;
        }
    }

    void CreateFloor(World &world, const DirectX::XMFLOAT3 &position) {
        // 各マスにフロアFBXをそのまま配置する（スケールは1x1x1、Yは設定値でオフセット）
        DirectX::XMFLOAT3 floorPos = {position.x, position.y + cfg_FloorYOffset, position.z};
        Transform transform{{floorPos}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};

        Entity floor = world.Create()
                           .With<Transform>(transform)
                           .With<Model>(cfg_FloorFBXPass)
                           .Build();

        // ステージ切り替え時に破棄されるよう、ステージ所有リストへ登録
        stageOwnedEntities_.push_back(floor);
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
        ApplyDefaultPointLightParams(light);

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
        ApplyDefaultPointLightParams(light);

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
        DirectX::XMFLOAT3 diffPosition = {position.x, position.y - 1.0f, position.z};
        Transform transform{diffPosition, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};

        Entity wallEntity = world.Create()
                                .With<Transform>(transform)
                                .With<Model>(cfg_WallFBXPass)
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
                                .With<Model>(cfg_HalfWallFBXPass)
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
                                .With<Model>(cfg_HalfWallFBXPass)
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
                                .With<Model>(cfg_HalfWallFBXPass)
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
                                .With<Model>(cfg_HalfWallFBXPass)
                                .With<MeshRenderer>(renderer)
                                .With<WallTag>()
                                .With<CollisionRightIsoTriPrism>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                .With<WallCollisionHandler>()
                                .Build();

        stageOwnedEntities_.push_back(wallEntity);
    }

    void CreateMovingObstacle(World &world, const DirectX::XMFLOAT3 &position, int blockType) {
        MovingObstacle obstacle;
        obstacle.startPos = position;
        obstacle.baseScale = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};

        auto resolvePatternIndex = [](int type) -> std::optional<int> {
            if (type >= 10 && type < 20) return type - 10;  // CSVのインデックス+10がID
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
                    position.x + obstacle.delta.x,
                    position.y + obstacle.delta.y,
                    position.z + obstacle.delta.z};
                obstacle.waitAtStart = p.waitAtStart;
                obstacle.waitAtEnd = p.waitAtEnd;
                obstacle.travelTime = p.travelTime;
            } else {
                DEBUGLOG_WARNING("[MoveObstacle] パターンが見つかりません index=" + std::to_string(*patternIndex));
            }
        });


        Transform transform{position, {0.0f, 0.0f, 0.0f}, obstacle.baseScale};

        Entity entity = world.Create()
                            .With<Transform>(transform)
                            .With<Model>(cfg_MovingObstacleFBXPass)
                            .With<WallTag>()
                            .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                            .With<WallCollisionHandler>()
                            .With<MovingObstacle>(obstacle)
                            .Build();

        stageOwnedEntities_.push_back(entity);
    }

    void CreateObjectA(World & world, const DirectX::XMFLOAT3 &position, int blockType) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;

        Entity ObjectAEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<Model>(cfg_AObstacleFBXPass)
                                     .With<MeshRenderer>(renderer)
                                     .With<WallTag>()
                                     .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                     .With<FloorWallCollisionHandler>()
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
                                   .With<WallTag>()
                                   .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                   .With<FloorWallCollisionHandler>()
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
                                   .With<WallTag>()
                                   .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                   .With<FloorWallCollisionHandler>()
                                   .Build();

        stageOwnedEntities_.push_back(ObjectAEntity);
    }

    void CreatFloorWall(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, cfg_WallSize, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        // 境界壁のデフォルト色を設定（白ではなく設定値）
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
        while (csvAngleDeg < 0.0f) csvAngleDeg += 360.0f;
        while (csvAngleDeg >= 360.0f) csvAngleDeg -= 360.0f;

        // 見た目補正: FBXのデフォルト向きが+90度ずれているため、モデルの回転のみ+90度補正
        const float visualYawDeg = -csvAngleDeg + 90.0f + 180.0f; // 180度反転で見た目を加速方向に合わせる

        DirectX::XMFLOAT3 adjustedPos = position;
        adjustedPos.y -= 0.5f;
        Transform transform{{adjustedPos}, {0.0f, visualYawDeg, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f};

        // プレイヤーへの影響角度はCSVそのまま（見た目補正は加えない）
        status.accelAngle = csvAngleDeg;

        Entity dashBoardEntity = world.Create()
                                     .With<Transform>(transform)
                                     .With<Model>(cfg_DashBoardFBXPass)
                                     .With<CollisionBox>(DirectX::XMFLOAT3{1.0f, 2.0f, 1.0f})
                                     .With<GimmickTag>()
                                     .With<DashBordCollisionHandler>()
                                     .With<DashBoardStatus>(status)
                                     .Build();

        stageOwnedEntities_.push_back(dashBoardEntity);
    }



    void BakeStageLighting(World & /*world*/) {
        // Deprecated placeholder（現状は CreateStageMap 内で BakeStageLights を実行）
    }

    void SetupStage(World &world, int stage) {
        // 既存のステージ所有エンティティを破棄
        for (const auto &entity : stageOwnedEntities_) {
            if (world.IsAlive(entity)) {
                world.DestroyEntityWithCause(entity, World::Cause::StageReset);
            }
        }
        stageOwnedEntities_.clear();

        // 念のため、タグでステージ要素をクリーンアップ（過去の登録漏れ対策）
        std::vector<Entity> toDestroy;
        world.ForEach<WallTag>([&](Entity e, WallTag &) { toDestroy.push_back(e); });
        world.ForEach<GoalTag>([&](Entity e, GoalTag &) { toDestroy.push_back(e); });
        world.ForEach<StartTag>([&](Entity e, StartTag &) { toDestroy.push_back(e); });
        world.ForEach<GimmickTag>([&](Entity e, GimmickTag &) { toDestroy.push_back(e); });
        for (auto e : toDestroy) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::StageReset);
            }
        }

        // 破棄を即時反映（次のステージ生成と重ならないように）
        world.FlushDestroyEndOfFrame();

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
        // グローバルにも反映して他コンポーネントから参照可能に
        g_respawnPending = true;
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) {
            ResetPlayerToStart(world, respawnPlayer_, true);
            StartFadeInNormal(world);
            pendingRespawn_ = false;
            g_respawnPending = false;
            respawnTimer_ = 0.0f;
        }
        if (auto* playerStatus = world.TryGet<PlayerStatus>(playerEntity_))
        {
            playerStatus->isStartAfterWallHit = false;
            DEBUGLOG("isStartAfterWallHitがfalseになりました " );
        }
    }

    void UpdateCameraReaction(float dt, World & /*world*/) {
        DirectX::XMFLOAT3 posOffset{0.0f, 0.0f, 0.0f};
        DirectX::XMFLOAT3 targetOffset{0.0f, 0.0f, 0.0f};
        float fovDelta = 0.0f;
        DirectX::XMFLOAT3 upVec = baseUp_;

        if (shakeActive_) UpdateShake(dt, posOffset, targetOffset, upVec);
        if (impulseActive_) UpdateImpulse(dt, posOffset, targetOffset);
        if (zoomActive_) UpdateZoom(dt, fovDelta);

        camera_.position = {cameraPosition_.x + posOffset.x, cameraPosition_.y + posOffset.y, cameraPosition_.z + posOffset.z};
        camera_.target = {baseTarget_.x + targetOffset.x, baseTarget_.y + targetOffset.y, baseTarget_.z + targetOffset.z};
        camera_.fovY = std::clamp(baseFovY_ + fovDelta, DirectX::XM_PIDIV4 * 0.25f, DirectX::XM_PIDIV2 * 1.5f);
        camera_.up = upVec;

        camera_.Proj = DirectX::XMMatrixPerspectiveFovLH(camera_.fovY, camera_.aspect, camera_.nearZ, camera_.farZ);
        camera_.Update();
    }

    void UpdateChargeOverlay(World &world, float dt) {
        if (!world.IsAlive(chargeOverlayEntity_)) return;
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
        DirectX::XMFLOAT3 baseForward{
            baseTarget_.x - cameraPosition_.x,
            baseTarget_.y - cameraPosition_.y,
            baseTarget_.z - cameraPosition_.z};
        float forwardLen = std::sqrt(baseForward.x * baseForward.x + baseForward.y * baseForward.y + baseForward.z * baseForward.z);
        if (forwardLen < 1e-4f) return;

        const float angleScale = 0.25f; // 揺れ量を角度に変換
        float pitch = sy * angleScale;
        float yaw = sx * angleScale;
        float roll = sz * angleScale * 0.5f;

        DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&baseForward);
        forwardVec = DirectX::XMVector3Normalize(forwardVec);

        DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        DirectX::XMVECTOR rotatedForward = DirectX::XMVector3TransformNormal(forwardVec, rot);
        DirectX::XMVECTOR rotatedUp = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&baseUp_), rot);

        DirectX::XMFLOAT3 newForward{};
        DirectX::XMFLOAT3 newUp{};
        DirectX::XMStoreFloat3(&newForward, rotatedForward);
        DirectX::XMStoreFloat3(&newUp, rotatedUp);

        targetOffset = {newForward.x * forwardLen - baseForward.x,
                        newForward.y * forwardLen - baseForward.y,
                        newForward.z * forwardLen - baseForward.z};
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
    Entity fadeAnimationEntity_{};
    Entity deathFadeAnimationEntity_{};
    Entity chargeOverlayEntity_{};
    DirectX::XMFLOAT3 cameraPosition_ = {0.0f, 30.0f, -7.0f};
    DirectX::XMFLOAT3 currentTarget_ = {0.0f, 0.0f, 0.0f};
    Camera camera_{};
    float baseFovY_ = DirectX::XM_PIDIV4;
    float cameraNear_ = 0.1f;
    float cameraFar_ = 1000.0f;
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

    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
};

// =========================================
// 衝突ハンドラーの実装（GameScene定義後）
// =========================================

// GameScene への依存を持たない UI トリガーのラッパー（定義後ならメンバー呼び出し可）
inline void GameScene_OnChargeStart(World &w) {
    if (g_GameScene) { g_GameScene->OnChargeStart(w); }
}
inline void GameScene_OnChargeRelease(World &w, float chargeAmount01) {
    if (g_GameScene) { g_GameScene->OnChargeRelease(w, chargeAmount01); }
}

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
