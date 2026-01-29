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
#include <cassert>
#include <filesystem>
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
#include "graphics/TextureManager.h"
#include "input/GamepadSystem.h"
#include "systems/UISystem.h"
#include "app/ServiceLocator.h"
#include "app/ResourceManager.h"
//#include "Game.h"
#include "scenes/StageConfig.h"
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"
#include "components/Animator.h"
#include "components/TransformHierarchy.h"

namespace
{
inline std::wstring Utf8toWide(const std::string& src)
{
    if (src.empty())
    {
        return std::wstring();
    }

    const int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
    if (len <= 0)
    {
        return std::wstring();
    }

    std::wstring dst(static_cast<size_t>(len), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &dst[0], len);
    if (written <= 0)
    {
        return std::wstring();
    }
    if (!dst.empty() && dst.back() == L'\0')
    {
        dst.pop_back();
    }
    return dst;
}
}

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

    inline static ConfigVar<float> cfg_PlayerPosX{"Title.Player", "PosX", 2.0f, "タイトル: Player 位置X"};
    inline static ConfigVar<float> cfg_PlayerPosY{"Title.Player", "PosY", -5.0f, "タイトル: Player 位置Y"};
    inline static ConfigVar<float> cfg_PlayerPosZ{"Title.Player", "PosZ", -2.0f, "タイトル: Player 位置Z"};
    inline static ConfigVar<float> cfg_PlayerScaleX{"Title.Player", "ScaleX", 5.0f, "タイトル: Player スケールX"};
    inline static ConfigVar<float> cfg_PlayerScaleY{"Title.Player", "ScaleY", 5.0f, "タイトル: Player スケールY"};
    inline static ConfigVar<float> cfg_PlayerScaleZ{"Title.Player", "ScaleZ", 5.0f, "タイトル: Player スケールZ"};
    inline static ConfigVar<float> cfg_PlayerRotX{"Title.Player", "RotX", 0.0f, "タイトル: Player 回転X"};
    inline static ConfigVar<float> cfg_PlayerRotY{"Title.Player", "RotY", 0.0f, "タイトル: Player 回転Y"};
    inline static ConfigVar<float> cfg_PlayerRotZ{"Title.Player", "RotZ", 0.0f, "タイトル: Player 回転Z"};
    inline static ConfigVar<std::string> cfg_SkyboxModelPath{"Title.Skybox", "ModelPath", "Assets/Textures/Skybox/skybox.fbx", "タイトル: Skybox モデルパス"};
    inline static ConfigVar<std::string> cfg_SkyboxTexturePath{"Title.Skybox", "TexturePath", "Assets/Textures/Skybox/Sky_Box.png", "タイトル: Skybox テクスチャパス"};
    inline static ConfigVar<float> cfg_SkyboxScale{"Title.Skybox", "Scale", 200.0f, "タイトル: Skybox スケール"};
    inline static ConfigVar<float> cfg_SkyboxSpeed{"Skybox", "Speed", 0.05f, "スカイボックスの回転速度(rad/sec)"};
    inline static ConfigVar<float> cfg_CameraBobAmplitude{"Title.Camera", "BobAmplitude", 0.2f, "Title: Camera bob amplitude"};
    inline static ConfigVar<float> cfg_CameraBobSpeed{"Title.Camera", "BobSpeed", 1.0f, "Title: Camera bob speed (rad/sec)"};

    // 壁見た目
    inline static ConfigVar<float> cfg_WallPosX{"Title.Wall", "PosX", 0.0f, "壁: Window 位置X"};
    inline static ConfigVar<float> cfg_WallPosY{"Title.Wall", "PosY", 1.0f, "壁: Window 位置Y"};
    inline static ConfigVar<float> cfg_WallPosZ{"Title.Wall", "PosZ", 0.0f, "壁: Window 位置Z"};
    inline static ConfigVar<float> cfg_WallScaleX{"Title.Wall", "ScaleX", 5.0000f, "壁: Wall スケールX"};
    inline static ConfigVar<float> cfg_WallScaleY{"Title.Wall", "ScaleY", 5.0000f, "壁: Wall スケールY"};
    inline static ConfigVar<float> cfg_WallScaleZ{"Title.Wall", "ScaleZ", 5.0000f, "壁: Wall スケールZ"};
    inline static ConfigVar<float> cfg_WallRotX{"Title.Wall", "RotX",  0.0f, "壁: Wall 回転X"};
    inline static ConfigVar<float> cfg_WallRotY{"Title.Wall", "RotY", 90.0f, "壁: Wall 回転Y"};
    inline static ConfigVar<float> cfg_WallRotZ{"Title.Wall", "RotZ",  0.0f, "壁: Wall 回転Z"};
   
    //壁の複数化の設定
    inline static ConfigVar<int> cfg_WallColumns{"Title.Wall", "Columns",11, "壁: 横に並べる個数（列数）"};
    inline static ConfigVar<int> cfg_WallRows{"Title.Wall", "Rows", 6, "壁: 縦に並べる個数（行数）"};
    inline static ConfigVar<int> cfg_WallThicknessRows{"Title.Wall", "ThicknessRows", 2, "壁: 枠(上下)の太さ(ブロック単位）"};
    inline static ConfigVar<int> cfg_WallThicknessCols{"Title.Wall", "ThicknessCols", 4, "壁: 枠(左右)の太さ(ブロック単位）"};
    inline static ConfigVar<float> cfg_WallSpacingX{"Title.Wall", "SpacingX", 0.6500f, "壁: 行間隔（X方向）"};//ブロックのサイズに合わせて変更
    inline static ConfigVar<float> cfg_WallRowSpacingZ{"Title.Wall", "RowSpacingZ",2.0f, "壁: 列間隔（Z方向）"};//ブロックのサイズに合わせて変更

    inline static ConfigVar<int> cfg_WallDirX{"Title.Wall", "DirX", 1, "壁: 列方向の増加向き (1 または -1)"};
    inline static ConfigVar<int> cfg_WallDirZ{"Title.Wall", "DirZ", -1, "壁: 行方向の増加向き (1 または -1)"};

    //フェード関連
    inline static ConfigVar<float> cfg_FadeSizeW{"Title.Fade", "Width", 1280.0f, "タイトル: フェードUIの幅"};
    inline static ConfigVar<float> cfg_FadeSizeH{"Title.Fade", "Height", 720.0f, "タイトル: フェードUIの高さ"};
    inline static ConfigVar<float> cfg_FadeSecondsPerFrame{"Title.Fade", "SecondsPerFrame", 0.1f, "タイトル: フェードアニメ1フレーム時間(秒)"};
    inline static ConfigVar<std::string> cfg_FadeTexturePath{"Title.Fade", "TexturePath", "./Assets/Textures/Fade/tex_fade.png", "タイトル: フェードテクスチャ"};

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
            light->direction = {0.3f, -1.0f, 0.3f};
            light->color = {1.0f, 1.0f, 1.0f, 1.0f};
        }
        ownedEntities_.push_back(dirLight);

        CreateWindows(world);
        GenerateWallGridTransforms(cfg_WallColumns.Get(),cfg_WallRows.Get());
       CreateWalls(world);

        CreatePlayer(world);
        CreateTextNormalFormats();
        CreateTitleSelectUI(world);
       
        //カメラの詳細設定
        float aspect = static_cast<float>(gfx->Width()) / gfx->Height();
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4, aspect, 0.1f, 10000.0f,
            {0, 0, -5}, {0, 0, 1}, {0, 1, 0});
        cameraBobPhase_ = 0.0f;
        cameraBobOffsetY_ = 0.0f;

        CreateSkybox(world);

#if defined(_DEBUG)
        static bool skyboxScaleTestsRan = false;
        if (!skyboxScaleTestsRan) {
            RunSkyboxScaleTests();
            skyboxScaleTestsRan = true;
        }
        static bool cameraBobTestsRan = false;
        if (!cameraBobTestsRan) {
            RunCameraBobTests();
            cameraBobTestsRan = true;
        }
#endif

       // DEBUGLOG("cfg_WallPosX=" + std::to_string(cfg_WallPosX.Get()) + " transforms=" + std::to_string(wallTransforms_.size()) + " entities=" + std::to_string(wallEntities_.size()));
        //サウンドの初期化
        SOUND_SYS.Init();

        isTransitioning_ = false;
        zoomTimer_ = 0.0f;
        isUiVisible_ = true;
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
            AnimationConfig::Paths::PlayerAnimTitle,
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
        world.Add<TransformHierarchy>(player);

        int targetIndex = -1;
        if (targetNode) {
            targetIndex = static_cast<int>(targetNode - &nodes[0]);
        }

        if (targetNode && targetNode->hasMesh) {
            world.Add<ModelComponent>(player, targetNode->component);
            std::string defaultClip = AnimationConfig::Clips::PlayerDefault;
            if (targetNode->component.isSkinned) {
                const bool defaultExists = std::any_of(clips.begin(), clips.end(), [&](const auto &c) { return c.name == defaultClip; });
                if (!defaultExists && !clips.empty()) {
                    defaultClip = clips.front().name;
                }
                AnimationTools::InitAnimator(world, player, clips, defaultClip);
            }

            std::vector<Entity> created(nodes.size());
            if (targetIndex >= 0 && static_cast<size_t>(targetIndex) < created.size()) {
                created[targetIndex] = player;
            }
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (static_cast<int>(i) == targetIndex) continue;
                const auto &node = nodes[i];
                Entity child = world.CreateEntity();
                world.Add<Transform>(child, Transform{node.translation, node.rotationDeg, node.scale});
                world.Add<TransformHierarchy>(child);
                if (node.hasMesh && node.component.indexCount > 0) {
                    world.Add<ModelComponent>(child, node.component);
                    if (node.component.isSkinned && !clips.empty()) {
                        AnimationTools::InitAnimator(world, child, clips, defaultClip);
                    }
                }
                created[i] = child;
            }

            for (size_t i = 0; i < nodes.size(); ++i) {
                Entity child = (static_cast<int>(i) == targetIndex) ? player : created[i];
                if (!world.IsAlive(child)) continue;
                int pIdx = nodes[i].parentIndex;
                Entity parent = player;
                if (pIdx >= 0 && static_cast<size_t>(pIdx) < created.size()) {
                    parent = (pIdx == targetIndex) ? player : created[pIdx];
                }
                auto *ch = world.TryGet<TransformHierarchy>(child);
                auto *ph = world.TryGet<TransformHierarchy>(parent);
                if (ch && ph && child != parent) {
                    ch->SetParent(parent);
                    ph->AddChild(child);
                }
            }
        } else {
            world.Add<Model>(player, modelPath);
        }

        world.Add<SceneOwnedTag>(player);
        world.Add<PlayerTag>(player);

        playerEntity_ = player;
        ownedEntities_.push_back(player);

        UITransform FadeAnimation;
        FadeAnimation.position = {0.0f, 0.0f};
        FadeAnimation.size = {cfg_FadeSizeW.Get(), cfg_FadeSizeH.Get()};
        FadeAnimation.anchor = {0.0f, 0.0f};
        FadeAnimation.pivot = {0.0f, 0.0f};

        const std::wstring fadePath = Utf8toWide(cfg_FadeTexturePath.Get());
        UIImage fade{fadePath};
        fade.opacity = 1.0f;
        fade.keepAspect = false;
        fade.overlay = true;

        SpriteSheetDesc fadeDesc = SpriteSheetDesc::Grid(
            AnimationConfig::UI::FadeFrames,
            AnimationConfig::UI::FadeCols,
            cfg_FadeSecondsPerFrame.Get(),
            /*loop*/ false);
        fadeDesc.playOnStart = false;

        Entity fadeOutAnimation = world.Create()
                                      .With<UITransform>(FadeAnimation)
                                      .With<UIImage>(fade)
                                      .Build();
        AnimationTools::AddSpriteSheet(world, fadeOutAnimation, fadeDesc);

        ownedEntities_.push_back(fadeOutAnimation);
        fadeEntity_ = fadeOutAnimation;
  
        isFading = false;
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

    void CreateWalls(World &world) {

         //既に作成されている場合は何もしない
         if (!wallEntities_.empty())
             return;

         if (!wallTransforms_.empty()) {
             size_t need = wallTransforms_.size();
             for (size_t i = wallEntities_.size(); i < need; ++i) {
                 const Transform &tf = wallTransforms_[i];

                 Entity e = world.Create()
                                .With<Transform>(tf)
                                .With<Model>("Assets/Models/StageObj/Wall/obj_wall.fbx")
                                .With<SceneOwnedTag>()
                                .Build();
                 wallEntities_.push_back(e);
                 ownedEntities_.push_back(e);
             }
             while (wallEntities_.size() > need)
             {//余分なエンティティがあったら破棄
                 Entity e = wallEntities_.back();
                 wallEntities_.pop_back();

                 ownedEntities_.erase(std::remove(ownedEntities_.begin(), ownedEntities_.end(), e), ownedEntities_.end());
                 if (world.IsAlive(e)) {
                     world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
                 }
             }
             return;

             int count = std::max(0, cfg_WallColumns.Get());
             for (int i = 0; i < count; ++i) {
                 DirectX::XMFLOAT3 wallPos {
                     cfg_WallPosX.Get() + static_cast<float>(i),
                         cfg_WallPosY.Get(),
                         cfg_WallPosZ.Get()
                 };
                 Transform transform{
                     {wallPos}, {cfg_WallRotX.Get(), cfg_WallRotY.Get(), cfg_WallRotZ.Get()}, {cfg_WallScaleX.Get(), cfg_WallScaleY.Get() ,cfg_WallScaleZ.Get()}};
                     
                 
             }
         }
         int count = std::max(0, cfg_WallColumns.Get());
         for (int i = 0; i < count; ++i) {
             DirectX::XMFLOAT3 wallPos{
                 cfg_WallPosX.Get() + static_cast<float>(i) * cfg_WallScaleX.Get(),
                 cfg_WallPosY.Get()  ,
                 cfg_WallPosZ.Get() + static_cast<float>(i) * cfg_WallScaleZ.Get()};
             Transform transform{
                 {wallPos}, {cfg_WallRotX.Get(), cfg_WallRotY.Get(), cfg_WallRotZ.Get()}, {cfg_WallScaleX.Get(), cfg_WallScaleY.Get(), cfg_WallScaleZ.Get()}};
             MeshRenderer mrWalls;
             mrWalls.meshType = MeshType::Cube;
             mrWalls.color = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
             Entity e = world.Create()
                            .With<Transform>(transform)
                            .With<Model>("Assets/Models/StageObj/Wall/obj_wall.fbx")
                            .With<SceneOwnedTag>()
                            .Build();
             wallEntities_.push_back(e);
             ownedEntities_.push_back(e);
         }
      
      }
      
    void SetWallTransforms(const std::vector<Transform> &transforms){
             wallTransforms_ = transforms;
         }

    void GenerateWallGridTransforms(int columns , int rows = -1) {
          wallTransforms_.clear();
          if (columns <= 0 || rows <= 0) {
              return;
          }
          if (rows <= 0) {
              rows = cfg_WallRows.Get();
          }

          const float spacingX = cfg_WallSpacingX.Get();         //幅
          const float rowSpacingZ = cfg_WallRowSpacingZ.Get();   //高さ
        //  const float rowSpacingY = cfg_WallRowSpacingY.Get(); //行間隔
          const float baseX = cfg_WallPosX.Get();
          const float baseY = cfg_WallPosY.Get() ;
          const float baseZ = cfg_WallPosZ.Get();
          const int thicknessRows = std::max(1, cfg_WallThicknessRows.Get());//上下
          const int thicknessCols = std::max(1, cfg_WallThicknessCols.Get());//左右
          const int dirX = std::clamp(cfg_WallDirX.Get(), -1, 1);
          const int dirZ = std::clamp(cfg_WallDirZ.Get(), -1, 1);
          const float halfCols = (static_cast<float>(columns) - 1.0f) * 0.5f;
          const float halfRows = (static_cast<float>(rows) - 1.0f) * 0.5f;

          //const float scaleUpWall = 5.0f;



          for (int r = 0; r < rows; ++r) {//縦
              for (int c =0; c < columns; ++c) {//横

                  const bool isTop    = (r < thicknessRows + 1.0f);//微調整したいときに数値を変更
                  const bool isBottom = (r >= rows - thicknessRows);
                  const bool isLeft   = (c < thicknessCols); 
                  const bool isRight  = (c >= columns - thicknessCols);

                  const bool isEdge = isTop || isBottom || isLeft || isRight;
                  if (!isEdge) {//空白用
                      continue;
                  }
                     
                 /* const float x = baseX + static_cast<float>(c) * spacingX * static_cast<float>(dirX);
                  const float y = baseY + static_cast<float>(r) * rowSpacingZ * static_cast<float>(dirZ);
                  const float z = baseZ - 0.3f; */
                  const float x = baseX + ((static_cast<float>(c) - halfCols) * spacingX * static_cast<float>(dirX) );
                  const float y = baseY + ((static_cast<float>(r)  - halfRows)* rowSpacingZ * static_cast<float>(dirZ));
                  const float z = baseZ - 0.3f; //微調整(奥行き)したいときに数値を変更
                      DirectX::XMFLOAT3 pos{x,y,z };
                      Transform t{
                          {pos}, {cfg_WallRotX.Get(), cfg_WallRotY.Get(), cfg_WallRotZ.Get()}, {cfg_WallScaleX.Get(), cfg_WallScaleY.Get(), cfg_WallScaleZ.Get()}};
                      wallTransforms_.push_back(t);
                  
              }
          }
      }

    void CreateSkybox(World &world) {
        const std::string modelPath = cfg_SkyboxModelPath.Get();
        if (modelPath.empty()) {
            DEBUGLOG_ERROR("[TitleScene] Skybox model path is empty");
            return;
        }
        const float scale = SanitizeSkyboxScale(cfg_SkyboxScale.Get());
        const float yawDeg = SkyboxRotationToDegrees(skyboxRotation_);
        Transform transform{
            camera_.position,
            {0.0f, yawDeg, 0.0f},
            {scale, scale, scale}};
        skyboxEntity_ = world.Create()
                            .With<Transform>(transform)
                            .With<Model>(modelPath)
                            .With<SceneOwnedTag>()
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
            t->rotation = {0.0f, SkyboxRotationToDegrees(skyboxRotation_), 0.0f};
            t->scale = {scale, scale, scale};
        }
    }

    void UpdateSkyboxRotation(float dt) {
        skyboxRotation_ += cfg_SkyboxSpeed.Get() * dt;
        if (skyboxRotation_ > DirectX::XM_2PI) {
            skyboxRotation_ -= DirectX::XM_2PI;
        }
    }

    bool EnsureSkyboxTextureLoaded() {
        if (skyboxTexture_ != TextureManager::INVALID_TEXTURE) {
            return true;
        }
        const std::string texturePath = cfg_SkyboxTexturePath.Get();
        if (!IsSkyboxTexturePathValid(texturePath)) {
            DEBUGLOG_ERROR("[TitleScene] Skybox texture path is empty");
            return false;
        }
        std::error_code ec;
        if (!std::filesystem::exists(texturePath, ec) || ec) {
            DEBUGLOG_ERROR("[TitleScene] Skybox texture not found: " + texturePath);
            return false;
        }
        auto &texMgr = ServiceLocator::Get<TextureManager>();
        skyboxTexture_ = texMgr.LoadFromFile(texturePath.c_str());
        if (skyboxTexture_ == TextureManager::INVALID_TEXTURE) {
            DEBUGLOG_ERROR("[TitleScene] Failed to load skybox texture: " + texturePath);
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


    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {

        // 既存実装そのまま（前と同じ内容）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        bool upPressd = input.GetKeyDown(VK_UP);
        bool downPressd = input.GetKeyDown(VK_DOWN);

        if (isFading) {
            world.Tick(deltaTime);
            if (auto *anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_)) {
                if (anim->isFinished) 
                {
                    if (auto *manager = ServiceLocator::TryGet<SceneManager>()) 
                    {
                        manager->ChangeSceneWithTransition("World1_StageSelect", world, TransitionDirection::Forward);
                    }
                }
            }
            return;
        }

        GamepadSystem *pad = ServiceLocator::TryGet<GamepadSystem>();
        if (pad) {
            float pady = pad->GetLeftStickY();
            bool dpadUp = pad->GetButton(GamepadSystem::Button_DPad_Up);
            bool dpadDown = pad->GetButton(GamepadSystem::Button_DPad_Down);
        
            if (pady > 0.8f && !stickUpPrev_)upPressd = true;
            if (pady < -0.8f && !stickDownPrev_)downPressd = true;
            if (dpadUp && !dpadUpPrev_)upPressd = true;
            if (dpadDown && !dpadDownPrev_)downPressd = true;

            stickUpPrev_ = (pady > 0.8f);
            stickDownPrev_ = (pady < -0.8f);
            dpadUpPrev_ = dpadUp;
            dpadDownPrev_ = dpadDown;
        }

        if (upPressd) {
            currentSelect = (currentSelect - 1 + 3) % 3;
            SOUND_SYS.PlaySE(cfg_SelectMP3Pass,true);
        }
        if (downPressd) {
            currentSelect = (currentSelect + 1) % 3;
            SOUND_SYS.PlaySE(cfg_SelectMP3Pass,true);
        }

        for (int i = 0; i < 3; ++i) {
            if (auto *hoverImg = world.TryGet<UIImage>(menuEntity_[i])) {
                hoverImg->opacity = (i == currentSelect) ? 1.0f : 0.0f;
            }
            if (auto *baseImg = world.TryGet<UIImage>(baseMenuEntity_[i])) {
                baseImg->opacity = (i == currentSelect) ? 0.0f : 1.0f;
            }
        }

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

        // 複数壁の更新
        for (size_t i = 0; i < wallEntities_.size(); ++i) {
            Entity e = wallEntities_[i];
            if (!world.IsAlive(e))
                continue;
            if (auto *t = world.TryGet<Transform>(e)) {
                if (i < wallTransforms_.size()) {
                    t->position = wallTransforms_[i].position;
                    t->rotation = wallTransforms_[i].rotation;
                 //   t->scale = wallTransforms_[i].scale;
                    t->scale = {cfg_WallScaleX.Get(),
                                cfg_WallScaleY.Get(),
                                cfg_WallScaleZ.Get()};
                } else {
                    int colIndex = static_cast<int>(i % std::max(1, cfg_WallColumns.Get()));
                    int rowIndex = static_cast<int>(i / std::max(1, cfg_WallColumns.Get()));

                    t->position = {
                        cfg_WallPosX.Get()  + static_cast<float>(colIndex) * cfg_WallScaleX.Get(),
                        cfg_WallPosY.Get() + static_cast<float>(rowIndex) * cfg_WallScaleY.Get(),
                        cfg_WallPosZ.Get()};
                    t->rotation = {cfg_WallRotX.Get(), cfg_WallRotY.Get(), cfg_WallRotZ.Get()};
                    t->scale = {cfg_WallScaleX.Get() , cfg_WallScaleY.Get(), cfg_WallScaleZ.Get()};
                }
            }
        }


        if (!isTransitioning_) {
            bool trigger = input.GetKeyDown(VK_RETURN);
            
            GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();


            if (padsystem && padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start})) {
                switch (currentSelect) {
                    case Start:
                        SOUND_SYS.PlaySE(cfg_EnterMP3Pass,false);
                        StageSave::Load(); 
                        DEBUGLOG("Enter pressed!");
                        trigger = true;
                        break;
                    case Restart:
                        StageSave::Delete(); 
                        StageSave::Load();   
                        break;
                    case Exit:
                        DEBUGLOG("Game End!");
                        PostQuitMessage(0);
                        break;

                }
            }
            if (trigger) {
               
                isTransitioning_ = true;
                isUiVisible_ = false;
                DEBUGLOG("Camera Zoom Start!");
            }
        } else {
            UpdateCameraZoom(world, deltaTime);
        }

        UpdateCameraBob(deltaTime);
        UpdateSkyboxRotation(deltaTime);
        UpdateSkyboxTransform(world);
        UpdateSkyboxTexture(world);

         world.Tick(deltaTime);

        //サウンドの音量設定の更新
        SOUND_SYS.UpdateVolume();

        /*if (input.GetKeyDown(VK_RETURN)) {
            DEBUGLOG("Enter pressed!");
            if (auto *maneger = ServiceLocator::TryGet<SceneManager>()) {
                maneger->ChangeSceneWithTransition("World1_StageSelect", world, TransitionDirection::Forward);
            }
        }

        
        if (padsystem) {
            if (padsystem->GetAnyButtonDown({GamepadSystem::Button_A, GamepadSystem::Button_Start})) {
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
        if (isUiVisible_) {
            world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
                MeshRenderer renderer;
                sys.Render(world);
            });
        }

        SOUND_SYS.PlayBGM(cfg_TitleMP3Pass);
    }

    void OnExit(World &world) override {
        world.ForEach<SceneOwnedTag>([&](Entity e, SceneOwnedTag &) {
            world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
        });
        world.DestroyAllEntitiesImmediate(World::Cause::SceneUnload);
        if (auto *resMgr = ServiceLocator::TryGet<ResourceManager>()) {
            resMgr->Clear();
        }
        if (auto *texMgr = ServiceLocator::TryGet<TextureManager>()) {
            texMgr->Shutdown();
            texMgr->Init(ServiceLocator::Get<GfxDevice>());
        }
        ModelLoader::ClearTextureCache();
        for (const auto &e : ownedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        ownedEntities_.clear();

        isFading = false;

        RenderingSystem::GetInstance().Shutdown(); //3Dレンダリング

        textSystem_.Shutdown();
        imageSystem_.Shutdown();

        skyboxTexture_ = TextureManager::INVALID_TEXTURE;
        skyboxTextureApplied_ = false;
    }



  private:
    struct SceneOwnedTag : IComponent {};

    void UpdateCameraZoom(World &world, float deltaTime) {
        const float duration = 1.5f;
        zoomTimer_ += deltaTime;

        float progress = std::min(zoomTimer_ / duration, 1.0f);

        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera_.position);
        DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camera_.target);
        DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(target, pos);

         pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, 3.5f * deltaTime)); //カメラ移動速度調整
         DirectX::XMStoreFloat3(&camera_.position, pos );
       /* pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, 1.5f * deltaTime));
        DirectX::XMStoreFloat3(&camera_.position, pos);*/

         //カメラズーム
        camera_.Zoom(-0.1f * deltaTime);
        camera_.Update();

        if (progress >= 1.0f) {
            StartFadeInNormal(world);
            isFading = true;
            return;
        }
    }

    void UpdateCameraBob(float deltaTime) {
        const float amplitude = std::max(0.0f, cfg_CameraBobAmplitude.Get());
        const float speed = std::max(0.0f, cfg_CameraBobSpeed.Get());

        if (amplitude <= 0.0f || speed <= 0.0f) {
            if (cameraBobOffsetY_ != 0.0f) {
                camera_.position.y -= cameraBobOffsetY_;
                camera_.target.y -= cameraBobOffsetY_;
                cameraBobOffsetY_ = 0.0f;
                camera_.Update();
            }
            return;
        }

        camera_.position.y -= cameraBobOffsetY_;
        camera_.target.y -= cameraBobOffsetY_;
        cameraBobPhase_ += speed * deltaTime;
        if (cameraBobPhase_ > DirectX::XM_2PI) {
            cameraBobPhase_ -= DirectX::XM_2PI;
        }
        cameraBobOffsetY_ = CameraBobOffset(cameraBobPhase_, amplitude);
        camera_.position.y += cameraBobOffsetY_;
        camera_.target.y += cameraBobOffsetY_;
        camera_.Update();
    }

    void StartFadeInNormal(World &world) {
        StartSpriteFade(world, fadeEntity_, 1, false);
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

     enum TitleSelect {
        Start = 0,
        Restart = 1,
        Exit = 2
     };

    void CreateTextNormalFormats();
    void CreateTitleSelectUI(World &world);

    static bool IsSkyboxTexturePathValid(const std::string &path) {
        return !path.empty();
    }

    static float SanitizeSkyboxScale(float scale) {
        return std::max(scale, 0.1f);
    }

    static float SkyboxRotationToDegrees(float rotationRad) {
        return DirectX::XMConvertToDegrees(rotationRad);
    }

    static float CameraBobOffset(float phaseRad, float amplitude) {
        return std::sin(phaseRad) * amplitude;
    }

#if defined(_DEBUG)
    void RunSkyboxScaleTests() {
        assert(!IsSkyboxTexturePathValid(""));
        assert(IsSkyboxTexturePathValid("Assets/Textures/Skybox/Sky_Box.png"));
        assert(SanitizeSkyboxScale(1.0f) == 1.0f);
        assert(SanitizeSkyboxScale(0.0f) == 0.1f);
        assert(SanitizeSkyboxScale(-2.0f) == 0.1f);
        assert(std::abs(SkyboxRotationToDegrees(DirectX::XM_PIDIV2) - 90.0f) < 1e-3f);
    }

    void RunCameraBobTests() {
        assert(std::abs(CameraBobOffset(0.0f, 1.0f)) < 1e-6f);
        assert(std::abs(CameraBobOffset(DirectX::XM_PIDIV2, 2.0f) - 2.0f) < 1e-3f);
        assert(std::abs(CameraBobOffset(DirectX::XM_PI, 2.0f)) < 1e-3f);
    }
#endif

    bool isTransitioning_ = false;
    float zoomTimer_ = 0.0f;
    bool isUiVisible_ = true;

    int currentSelect = 0;
    bool stickUpPrev_ = false;
    bool stickDownPrev_ = false;
    bool dpadUpPrev_ = false;
    bool dpadDownPrev_ = false;

    bool fadeStart = false;
    bool isFading = false;
    std::vector<Entity> ownedEntities_{};

    // 複数壁を保持する配列
    std::vector<Entity> wallEntities_{};
    // 配置を渡したい場合にセットする配列
    std::vector<Transform> wallTransforms_{};
 
    //Entity wallEntitiy_{};
    Entity fadeEntity_{};
    Entity playerEntity_{};
    Entity objectEntity_{};
    Entity skyboxEntity_{};
    Entity menuEntity_[3]{};
    Entity baseMenuEntity_[3]{};
    TextureManager::TextureHandle skyboxTexture_ = TextureManager::INVALID_TEXTURE;
    bool skyboxTextureApplied_ = false;
    float skyboxRotation_ = 0.0f;
    float cameraBobPhase_ = 0.0f;
    float cameraBobOffsetY_ = 0.0f;

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    Camera camera_{};

    const std::wstring normalPaths[3] = {
        {L"./Assets/Textures/UI/TitleUI/title4.png "},
        {L"./Assets/Textures/UI/TitleUI/title6.png"},
        {L"./Assets/Textures/UI/TitleUI/title2.png"},
    };

    const std::wstring selectPaths[3] = {
        {L"./Assets/Textures/UI/TitleUI/title3.png "},
        {L"./Assets/Textures/UI/TitleUI/title5.png"},
        {L"./Assets/Textures/UI/TitleUI/title1.png"},
    };
};