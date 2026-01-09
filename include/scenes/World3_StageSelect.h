/**
 * @file StageSelect.h
 * @brief ワールド3セレクトシーン
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
 * @class World3_StageSelectScene
 * @brief ワールドセレクト2のシーン
 */
class World3_StageSelectScene : public IScene {
  public:
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
            stats.worldCount = 3;
        });

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
        CreateStageSelectUI(world);
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
                } else if (stats.selectStage == maxStage_) {
                    if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                        manager->ChangeScene("World4_StageSelect", world);
                    }
                }
            }
            if (input.GetKeyDown(VK_LEFT)) {
                if (stats.selectStage > 1) {
                    stats.selectStage--;
                } else {
                    if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                        manager->ChangeScene("World2_StageSelect", world);
                    }
                }
            }

            GamepadSystem *padsystem2 = ServiceLocator::TryGet<GamepadSystem>();
            if (padsystem2) {
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Right, GamepadSystem::Button_B})) {
                    if (stats.selectStage < maxStage_) {
                        stats.selectStage++;
                    } else if (stats.selectStage == maxStage_) {
                        if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                            maneger->ChangeScene("World4_StageSelect", world);
                        }
                    }
                }
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Left, GamepadSystem::Button_X})) {
                    if (stats.selectStage > 1) {
                        stats.selectStage--;
                    } else {
                        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                            manager->ChangeScene("World2_StageSelect", world);
                        }
                    }
                }

                stats.selectStage = std::clamp(stats.selectStage, 1, maxStage);

                if (auto *StageSelectText = world.TryGet<UIText>(StageSelectEntity_)) {
                    CreateTextStageNoFormats();
                    std::wstringstream ss;
                    ss << L"PSS-00" << stats.selectStage;
                    StageSelectText->text = ss.str();
                }
            }
        });

        world.Tick(deltaTime);
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

        if (world.IsAlive(StageSelectEntity_)) {
            world.DestroyEntityWithCause(StageSelectEntity_, World::Cause::SceneUnload);
            StageSelectEntity_ = {};
        }
        textSystem_.Shutdown();
        imageSystem_.Shutdown();
    }


  private:
    void CreateTextNormalFormats();
    void CreateTextStageNoFormats();
    void CreateStageSelectUI(World &world);

    Entity StageSelectEntity_{};

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};

    std::vector<Entity> ownedEntities_{};

    int maxStage_ = 8;
};