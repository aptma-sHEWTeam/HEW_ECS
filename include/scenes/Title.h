/**
 * @file title.h
 * @brief タイトルシーン
 * @author 立山悠朔
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include <vector>
#include <algorithm>
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

/**
 * @class TitleScene
 * @brief ワールドセレクト2のシーン
 */
class TitleScene : public IScene {
  public:
    void OnEnter(World &world) override {
        // 既存実装そのまま
        bool hasGameStatus = false;
        world.ForEach<GameStatus>([&](Entity, GameStatus &) { hasGameStatus = true; });
        if (!hasGameStatus) {
            world.Create().With<GameStatus>().Build();
        }

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
        CreateTitleSelectUI(world);

        float aspect = static_cast<float>(gfx->Width()) / gfx->Height();
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4, aspect, 0.1f, 1000.0f,
            {0, 0, -10}, {0, 0, 0}, {0, 1, 0});

        isTransitioning_ = false;
        zoomTimer_ = 0.0f;
    }
    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        // 既存実装そのまま（前と同じ内容）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        if (!isTransitioning_) {
            bool trigger = input.GetKeyDown(VK_RETURN);
            GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();

            if (padsystem && padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start, GamepadSystem::Button_X})) {
                DEBUGLOG("Enter pressed!");
                trigger = true;
            }
            if (trigger) {
                isTransitioning_ = true;
                DEBUGLOG("Camera Zoom Start!");
            }
        } else {
            UpdateCameraZoom(world, deltaTime);
        }
        /*if (input.GetKeyDown(VK_RETURN)) {
            DEBUGLOG("Enter pressed!");
            if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                maneger->ChangeSceneWithTransition("World1_StageSelect", world, TransitionDirection::Forward);
            }
        }

        
        if (padsystem) {
            if (padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start, GamepadSystem::Button_X})) {
                DEBUGLOG("Enter pressed!");
                if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                    maneger->ChangeSceneWithTransition("World1_StageSelect", world, TransitionDirection::Forward);
                }
            }
        }*/
    }

    void OnRender(World &world) {
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            MeshRenderer renderer;
            sys.Render(world);
        });
    }

    void OnExit(World &world) override {
        for (const auto &e : ownedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        textSystem_.Shutdown();
        imageSystem_.Shutdown();
    }

  private:
    void UpdateCameraZoom(World &world, float deltaTime) {
        //遷移の時間
        const float duration = 2.0f;
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

        //UIのフェード
        for (auto &entity : ownedEntities_) {
            //左スライド(移動する速度設定)
            if (auto *transform = world.TryGet<UITransform>(entity)) {
                //transform->position.x -= 800.0f * deltaTime; // Disabled for zoom transition
            }
            if (auto *image = world.TryGet<UIImage>(entity)) {
                image->opacity = 1.0f - progress;
            }
            if (auto *text = world.TryGet<UIText>(entity)) {
                text->color.w = 1.0f - progress;
            }
        }

        //シーン遷移
        if (progress >= 1.0f) {
            if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                manager->ChangeSceneWithTransition("World1_StageSelect", world, TransitionDirection::Forward);
            }
        }
    }

    void CreateTextNormalFormats();
    void CreateTitleSelectUI(World &world);

    bool isTransitioning_ = false;
    float zoomTimer_ = 0.0f;

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    Camera camera_{};

    std::vector<Entity> ownedEntities_{};
};
