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
    inline static ConfigVar<float> cfg_UICountPosX{"UI.StageSelect.Counter", "CountPosX", 20.0f};
    inline static ConfigVar<float> cfg_UICountPosY{"UI.StageSelect.Counter", "CountPosY", 170.0f};
    inline static ConfigVar<float> cfg_UICountW{"UI.StageSelect.Counter", "CountWidth", 200.0f};
    inline static ConfigVar<float> cfg_UICountH{"UI.StageSelect.Counter", "CountHeight", 40.0f};
    inline static ConfigVar<float> cfg_UICountR{"UI.StageSelect.Counter", "CountColorR", 0.0f};
    inline static ConfigVar<float> cfg_UICountG{"UI.StageSelect.Counter", "CountColorG", 1.0f};
    inline static ConfigVar<float> cfg_UICountB{"UI.StageSelect.Counter", "CountColorB", 1.0f};

    void OnEnter(World &world) override {
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
        world.ForEach<GamepadSystem>([&](Entity e, GamepadSystem &padsystem) {
            // Wire input to UI interaction system once
            world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
                if (!sys.input_) {
                    sys.input_ = &input;
                }
            });

            // Tick更新
            world.Tick(deltaTime);

            //enterを押したらシーン移動
            if (input.GetKeyDown(VK_RETURN)) {
                DEBUGLOG("Enter pressed!");
                auto *maneger = ServiceLocator::TryGet<SceneManager>();
                maneger->ChangeScene("Game", world);
            }
            if (padsystem.GetButtonDown(padsystem.Button_A)) {
                DEBUGLOG("Enter pressed!");
                auto *maneger = ServiceLocator::TryGet<SceneManager>();
                maneger->ChangeScene("Game", world);
            }

            world.ForEach<StageProgress>([&](Entity e, StageProgress &stats) {
                const int maxStage = GetAvailableStageCount();

                if (input.GetKeyDown(VK_RIGHT) && stats.selectStage < maxStage) {
                    stats.selectStage++;
                }
                if (input.GetKeyDown(VK_LEFT) && stats.selectStage > 1) {
                    stats.selectStage--;
                }

                if (padsystem.GetButtonDown(padsystem.Button_B)) {
                    stats.selectStage++;
                }
                if (padsystem.GetButton(padsystem.Button_X)) {
                    stats.selectStage--;
                }

                stats.selectStage = std::clamp(stats.selectStage, 1, maxStage);

                if (auto *StageSelectText = world.TryGet<UIText>(StageSelectEntity_)) {
                    std::wstringstream ss;
                    ss << L"StageNo : " << stats.selectStage << L"/" << maxStage;
                    StageSelectText->text = ss.str();
                }
            });
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
    void CreateTextFormats() {
        TextSystem::TextFormat hud;
        hud.fontSize = 24.0f;
        hud.fontFamily = L"メイリオ";
        hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        textSystem_.CreateTextFormat("hud", hud);
    }

    void CreateStageSelectUI(World &world) {
        UITransform CountTransform;
        CountTransform.position = {cfg_UICountPosX, cfg_UICountPosY};
        CountTransform.size = {cfg_UICountW, cfg_UICountH};
        CountTransform.anchor = {0.0f, 0.0f};
        CountTransform.pivot = {0.0f, 0.0f};

        UIText CountText{L"Stage Select: Press Enter"};
        CountText.color = {cfg_UICountR, cfg_UICountG, cfg_UICountB, 1.0f};
        CountText.formatId = "hud";

        Entity e = world.Create()
                       .With<UITransform>(CountTransform)
                       .With<UIText>(CountText)
                       .Build();

        StageSelectEntity_ = e;
        ownedEntities_.push_back(e);
    }

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    std::vector<Entity> ownedEntities_{};
    Entity StageSelectEntity_{};
};
