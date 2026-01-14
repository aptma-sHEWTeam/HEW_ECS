/**
 * @file StageSelect.h
 * @brief ワールド1セレクトシーン
 * @author 立山悠朔
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include <vector>
#include <algorithm>
#include<DirectXMath.h>

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

/**
 * @class World1_StageSelectScene
 * @brief ワールドセレクト1のシーン
 */
class World1_StageSlectScene : public IScene {
  public:
    // ステージセレクト画面のカウンタUI設定
    inline static ConfigVar<float> cfg_UICountPosX{"UI.StageSelect.Counter", "CountPosX", 1000.0f, "ステージセレクトカウンタのX座標"};
    inline static ConfigVar<float> cfg_UICountPosY{"UI.StageSelect.Counter", "CountPosY", 500.0f, "ステージセレクトカウンタのY座標"};
    inline static ConfigVar<float> cfg_UICountW{"UI.StageSelect.Counter", "CountWidth", 200.0f, "ステージセレクトカウンタの幅"};
    inline static ConfigVar<float> cfg_UICountH{"UI.StageSelect.Counter", "CountHeight", 40.0f, "ステージセレクトカウンタの高さ"};
    inline static ConfigVar<float> cfg_UICountR{"UI.StageSelect.Counter", "CountColorR", 0.0f, "ステージセレクトカウンタの色 R"};
    inline static ConfigVar<float> cfg_UICountG{"UI.StageSelect.Counter", "CountColorG", 1.0f, "ステージセレクトカウンタの色 G"};
    inline static ConfigVar<float> cfg_UICountB{"UI.StageSelect.Counter", "CountColorB", 1.0f, "ステージセレクトカウンタの色 B"};

    void OnEnter(World &world) override {
        // 既存実装そのまま
        bool hasGameStatus = false;
        world.ForEach<GameStatus>([&](Entity, GameStatus &) { hasGameStatus = true; });
        if (!hasGameStatus) {
            world.Create().With<GameStatus>().Build();
        }

        bool hasStageProgress = false;
        world.ForEach<StageProgress>([&](Entity, StageProgress &) { hasStageProgress = true; });
        if (!hasStageProgress) {
            world.Create().With<StageProgress>().Build();
        }
        //maxStage_ = GetAvailableStageCount(); //今はオブジェクトを8個表示したいから消してるステージの読み込みをしっかり出来たら直す
        world.ForEach<StageProgress>([&](Entity, StageProgress &progress) {
            progress.selectStage = std::clamp(progress.selectStage, 1, maxStage_);
            progress.currentStage = std::clamp(progress.currentStage, 1, maxStage_);
        });

        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("[StageSelect] GfxDevice not found");
            return;
        }
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

         world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            stats.selectStage = 0;
            stats.worldCount = 1;
        });
       
        //カメラを初期化
        camera_ = Camera::LookAtLH(
            baseFovY_,
            screenWidth / screenHeight,
            cameraNear_,
            cameraFar_,
            cameraPosition_,
            baseTarget_,
            baseUp_
        );

        // UICanvas & Systems
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

         CreateTextNormalFormats();
        // Create UI once
        CreateStageSelectUI(world);

        CreateObject(world, {5.0f, 0.0f, 0.0f});   //右
        CreateObject(world, {3.0f, 0.0f, 3.0f});   //右上
        CreateObject(world, {0.0f,0.0f,5.0f});     //上
        CreateObject(world, {-3.0f, 0.0f, 3.0f});  //左上
        CreateObject(world, {-5.0f, 0.0f, 0.0f});  //左
        CreateObject(world, {-3.0f, 0.0f, -3.0f}); //左下
        CreateObject(world, {0.0f, 0.0f,-5.0f});   //下
        CreateObject(world, {3.0f, 0.0f, -3.0f});  //右下
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        // 既存実装そのまま（前と同じ内容）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        if (input.GetKeyDown(VK_RETURN)) {
            DEBUGLOG("Enter pressed!");
            if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                maneger->ChangeScene("Game", world);
            }
        }

        GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();
        if (padsystem) {
            if (padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start, GamepadSystem::Button_X})) {
                DEBUGLOG("Enter pressed!");
                if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                    maneger->ChangeScene("Game", world);
                }
            }
        }

        const int maxStage = maxStage_;

        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            if (input.GetKeyDown(VK_RIGHT)) {
                if (stats.selectStage < maxStage_) {
                    stats.selectStage++;
                    targetAngle_ += DirectX::XM_2PI / maxStage_;
                } else if (stats.selectStage == maxStage_) {
                    if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                        manager->ChangeScene("World2_StageSelect", world);
                    }
                }
            }
            if (input.GetKeyDown(VK_LEFT) && stats.selectStage > 1) {
                stats.selectStage--;
                targetAngle_ -= DirectX::XM_2PI / maxStage_;
            }

            GamepadSystem *padsystem2 = ServiceLocator::TryGet<GamepadSystem>();
            if (padsystem2) {
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Right, GamepadSystem::Button_B})) {
                    if (stats.selectStage < maxStage_) {
                        stats.selectStage++;
                        targetAngle_+= DirectX::XM_2PI / maxStage_;
                    } else if (stats.selectStage == maxStage_) {
                        if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                            maneger->ChangeScene("World2_StageSelect", world);
                        }
                    }
                    
                }
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Left, GamepadSystem::Button_X})) {
                    stats.selectStage--;
                    targetAngle_ -= DirectX::XM_2PI / maxStage_;
                }
            }

            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage);

            if (auto *StageSelectText = world.TryGet<UIText>(StageSelectEntity_)) {
                CreateTextStageNoFormats();
                std::wstringstream ss;
                ss << L"PSS-00" << stats.selectStage;
                StageSelectText->text = ss.str();
            }
        });
       
        currentAngle_ += (targetAngle_ - currentAngle_) * deltaTime * rotateSpeed_;
        world.ForEach<Transform, ObjectPos>([&](Entity, Transform &transform, ObjectPos &pos) {
            float angle = currentAngle_;
            float x = pos.basepos.x;
            float z = pos.basepos.z;
            transform.position.x = x * cosf(angle) - z * sinf(angle);
            transform.position.z = x * sinf(angle) + z * cosf(angle);
        });



        world.Tick(deltaTime);

    }

    void OnRender(World &world) override {
        auto &renderer = ServiceLocator::Get<RenderSystem>(); 
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            MeshRenderer renderer;
            sys.Render(world);
        });
        renderer.Render(world, camera_);
    }

    void OnExit(World &world) override {
        for (const auto &e : ownedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();
        for (const auto &e : objectOwnedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        objectOwnedEntities_.clear();
        if (world.IsAlive(StageSelectEntity_)) {
            world.DestroyEntityWithCause(StageSelectEntity_, World::Cause::SceneUnload);
            StageSelectEntity_ = {};
        }

        SOUND_SYS.StopBGM();
        textSystem_.Shutdown();
        imageSystem_.Shutdown();

     
    }

     const Camera &GetCameraSelect() const {return camera_;}

  private:
      struct ObjectPos {
        DirectX::XMFLOAT3 basepos;
     };
    void CreateObject(World &world, const DirectX::XMFLOAT3 &position) {
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
        MeshRenderer renderer;
        renderer.meshType = MeshType::Cube;
        renderer.color = {1.0f, 1.0f, 1.0f};
        ObjectPos pos;
        pos.basepos = position;

        Entity Object = world.Create()
                            .With<Transform>(transform)
                            .With<MeshRenderer>(renderer)
                            .With<ObjectPos>(pos)
                            .Build();

        objectOwnedEntities_.push_back(Object);
    }

    void CreateTextStageNoFormats();
    void CreateTextNormalFormats();
    void CreateStageSelectUI(World &world);

    Entity StageSelectEntity_{};

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    Camera camera_{};
    float baseFovY_ = 60.0f;
    float cameraNear_ = 0.1f;
    float cameraFar_ = 1000.0f;
    DirectX::XMFLOAT3 baseUp_ = {0.0f, 1.0f, 0.0f};
    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 cameraPosition_ = {6.0f, -1.5f, 0.0f};
    DirectX::XMFLOAT3 currentTarget_ = {0.0f, 0.0f, 0.0f};
    float currentAngle_ = 0.0f;
    float targetAngle_ = 0.0f;
    float rotateSpeed_ = 6.0f; 

    std::vector<Entity> ownedEntities_{};
    std::vector<Entity> objectOwnedEntities_;

    int maxStage_ = 8;
};
