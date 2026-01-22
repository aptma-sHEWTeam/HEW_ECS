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
#include <filesystem>
#include <cassert>
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
#include "components/ModelComponent.h"
#include "components/PointLight.h"
#include "components/TransformHierarchy.h"
#include "systems/RenderingSystem.h"
#include "graphics/TextureManager.h"

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
    inline static ConfigVar<std::string> cfg_SkyboxModelPath{"StageSelect.Skybox", "ModelPath", "Assets/Textures/Skybox/skybox.fbx", "ステージセレクト: Skybox モデルパス"};
    inline static ConfigVar<std::string> cfg_SkyboxTexturePath{"StageSelect.Skybox", "TexturePath", "Assets/Textures/Skybox/Sky_Box.png", "ステージセレクト: Skybox テクスチャパス"};
    inline static ConfigVar<std::string> cfg_SkyboxWorld1TexturePath{"StageSelect.Skybox.World1", "TexturePath", "", "ステージセレクト: World1 Skybox テクスチャパス"};
    inline static ConfigVar<std::string> cfg_SkyboxWorld2TexturePath{"StageSelect.Skybox.World2", "TexturePath", "", "ステージセレクト: World2 Skybox テクスチャパス"};
    inline static ConfigVar<std::string> cfg_SkyboxWorld3TexturePath{"StageSelect.Skybox.World3", "TexturePath", "", "ステージセレクト: World3 Skybox テクスチャパス"};
    inline static ConfigVar<std::string> cfg_SkyboxWorld4TexturePath{"StageSelect.Skybox.World4", "TexturePath", "", "ステージセレクト: World4 Skybox テクスチャパス"};
    inline static ConfigVar<float> cfg_SkyboxScale{"StageSelect.Skybox", "Scale", 200.0f, "ステージセレクト: Skybox スケール"};

    // ワールドごとの最大ステージ数定義
    static constexpr int MAX_STAGES_WORLD1 = 3;
    static constexpr int MAX_STAGES_WORLD2 = 4;
    static constexpr int MAX_STAGES_WORLD3 = 5;
    static constexpr int MAX_STAGES_WORLD4 = 6;
    struct max_stages {
        int Stage_Num;
        int Serial;
    };

    inline static max_stages ms[4] = {{MAX_STAGES_WORLD1, 1}, {MAX_STAGES_WORLD2, 2}, {MAX_STAGES_WORLD3, 3}, {MAX_STAGES_WORLD4, 4}};

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

        skyboxTexture_ = TextureManager::INVALID_TEXTURE;
        skyboxTextureApplied_ = false;
        EnsureSkyboxTextureLoaded();

        CreateSkybox(world);

#if defined(_DEBUG)
        static bool skyboxScaleTestsRan = false;
        if (!skyboxScaleTestsRan) {
            RunSkyboxScaleTests();
            skyboxScaleTestsRan = true;
        }
#endif

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
            int stageCount = maxStage_;
            if (worldNumber_ >= 1 && worldNumber_ <= 4) {
                int idx = worldNumber_ - 1;
                if (ms[idx].Stage_Num > 0) {
                    stageCount = ms[idx].Stage_Num;
                }
            }

            std::string fallbackPath = "Assets/Models/SelectObj_ISS/Station/World1/Station3.fbx";
            const float radius = 5.0f;
            for (int i = 0; i < stageCount; ++i) {
                // place stations evenly around a circle
                float angle = static_cast<float>(i) * DirectX::XM_2PI / static_cast<float>(stageCount);
                float x = cosf(angle) * radius;
                float z = sinf(angle) * radius;

                std::string modelPath = basePath + std::to_string(i + 1) + ".fbx";
                std::error_code ec;
                if (!std::filesystem::exists(modelPath, ec) || ec) {
                    modelPath = fallbackPath;
                }

                CreateObject(world, {x, 0.0f, z}, modelPath);
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
                padsystem->GetAnyButtonDown({GamepadSystem::Button_X})) {
                trigger = true;
                SOUND_SYS.PlaySE(cfg_EnterMP3Pass);
            }
            if (trigger) {
                isTransitioning_ = true;
                zoomTimer_ = 0.0f;
                DEBUGLOG("StageSelect Camera Zoom Start!");
            }
        } else {
            UpdateCameraZoom(world, deltaTime);
        }

        UpdateSkyboxTransform(world);
        UpdateSkyboxTexture(world);

        // ステージ選択処理
        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            bool rightPressed = input.GetKeyDown(VK_RIGHT);
            bool leftPressed = input.GetKeyDown(VK_LEFT);

            if (padsystem) {
                float gx = padsystem->GetLeftStickX();
                bool dpadRightNow = padsystem->GetButton(padsystem->Button_DPad_Right);
                bool dpadLeftNow = padsystem->GetButton(padsystem->Button_DPad_Left);

                const float STICK_THRESHOLD = 0.8f;
                bool stickRightNow = gx > STICK_THRESHOLD;
                bool stickLeftNow = gx < -STICK_THRESHOLD;

                //スティックでの切り替え
                if (stickRightNow && !stickRightPrev_) {
                    rightPressed = true;
                }
                if (stickLeftNow && !stickLeftPrev_) {
                    leftPressed = true;
                }
                //ボタンでの切り替え
                if (dpadRightNow && !dpadRightPrev_) {
                    rightPressed = true;
                }
                if (dpadLeftNow && !dpadLeftPrev_) {
                    leftPressed = true;
                }

                stickRightPrev_ = stickRightNow;
                stickLeftPrev_ = stickLeftNow;
                dpadRightPrev_ = dpadRightNow;
                dpadLeftPrev_ = dpadLeftNow;
            }

           

            if (rightPressed) {
                if (stats.selectStage < maxStage_) {
                    stats.selectStage++;
                    targetAngle_ -= DirectX::XM_2PI / maxStage_;
                    SOUND_SYS.PlaySE(cfg_SelectMP3Pass);
                } else if (stats.selectStage == maxStage_) {
                    // 次のワールドへ
                    stats.selectStage = 1;
                    GoToNextWorld(world);
                }
            }
            if (leftPressed) {
                if (stats.selectStage > 1) {
                    stats.selectStage--;
                    targetAngle_ += DirectX::XM_2PI / maxStage_;
                    SOUND_SYS.PlaySE(cfg_SelectMP3Pass);
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

        renderer.Render(world, renderCamera);
        if (gfx) {
            gfx->Ctx()->OMSetDepthStencilState(nullptr, 0);
        }
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.SetRenderOffset(uiOffsetX);
            sys.Render(world);
            sys.SetRenderOffset(0.0f);
        });

        SOUND_SYS.PlayBGM(cfg_TitleMP3Pass);
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
    float cameraFar_ = 10000.0f;
    DirectX::XMFLOAT3 baseUp_ = {0.0f, 1.0f, 0.0f};
    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 cameraPosition_ = {8.0f, 1.5f, 0.0f};

    // Animation
    float currentAngle_ = 0.0f;
    float targetAngle_ = 0.0f;
    float rotateSpeed_ = 6.0f;

    std::vector<Entity> ownedEntities_{};
    std::vector<Entity> objectOwnedEntities_;
    Entity skyboxEntity_{};
    TextureManager::TextureHandle skyboxTexture_ = TextureManager::INVALID_TEXTURE;
    bool skyboxTextureApplied_ = false;

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

    void CreateSkybox(World &world) {
        const std::string modelPath = cfg_SkyboxModelPath.Get();
        if (modelPath.empty()) {
            DEBUGLOG_ERROR("[StageSelect] Skybox model path is empty");
            return;
        }
        const float scale = SanitizeSkyboxScale(cfg_SkyboxScale.Get());
        const float yawDeg = GetCameraYawDeg(camera_);
        Transform transform{
            {camera_.position.x, camera_.position.y, camera_.position.z},
            {0.0f, yawDeg, 0.0f},
            {scale, scale, scale}};
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
            const float scale = SanitizeSkyboxScale(cfg_SkyboxScale.Get());
            t->position = camera_.position;
            t->rotation = {0.0f, GetCameraYawDeg(camera_), 0.0f};
            t->scale = {scale, scale, scale};
        }
    }

    static float GetCameraYawDeg(const Camera &cam) {
        using namespace DirectX;
        const XMVECTOR pos = XMLoadFloat3(&cam.position);
        const XMVECTOR target = XMLoadFloat3(&cam.target);
        XMVECTOR dir = XMVectorSubtract(target, pos);
        dir = XMVector3Normalize(dir);

        XMFLOAT3 d{};
        XMStoreFloat3(&d, dir);
        const float yawRad = atan2f(d.x, d.z);
        return DirectX::XMConvertToDegrees(yawRad);
    }

    std::string ResolveSkyboxTexturePath() const {
        {
            ConfigVar<std::string> titleSkyboxTexture{"Title.Skybox", "TexturePath", "", ""};
            const std::string titlePath = titleSkyboxTexture.Get();
            if (!titlePath.empty()) {
                return titlePath;
            }
        }

        std::string worldPath;
        switch (worldNumber_) {
            case 1:
                worldPath = cfg_SkyboxWorld1TexturePath.Get();
                break;
            case 2:
                worldPath = cfg_SkyboxWorld2TexturePath.Get();
                break;
            case 3:
                worldPath = cfg_SkyboxWorld3TexturePath.Get();
                break;
            case 4:
                worldPath = cfg_SkyboxWorld4TexturePath.Get();
                break;
            default:
                break;
        }
        if (!worldPath.empty()) {
            return worldPath;
        }
        return cfg_SkyboxTexturePath.Get();
    }

    bool EnsureSkyboxTextureLoaded() {
        if (skyboxTexture_ != TextureManager::INVALID_TEXTURE) {
            return true;
        }
        const std::string texturePath = ResolveSkyboxTexturePath();
        if (!IsSkyboxTexturePathValid(texturePath)) {
            DEBUGLOG_ERROR("[StageSelect] Skybox texture path is empty");
            return false;
        }
        std::error_code ec;
        if (!std::filesystem::exists(texturePath, ec) || ec) {
            DEBUGLOG_ERROR("[StageSelect] Skybox texture not found: " + texturePath);
            return false;
        }
        auto &texMgr = ServiceLocator::Get<TextureManager>();
        skyboxTexture_ = texMgr.LoadFromFile(texturePath.c_str());
        if (skyboxTexture_ == TextureManager::INVALID_TEXTURE) {
            DEBUGLOG_ERROR("[StageSelect] Failed to load skybox texture: " + texturePath);
            return false;
        }
        return true;
    }

    bool ApplySkyboxTextureRecursive(World &world, Entity entity) {
        bool applied = false;
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

    static bool IsSkyboxTexturePathValid(const std::string &path) {
        return !path.empty();
    }

    static float SanitizeSkyboxScale(float scale) {
        return std::max(scale, 0.1f);
    }

#if defined(_DEBUG)
    void RunSkyboxScaleTests() {
        assert(!IsSkyboxTexturePathValid(""));
        assert(IsSkyboxTexturePathValid("Assets/Textures/Skybox/Sky_Box.png"));
        assert(SanitizeSkyboxScale(1.0f) == 1.0f);
        assert(SanitizeSkyboxScale(0.0f) == 0.1f);
        assert(SanitizeSkyboxScale(-2.0f) == 0.1f);
    }
#endif

    bool isTransitioning_ = false;
    float zoomTimer_ = 0.0f;

    bool stickRightPrev_ = false;
    bool stickLeftPrev_ = false;
    bool dpadRightPrev_ = false;
    bool dpadLeftPrev_ = false;


    // UI creation methods defined in StageUI.cpp
    void CreateTextStageNoFormats();
    void CreateTextNormalFormats();
    void CreateStageSelectUI(World &world);
};
