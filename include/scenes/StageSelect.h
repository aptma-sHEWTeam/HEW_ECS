/**
 * @file StageSelect.h
 * @brief セレクトシーン
 * @author 立山悠朔
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include <vector>
#include <algorithm>

#include "graphics/Effect.h"

#include "config/ConfigVar.h"
#include "components/UIComponents.h"
#include "components/StageComponents.h"
#include "graphics/TextSystem.h"
#include "graphics/ImageSystem.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "app/ServiceLocator.h"

/**
 * @class StageSlectScene
 * @brief 3DゲームとUIを統合したシーン
 */
class StageSlectScene : public IScene {
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
        maxStage_ = GetAvailableStageCount();
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
        CreateTextFormats();

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

        // Create UI once
        CreateStageSelectUI(world);
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        // 既存実装そのまま（前と同じ内容）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        world.Tick(deltaTime);

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

        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            const int maxStage = maxStage_;

            if (input.GetKeyDown(VK_RIGHT) && stats.selectStage < maxStage) {
                stats.selectStage++;
            }
            if (input.GetKeyDown(VK_LEFT) && stats.selectStage > 1) {
                stats.selectStage--;
            }

            GamepadSystem *padsystem2 = ServiceLocator::TryGet<GamepadSystem>();
            if (padsystem2) {
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Right, GamepadSystem::Button_B})) {
                    stats.selectStage++;
                }
                if (padsystem2->GetAnyButtonDown({GamepadSystem::Button_DPad_Left, GamepadSystem::Button_X})) {
                    stats.selectStage--;
                }
            }

            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage);

            if (auto *StageSelectText = world.TryGet<UIText>(StageSelectEntity_)) {
                std::wstringstream ss;
                ss << L"PSS-00" << stats.selectStage;
                StageSelectText->text = ss.str();
            }
        });
    }

    void OnRender(World &world) override {
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
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
    void CreateTextFormats();

    void CreateStageSelectUI(World &world);


    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    std::vector<Entity> ownedEntities_{};
    Entity StageSelectEntity_{};
    int maxStage_ = 1;
};
