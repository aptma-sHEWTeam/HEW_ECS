/**
 * @file StageSelect.h
 * @brief ワールドセレクトシーン
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
 * @class WorldSlectScene
 * @brief 3DゲームとUIを統合したシーン
 */
class WorldSelectScene : public IScene {
  public:
      void OnEnter(World& world) override{
       
    }

      void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        if (input.GetKeyDown(VK_RETURN)) {
            DEBUGLOG("Enter pressed!");
            if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                maneger->ChangeScene("StageSelect", world);
            }
        }

          GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();
        if (padsystem) {
            if (padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start, GamepadSystem::Button_X})) {
                DEBUGLOG("Enter pressed!");
                if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                    maneger->ChangeScene("StageSelect", world);
                }
            }
        }

        world.Tick(deltaTime);
      }

      void OnRender(World &world) {
      }

      void OnExit(World &world) override{
      }
  
  private:

};
