/**
 * @file StageSelectScene.h
 * @brief 統合されたワールドセレクトシーン
 * @author 山内陽/立山悠朔
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include <vector>
#include <algorithm>
#include <string>
#include <array>
#include <DirectXMath.h>

#include "graphics/Effect.h"
#include "config/ConfigVar.h"
#include "components/UIComponents.h"
#include "components/StageComponents.h"
#include "graphics/TextSystem.h"
#include "graphics/Camera.h"
#include "graphics/ImageSystem.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "app/ServiceLocator.h"
#include "systems/ModelLoadingSystem.h"
#include "components/Light.h"
#include "components/PointLight.h"
#include "systems/RenderingSystem.h"

/**
 * @class StageSelectScene
 * @brief ワールドセレクトの統合シーンクラス
 */
class StageSelectScene : public IScene {
  public:
    // ステージセレクト画面のカウンタUI設定
    inline static ConfigVar<float> cfg_UICountPosX{"UI.StageSelect.Counter", "CountPosX", 1000.0f, "ステージセレクトカウンタのX座標"};
    inline static ConfigVar<float> cfg_UICountPosY{"UI.StageSelect.Counter", "CountPosY", 500.0f, "ステージセレクトカウンタのY座標"};
    inline static ConfigVar<float> cfg_UICountW{"UI.StageSelect.Counter", "CountWidth", 200.0f, "ステージセレクトカウンタの幅"};
    inline static ConfigVar<float> cfg_UICountH{"UI.StageSelect.Counter", "CountHeight", 40.0f, "ステージセレクトカウンタの高さ"};
    inline static ConfigVar<float> cfg_UICountR{"UI.StageSelect.Counter", "CountColorR", 0.0f, "ステージセレクトカウンタの色 R"};
    inline static ConfigVar<float> cfg_UICountG{"UI.StageSelect.Counter", "CountColorG", 1.0f, "ステージセレクトカウンタの色 G"};
    inline static ConfigVar<float> cfg_UICountB{"UI.StageSelect.Counter", "CountColorB", 1.0f, "ステージセレクトカウンタの色 B"};

    // ワールドごとの最大ステージ数定義
    static constexpr int MAX_STAGES_WORLD1 = 3;
    static constexpr int MAX_STAGES_WORLD2 = 4;
    static constexpr int MAX_STAGES_WORLD3 = 5;
    static constexpr int MAX_STAGES_WORLD4 = 6;
    struct max_stages {
        inline static int Stage_Num;
        inline static int Serial;
    };

    
    static max_stages ms[4] = {
        {1, 3},
        {2, 4},
        {3, 5},
        {4, 6}};


    /**
     * @brief コンストラクタ
     * @param worldNumber ワールド番号 (1-4)
     */
    StageSelectScene(int worldNumber) : worldNumber_(worldNumber) {
        switch (worldNumber_) {
            case 1:
                maxStage_ = MAX_STAGES_WORLD1;
                break;
            case 2:
                maxStage_ = MAX_STAGES_WORLD2;
                break;
            case 3:
                maxStage_ = MAX_STAGES_WORLD3;
                break;
            case 4:
                maxStage_ = MAX_STAGES_WORLD4;
                break;
            default:
                maxStage_ = 3;
                break;
        }
    }

    void OnEnter(World &world) override {
        // GameStatusの確認と生成
        bool hasGameStatus = false;
        world.ForEach<GameStatus>([&](Entity, GameStatus &) { hasGameStatus = true; });
        if (!hasGameStatus) {
            world.Create().With<GameStatus>().Build();
        }

        // StageProgressの確認と生成
        bool hasStageProgress = false;
        world.ForEach<StageProgress>([&](Entity, StageProgress &) { hasStageProgress = true; });
        if (!hasStageProgress) {
            world.Create().With<StageProgress>().Build();
        }

        world.ForEach<StageProgress>([&](Entity, StageProgress &progress) {
            progress.selectStage = std::clamp(progress.selectStage, 1, maxStage_);
            progress.currentStage = std::clamp(progress.currentStage, 1, maxStage_);
        });

        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("[StageSelect] GfxDevice not found");
            return;
        }

        RenderingSystem::GetInstance().Initialize(gfx->Dev());
        RenderingSystem::GetInstance().SetAmbientLight({0.15f, 0.15f, 0.2f}, 1.0f);
        if (!textSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("[StageSelect] TextSystem init failed");
            return;
        }
        if (!imageSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("[StageSelect] ImageSystem init failed");
            return;
        }

        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        // ワールド移動時のステージ番号初期化
        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            stats.worldCount = worldNumber_;
            if (stats.IsWorldBack) {
                stats.selectStage = maxStage_;
                stats.IsWorldBack = false;
            } else {
                stats.selectStage = 1;
            }
        });

        // カメラ初期化
        camera_ = Camera::LookAtLH(
            baseFovY_,
            screenWidth / screenHeight,
            cameraNear_,
            cameraFar_,
            cameraPosition_,
            baseTarget_,
            baseUp_);

        isTransitioning_ = false;
        zoomTimer_ = 0.0f;

        Entity dirLight = world.Create().With<DirectionalLight>().Build();
        if (auto *light = world.TryGet<DirectionalLight>(dirLight)) {
            light->direction = {0.0f, -1.0f, 0.0f};
            light->color = {1.0f, 1.0f, 1.0f, 1.0f};
        }
        ownedEntities_.push_back(dirLight);

        // UIシステムの構築
        Entity canvas = world.Create().With<UICanvas>().Build();
        ownedEntities_.push_back(canvas);

        Entity uiRenderSystem = world.Create().With<UIRenderSystem>().Build();
        if (auto *renderSys = world.TryGet<UIRenderSystem>(uiRenderSystem)) {
            renderSys->SetTextSystem(&textSystem_);
            renderSys->SetImageSystem(&imageSystem_);
            renderSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiRenderSystem);

        Entity uiInteractionSystem = world.Create().With<UIInteractionSystem>().Build();
        if (auto *interactionSys = world.TryGet<UIInteractionSystem>(uiInteractionSystem)) {
            interactionSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiInteractionSystem);

        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        ownedEntities_.push_back(modelLoaderSystem);

        CreateTextNormalFormats();
        CreateStageSelectUI(world);

        // 3Dオブジェクト(Station)の配置
        std::string worldName = "World" + std::to_string(worldNumber_);
        std::string basePath = "Assets/Models/SelectObj_ISS/Station/" + worldName + "/Station";

        if (worldNumber_ == 1) {
            // World1: 3 stages
            CreateObject(world, {5.0f, 0.0f, 0.0f}, basePath + "1.fbx");
            CreateObject(world, {-2.5f, 0.0f, 4.33f}, basePath + "2.fbx");
            CreateObject(world, {-2.5f, 0.0f, -4.33f}, basePath + "3.fbx");
        } else {
            for (int i = 0; i < worldNumber_; i++) {
                std::string fallbackPath = "Assets/Models/SelectObj_ISS/Station/World1/Station3.fbx";
                CreateObject(world, {5.0f, 0.0f, 0.0f}, fallbackPath);
            }
        }
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        // ゲームシーンへの遷移 (Enter / Aボタン)
        bool trigger = input.GetKeyDown(VK_RETURN);
        GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();
        if (!isTransitioning_) {
            if (padsystem &&
                padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start, GamepadSystem::Button_X})) {
                trigger = true;
            }
            if (trigger) {
                isTransitioning_ = true;
                zoomTimer_ = 0.0f;
                DEBUGLOG("StageSelect Camera Zoom Start!");
            }
        } else {
            UpdateCameraZoom(world, deltaTime);
        }

        // ステージ選択処理
        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            bool rightPressed = input.GetKeyDown(VK_RIGHT);
            bool leftPressed = input.GetKeyDown(VK_LEFT);

            if (padsystem) {
                if (padsystem->GetAnyButtonDown({GamepadSystem::Button_DPad_Right, GamepadSystem::Button_B}))
                    rightPressed = true;
                if (padsystem->GetAnyButtonDown({GamepadSystem::Button_DPad_Left, GamepadSystem::Button_X}))
                    leftPressed = true;
            }

            if (rightPressed) {
                if (stats.selectStage < maxStage_) {
                    stats.selectStage++;
                    targetAngle_ -= DirectX::XM_2PI / maxStage_;
                } else if (stats.selectStage == maxStage_) {
                    // 次のワールドへ
                    GoToNextWorld(world);
                }
            }
            if (leftPressed) {
                if (stats.selectStage > 1) {
                    stats.selectStage--;
                    targetAngle_ += DirectX::XM_2PI / maxStage_;
                } else {
                    // 前のワールドへ
                    GoToPrevWorld(world, stats);
                }
            }

            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage_);

            // UIテキスト更新
            if (auto *StageSelectText = world.TryGet<UIText>(StageSelectEntity_)) {
                CreateTextStageNoFormats();
                std::wstringstream ss;
                ss << L"PSS-00" << stats.selectStage;
                StageSelectText->text = ss.str();
            }
        });

        // 回転アニメーション
        currentAngle_ += (targetAngle_ - currentAngle_) * deltaTime * rotateSpeed_;
        world.ForEach<Transform, ObjectPos>([&](Entity, Transform &transform, ObjectPos &pos) {
            float angle = currentAngle_;
            float x = pos.basepos.x;
            float z = pos.basepos.z;
            transform.position.x = x * cosf(angle) - z * sinf(angle);
            transform.position.z = x * sinf(angle) + z * cosf(angle);
        });

        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
            sp.worldCount = worldNumber_;
        });

        world.Tick(deltaTime);
    }

    void OnRender(World &world) override {
        auto &renderer = ServiceLocator::Get<RenderSystem>();

        // トランジション中のオフセット値を取得
        float transitionOffset = 0.0f;
        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            if (manager->IsTransitioning()) {
                TransitionDirection dir = manager->GetTransitionDirection();
                // カメラ回転遷移(Left/Right)時はUIスライドしない
                if (dir != TransitionDirection::Left && dir != TransitionDirection::Right) {
                    transitionOffset = manager->GetTransitionOffset();
                }
            }
        }

        // トランジション中はカメラをオフセットして演出
        Camera renderCamera = camera_;
        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            if (manager->IsTransitioning()) {
                TransitionDirection dir = manager->GetTransitionDirection();
                float offset = manager->GetTransitionOffset();

                if (dir == TransitionDirection::Forward) {
                    // Zoom In Transition: 奥から拡大しながら自然に出てくる (Title -> StageSelect)
                    float progress = manager->GetTransitionProgress();

                    // イージング関数で滑らかに（加速→減速）
                    float easedProgress = progress < 0.5f
                                              ? 4.0f * progress * progress * progress
                                              : 1.0f - powf(-2.0f * progress + 2.0f, 3.0f) / 2.0f;

                    // カメラ位置: 奥(-30)から通常位置(0)へ
                    float startDist = -40.0f;
                    float currentDist = startDist * (1.0f - easedProgress);
                    renderCamera.position.z += currentDist;
                    renderCamera.target.z += currentDist * 0.5f; // ターゲットは半分だけ動かして奥行き感を出す

                    // FOV: 広め(60度)から通常(40度)へ収束 → 膨らんで近づく感覚
                    float startFov = DirectX::XMConvertToRadians(60.0f);
                    float endFov = DirectX::XMConvertToRadians(baseFovY_);
                    renderCamera.fovY = startFov + (endFov - startFov) * easedProgress;

                    // 軽微な上昇演出: 少し下から上がってくる
                    float verticalOffset = -2.0f * (1.0f - easedProgress);
                    renderCamera.position.y += verticalOffset;

                    renderCamera.Update();
                } else if (dir == TransitionDirection::Right || dir == TransitionDirection::Left) {
                    // カメラ回転遷移: カメラが120度横を向く演出
                    float progress = manager->GetTransitionProgress();
                    TransitionPhase phase = manager->GetTransitionPhase();

                    // イージング
                    float easedProgress = progress < 0.5f
                                              ? 4.0f * progress * progress * progress
                                              : 1.0f - powf(-2.0f * progress + 2.0f, 3.0f) / 2.0f;

                    // 回転方向: Right=負方向(-120度), Left=正方向(+120度)
                    float maxAngle = DirectX::XMConvertToRadians(120.0f);
                    if (dir == TransitionDirection::Right) {
                        maxAngle = -maxAngle;
                    }

                    // フェーズに応じた回転角度
                    float currentAngle = 0.0f;
                    if (phase == TransitionPhase::SlideOut) {
                        // 出て行く: 0 → 120度
                        currentAngle = maxAngle * easedProgress;
                    } else {
                        // 入ってくる: 逆側120度 → 0（正面を向く）
                        currentAngle = -maxAngle * (1.0f - easedProgress);
                    }

                    // カメラ位置から原点へのベースベクトルを回転させる
                    // 元のターゲット方向: (0,0,0) - cameraPosition_ = (-8, -1.5, 0)
                    float baseDirX = baseTarget_.x - cameraPosition_.x;
                    float baseDirZ = baseTarget_.z - cameraPosition_.z;

                    // Y軸周りに回転
                    float cosA = cosf(currentAngle);
                    float sinA = sinf(currentAngle);
                    float rotatedDirX = baseDirX * cosA - baseDirZ * sinA;
                    float rotatedDirZ = baseDirX * sinA + baseDirZ * cosA;

                    // 新しいターゲット = カメラ位置 + 回転後の方向
                    renderCamera.target.x = renderCamera.position.x + rotatedDirX;
                    renderCamera.target.z = renderCamera.position.z + rotatedDirZ;

                    renderCamera.Update();
                }
            }
        }

        // UI描画時にもオフセットを適用
        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        float screenWidth = gfx ? static_cast<float>(gfx->Width()) : 1280.0f;
        float uiOffsetX = -transitionOffset * screenWidth;

        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.SetRenderOffset(uiOffsetX);
            sys.Render(world);
            sys.SetRenderOffset(0.0f); // Reset after rendering
        });
        renderer.Render(world, renderCamera);
    }

    void OnExit(World &world) override {
        for (const auto &e : ownedEntities_) {
            DestroyEntityHierarchy(world, e);
        }
        ownedEntities_.clear();

        for (const auto &e : objectOwnedEntities_) {
            DestroyEntityHierarchy(world, e);
        }
        objectOwnedEntities_.clear();

        if (world.IsAlive(StageSelectEntity_)) {
            DestroyEntityHierarchy(world, StageSelectEntity_);
            StageSelectEntity_ = {};
        }

        SOUND_SYS.StopBGM();
        textSystem_.Shutdown();
        imageSystem_.Shutdown();
    }

    void StartFadeInNormal(World &world) {
        StartSpriteFade(world, fadeAnimationEntity_, -1, false);
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

    const Camera &GetCameraSelect() const {
        return camera_;
    }
    int GetWorldNumber() const {
        return worldNumber_;
    }

  private:
    struct ObjectPos {
        DirectX::XMFLOAT3 basepos;
    };

    int worldNumber_;
    int maxStage_;

    Entity fadeAnimationEntity_{};
    Entity StageSelectEntity_{};
    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    Camera camera_{};

    // Camera params
    float baseFovY_ = 40.0f;
    float cameraNear_ = 0.1f;
    float cameraFar_ = 1000.0f;
    DirectX::XMFLOAT3 baseUp_ = {0.0f, 1.0f, 0.0f};
    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 cameraPosition_ = {8.0f, 1.5f, 0.0f};

    // Animation
    float currentAngle_ = 0.0f;
    float targetAngle_ = 0.0f;
    float rotateSpeed_ = 6.0f;

    std::vector<Entity> ownedEntities_{};
    std::vector<Entity> objectOwnedEntities_;

    void DestroyEntityHierarchy(World &world, Entity root) {
        if (!world.IsAlive(root)) {
            return;
        }

        std::vector<Entity> stack;
        stack.push_back(root);

        while (!stack.empty()) {
            Entity current = stack.back();
            stack.pop_back();

            if (auto *hier = world.TryGet<TransformHierarchy>(current)) {
                for (const auto &child : hier->GetChildren()) {
                    if (world.IsAlive(child)) {
                        stack.push_back(child);
                    }
                }
            }

            if (world.IsAlive(current)) {
                world.DestroyEntityWithCause(current, World::Cause::SceneUnload);
            }
        }
    }

    void CreateObject(World &world, const DirectX::XMFLOAT3 &position, const std::string &modelPath) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {0.1f, 0.1f, 0.1f}};
        ObjectPos pos;
        pos.basepos = position;

        PointLight light{
            DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f},
            1.0f,
            10.0f};
        light.SetAttenuation(1.0f, 1.0f, 1.0f);

        auto builder = world.Create().With<Transform>(transform).With<ObjectPos>(pos).With<PointLight>(light);

        if (!modelPath.empty()) {
            builder.With<Model>(modelPath);
        } else {
            // モデルがない場合はCubeを表示（World2-4互換）
            MeshRenderer renderer;
            renderer.meshType = MeshType::Cube;
            renderer.color = {1.0f, 1.0f, 1.0f};
            builder.With<MeshRenderer>(renderer);
        }

        Entity obj = builder.Build();
        objectOwnedEntities_.push_back(obj);
    }

    void GoToNextWorld(World &world) {
        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            std::string nextScene = "World" + std::to_string(worldNumber_ + 1) + "_StageSelect";

            if (worldNumber_ >= 4) {
                // 現状維持（何もしないorループ）
            } else {
                // 次のワールドへ: 右方向にスライド（進む）
                manager->ChangeSceneWithTransition(nextScene.c_str(), world, TransitionDirection::Right);
            }
        }
    }

    void GoToPrevWorld(World &world, StageProgress &stats) {
        if (worldNumber_ <= 1)
            return; // World1以前はない

        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            stats.IsWorldBack = true;
            std::string prevScene = "World" + std::to_string(worldNumber_ - 1) + "_StageSelect";
            // 前のワールドへ: 左方向にスライド（戻る）
            manager->ChangeSceneWithTransition(prevScene.c_str(), world, TransitionDirection::Left);
        }
    }

    void UpdateCameraZoom(World &world, float deltaTime) {
        //遷移の時間
        const float duration = 0.3f;
        zoomTimer_ += deltaTime;

        float progress = std::min(zoomTimer_ / duration, 1.0f);

        //ターゲットに向かってベクトルを計算し、カメラの位置を近づける
        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera_.position);
        DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camera_.target);
        DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(target, pos);

        pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, 0.5f * deltaTime));
        DirectX::XMStoreFloat3(&camera_.position, pos);

        //視野角
        camera_.Zoom(-0.1f * deltaTime);
        camera_.Update();
        //シーン遷移
        if (progress >= 1.0f) {
            if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                manager->ChangeScene("Game", world);
            }
        }
    }

    bool isTransitioning_ = false;
    float zoomTimer_ = 0.0f;

    // UI creation methods defined in StageUI.cpp
    void CreateTextStageNoFormats();
    void CreateTextNormalFormats();
    void CreateStageSelectUI(World &world);
};
