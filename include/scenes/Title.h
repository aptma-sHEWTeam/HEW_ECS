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
#include "systems/ModelLoadingSystem.h"


#include "components/MeshRenderer.h"
#include "systems/RenderingSystem.h"
#include "components/Light.h"

#include "graphics/TextSystem.h"
#include "graphics/Camera.h"
#include "graphics/ImageSystem.h"
#include "graphics/ModelLoader.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "app/ServiceLocator.h"
//#include "Game.h"
#include "scenes/StageConfig.h"
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"
#include "components/Animator.h"


/**
 * @class TitleScene
 * @brief ワールドセレクト2のシーン
 * 
 * 2026/01/15 
 * 　亀多　3Dオブジェクト表示
 */
class TitleScene : public IScene {
  public:
    inline static ConfigVar<float> cfg_WindowPosX{"Title.Window", "PosX", 0.0f, "タイトル: Window 位置X"};
    inline static ConfigVar<float> cfg_WindowPosY{"Title.Window", "PosY", 0.0f, "タイトル: Window 位置Y"};
    inline static ConfigVar<float> cfg_WindowPosZ{"Title.Window", "PosZ", 0.0f, "タイトル: Window 位置Z"};
    inline static ConfigVar<float> cfg_WindowScaleX{"Title.Window", "ScaleX", 0.650000f, "タイトル: Window スケールX"};
    inline static ConfigVar<float> cfg_WindowScaleY{"Title.Window", "ScaleY", 0.650000f, "タイトル: Window スケールY"};
    inline static ConfigVar<float> cfg_WindowScaleZ{"Title.Window", "ScaleZ", 0.650000f, "タイトル: Window スケールZ"};
    inline static ConfigVar<float> cfg_WindowRotX{"Title.Window", "RotX", 0.0f, "タイトル: Window 回転X"};
    inline static ConfigVar<float> cfg_WindowRotY{"Title.Window", "RotY", 90.0f, "タイトル: Window 回転Y"};
    inline static ConfigVar<float> cfg_WindowRotZ{"Title.Window", "RotZ", 0.0f, "タイトル: Window 回転Z"};

    inline static ConfigVar<float> cfg_PlayerPosX{"Title.Player", "PosX", 4.0f, "タイトル: Player 位置X"};
    inline static ConfigVar<float> cfg_PlayerPosY{"Title.Player", "PosY", -1.0f, "タイトル: Player 位置Y"};
    inline static ConfigVar<float> cfg_PlayerPosZ{"Title.Player", "PosZ", 1.0f, "タイトル: Player 位置Z"};
    inline static ConfigVar<float> cfg_PlayerScaleX{"Title.Player", "ScaleX", 3.0f, "タイトル: Player スケールX"};
    inline static ConfigVar<float> cfg_PlayerScaleY{"Title.Player", "ScaleY", 5.0f, "タイトル: Player スケールY"};
    inline static ConfigVar<float> cfg_PlayerScaleZ{"Title.Player", "ScaleZ", 1.0f, "タイトル: Player スケールZ"};
    inline static ConfigVar<float> cfg_PlayerRotX{"Title.Player", "RotX", 0.0f, "タイトル: Player 回転X"};
    inline static ConfigVar<float> cfg_PlayerRotY{"Title.Player", "RotY", 0.0f, "タイトル: Player 回転Y"};
    inline static ConfigVar<float> cfg_PlayerRotZ{"Title.Player", "RotZ", 0.0f, "タイトル: Player 回転Z"};

    Camera GetCameraTitle() const { return camera_; }

    void OnEnter(World &world) override {
        // 既存実装そのまま
        bool hasGameStatus = false;
        world.ForEach<GameStatus>([&](Entity, GameStatus &) { hasGameStatus = true; });
        if (!hasGameStatus) {
            world.Create().With<GameStatus>().Build();
        }
        Entity modelLoaderSystem = world.Create().With<ModelLoadingSystem>().Build();
        world.Add<SceneOwnedTag>(modelLoaderSystem);
        ownedEntities_.push_back(modelLoaderSystem);

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

        //3Dレンダリングの初期化
        RenderingSystem::GetInstance().Initialize(gfx->Dev());

        try {
            auto &renderer = ServiceLocator::Get<RenderSystem>();
            renderer.Init();
        } catch (...) {
            DEBUGLOG_ERROR("[TitleScene] Failed to init RenderSystem");
        }
        

        float screenWidth = static_cast<float>(gfx->Width());
        float screenHeight = static_cast<float>(gfx->Height());

        // UICanvas & Systems
        Entity canvas = world.Create().With<UICanvas>().With<SceneOwnedTag>().Build();
        ownedEntities_.push_back(canvas);

        Entity uiRenderSystem = world.Create().With<UIRenderSystem>().With<SceneOwnedTag>().Build();
        if (auto *renderSys = world.TryGet<UIRenderSystem>(uiRenderSystem)) {
            renderSys->SetTextSystem(&textSystem_);
            renderSys->SetImageSystem(&imageSystem_);
            renderSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiRenderSystem);

        Entity uiInteractionSystem = world.Create().With<UIInteractionSystem>().With<SceneOwnedTag>().Build();
        if (auto *interactionSys = world.TryGet<UIInteractionSystem>(uiInteractionSystem)) {
            interactionSys->SetScreenSize(screenWidth, screenHeight);
        }
        ownedEntities_.push_back(uiInteractionSystem);

        Entity dirLight = world.Create().With<DirectionalLight>().With<SceneOwnedTag>().Build();
        if (auto *light = world.TryGet<DirectionalLight>(dirLight)) {
            light->direction = {0.0f, -1.0f, 0.0f};
            light->color = {1.0f, 1.0f, 1.0f, 1.0f};
        }
        ownedEntities_.push_back(dirLight);
        
        CreateWindows(world);
        CreatePlayer(world);
        CreateTextNormalFormats();
        CreateTitleSelectUI(world);
       

        float aspect = static_cast<float>(gfx->Width()) / gfx->Height();
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4, aspect, 0.1f, 1000.0f,
            {0, 0, -10}, {0, 0, 0}, {0, 1, 0});

        isTransitioning_ = false;
        zoomTimer_ = 0.0f;
        
    }
      //プレイヤーの仮描画
      void CreatePlayer(World &world) {

          float s = cfg_PlayerScale;
          DirectX::XMFLOAT3 pos{cfg_PlayerPosX.Get(), cfg_PlayerPosY.Get(), cfg_PlayerPosZ.Get()};
          Transform transform{
              {pos}, {cfg_PlayerRotX.Get(), cfg_PlayerRotY.Get(), cfg_PlayerRotZ.Get()}, {cfg_PlayerScaleX.Get(), cfg_PlayerScaleY.Get(), cfg_PlayerScaleZ.Get()}};
          MeshRenderer mrPlayer;
          mrPlayer.meshType = MeshType::Sphere;
          mrPlayer.color = DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};

          const std::string modelPath = AnimationConfig::Paths::PlayerModel;
          std::vector<std::string> animPaths = {
              AnimationConfig::Paths::PlayerAnimFry,
          };
          std::vector<std::string> animAliases = {
              AnimationConfig::Clips::PlayerIdle,
          };

          std::vector<ModelPrefabNode> nodes = ModelLoader::LoadModel(modelPath);
          auto clips = AnimationTools::LoadClipsFromFiles(animPaths, {AnimationConfig::Paths::PlayerAnimFallback}, animAliases);

          ModelPrefabNode *targetNode = nullptr;
          for (auto &node : nodes) {
              if (node.hasMesh) {
                  targetNode = &node;
                  break;
              }
          }
          if (!targetNode && !nodes.empty())
              targetNode = &nodes[0];

          Entity player = world.CreateEntity();
          world.Add<Transform>(player, transform);

          if (targetNode && targetNode->hasMesh) {
              world.Add<ModelComponent>(player, targetNode->component);
              if (targetNode->component.isSkinned) {
                  std::string defaultClip = AnimationConfig::Clips::PlayerDefault;
                  const bool defaultExists = std::any_of(clips.begin(), clips.end(), [&](const auto &c) { return c.name == defaultClip; });
                  if (!defaultExists && !clips.empty()) {
                      defaultClip = clips.front().name;
                  }
                  AnimationTools::InitAnimator(world, player, clips, defaultClip);
              }
          } else {
              world.Add<Model>(player, modelPath);
          }

          world.Add<SceneOwnedTag>(player);
          world.Add<PlayerTag>(player);

          playerEntity_ = player;
          ownedEntities_.push_back(player);

          // CreatePlayer(world);
      }
    

      void CreateWindows(World& world) 
      {
        //確認用オブジェクト
        DirectX::XMFLOAT3 objPos{cfg_WindowPosX.Get(), cfg_WindowPosY.Get(), cfg_WindowPosZ.Get()};
        Transform transform{
            {objPos}, {cfg_WindowRotX.Get(), cfg_WindowRotY.Get(), cfg_WindowRotZ.Get()}, {cfg_WindowScaleX.Get(), cfg_WindowScaleY.Get(), cfg_WindowScaleZ.Get()}}; 
        MeshRenderer meshrenderer;
        meshrenderer.meshType = MeshType::Cube;
        meshrenderer.color = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
        objectEntity_ = world.Create()
                                   .With<Transform>(transform)
                                   .With<Model>("Assets/Models/StageObj/Window/window.fbx")
                                   .With<SceneOwnedTag>()
                                   .Build();
        ownedEntities_.push_back(objectEntity_);
      }
    
      void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        // 既存実装そのまま（前と同じ内容）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        if (world.IsAlive(objectEntity_)) {
            if (auto *t = world.TryGet<Transform>(objectEntity_)) {
                t->position = {cfg_WindowPosX.Get(), cfg_WindowPosY.Get(), cfg_WindowPosZ.Get()};
                t->rotation = {cfg_WindowRotX.Get(), cfg_WindowRotY.Get(), cfg_WindowRotZ.Get()};
                t->scale = {cfg_WindowScaleX.Get(), cfg_WindowScaleY.Get(), cfg_WindowScaleZ.Get()};
            }
        }

        if (world.IsAlive(playerEntity_)) {
            if (auto *t = world.TryGet<Transform>(playerEntity_)) {
                t->position = {cfg_PlayerPosX.Get(), cfg_PlayerPosY.Get(), cfg_PlayerPosZ.Get()};
                t->rotation = {cfg_PlayerRotX.Get(), cfg_PlayerRotY.Get(), cfg_PlayerRotZ.Get()};
                t->scale = {cfg_PlayerScaleX.Get(), cfg_PlayerScaleY.Get(), cfg_PlayerScaleZ.Get()};
            }
        }

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

       void OnRender(World &world) override {

           auto *gfx = ServiceLocator::TryGet<GfxDevice>();
           if (!gfx)
               return;
           try {
               auto &renderer = ServiceLocator::Get<RenderSystem>();
               //3Dオブジェクトを描画
               renderer.Render(world, camera_);

           } catch (...) {
               DEBUGLOG_ERROR("[TitkeScene] Failed to get RenderSystem from ServiceLocator");
           }
          world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
              MeshRenderer renderer;
              sys.Render(world);
          });
      }

    void OnExit(World &world) override {
        world.ForEach<SceneOwnedTag>([&](Entity e, SceneOwnedTag &) {
            world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
        });
        world.DestroyAllEntitiesImmediate(World::Cause::SceneUnload);
        for (const auto &e : ownedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

          RenderingSystem::GetInstance().Shutdown();//3Dレンダリング

          textSystem_.Shutdown();
          imageSystem_.Shutdown();
      }
   
  private:
    struct SceneOwnedTag : IComponent {};

       void UpdateCameraZoom(World &world, float deltaTime) {
         const float duration = 2.0f;
         zoomTimer_ += deltaTime;

         float progress = std::min(zoomTimer_ / duration, 1.0f);

         DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera_.position);
         DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camera_.target);
         DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(target, pos);

         pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, 0.5f * deltaTime));
         DirectX::XMStoreFloat3(&camera_.position, pos);

         camera_.Zoom(-0.1f * deltaTime);
         camera_.Update();

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

       std::vector<Entity> ownedEntities_{};

       Entity playerEntity_{};
       Entity objectEntity_{};

       TextSystem textSystem_{};
       ImageSystem imageSystem_{};
       Camera camera_{};
};
