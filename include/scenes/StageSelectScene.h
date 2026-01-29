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
#include <random>
#include <cmath>
#include <DirectXMath.h>

#include <Windows.h>

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
#include "app/ResourceManager.h"
#include "graphics/TextureManager.h"
#include "systems/ModelLoadingSystem.h"
#include "components/Light.h"
#include "components/ModelComponent.h"
#include "components/PointLight.h"
#include "components/TransformHierarchy.h"
#include "systems/RenderingSystem.h"
#include "graphics/TextureManager.h"

namespace
{
inline std::wstring Utf8ToWide(const std::string& src)
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
 * @class StageSelectScene
 * @brief ワールドセレクトの統合シーンクラス
 */
class StageSelectScene : public IScene {
  public:
    // Fade state used for world-to-world transition handling
    enum class WorldFadeState {
        None,
        FadeOut,
        FadeIn,
    };

    // StageSelect config
    inline static ConfigVar<int> cfg_WorldCount{"StageSelect.World", "WorldCount", 4, "ステージセレクト: ワールド数"};

    inline static ConfigVar<int> cfg_MaxStagesWorld1{"StageSelect.World1", "MaxStages", 3, "ステージセレクト: World1 最大ステージ数"};
    inline static ConfigVar<int> cfg_MaxStagesWorld2{"StageSelect.World2", "MaxStages", 4, "ステージセレクト: World2 最大ステージ数"};
    inline static ConfigVar<int> cfg_MaxStagesWorld3{"StageSelect.World3", "MaxStages", 5, "ステージセレクト: World3 最大ステージ数"};
    inline static ConfigVar<int> cfg_MaxStagesWorld4{"StageSelect.World4", "MaxStages", 6, "ステージセレクト: World4 最大ステージ数"};

    inline static ConfigVar<std::string> cfg_StationModelBasePath{"StageSelect.Station", "BasePath", "Assets/Models/SelectObj_ISS/Station/", "ステージセレクト: ステーションモデルのベースパス"};
    inline static ConfigVar<std::string> cfg_StationModelPrefix{"StageSelect.Station", "ModelPrefix", "station", "ステージセレクト: ステーションモデル接頭辞"};
    inline static ConfigVar<std::string> cfg_StationModelSuffix{"StageSelect.Station", "ModelSuffix", ".fbx", "ステージセレクト: ステーションモデル拡張子"};
    inline static ConfigVar<std::string> cfg_StationFallbackModelPath{"StageSelect.Station", "FallbackModelPath", "Assets/Models/SelectObj_ISS/Station/World1/station3.fbx", "ステージセレクト: ステーションモデルのフォールバック"};
    inline static ConfigVar<float> cfg_StationRadius{"StageSelect.Station", "Radius", 5.0f, "ステージセレクト: ステーション配置半径"};
    inline static ConfigVar<float> cfg_StationScale{"StageSelect.Station", "Scale", 0.1f, "ステージセレクト: ステーションスケール"};

    inline static ConfigVar<float> cfg_StationPointLightR{"StageSelect.Station.PointLight", "R", 1.0f, "ステージセレクト: ステーション点光源 R"};
    inline static ConfigVar<float> cfg_StationPointLightG{"StageSelect.Station.PointLight", "G", 0.9f, "ステージセレクト: ステーション点光源 G"};
    inline static ConfigVar<float> cfg_StationPointLightB{"StageSelect.Station.PointLight", "B", 0.9f, "ステージセレクト: ステーション点光源 B"};
    inline static ConfigVar<float> cfg_StationPointLightIntensity{"StageSelect.Station.PointLight", "Intensity", 150.0f, "ステージセレクト: ステーション点光源 強度"};
    inline static ConfigVar<float> cfg_StationPointLightRange{"StageSelect.Station.PointLight", "Range", 100.0f, "ステージセレクト: ステーション点光源 距離"};
    inline static ConfigVar<float> cfg_StationPointLightAttenConstant{"StageSelect.Station.PointLight", "AttenConstant", 1.0f, "ステージセレクト: ステーション点光源 減衰(定数)"};
    inline static ConfigVar<float> cfg_StationPointLightAttenLinear{"StageSelect.Station.PointLight", "AttenLinear", 1.0f, "ステージセレクト: ステーション点光源 減衰(一次)"};
    inline static ConfigVar<float> cfg_StationPointLightAttenQuadratic{"StageSelect.Station.PointLight", "AttenQuadratic", 1.0f, "ステージセレクト: ステーション点光源 減衰(二次)"};

    inline static ConfigVar<float> cfg_FadeSizeW{"StageSelect.Fade", "Width", 1280.0f, "ステージセレクト: フェードUIの幅"};
    inline static ConfigVar<float> cfg_FadeSizeH{"StageSelect.Fade", "Height", 720.0f, "ステージセレクト: フェードUIの高さ"};
    inline static ConfigVar<float> cfg_FadeSecondsPerFrame{"StageSelect.Fade", "SecondsPerFrame", 0.1f, "ステージセレクト: フェードアニメ1フレーム時間(秒)"};
    inline static ConfigVar<std::string> cfg_FadeTexturePath{"StageSelect.Fade", "TexturePath", "./Assets/Textures/Fade/tex_fade.png", "ステージセレクト: フェードテクスチャ"};


    inline static ConfigVar<float> cfg_StickThreshold{"StageSelect.Input", "StickThreshold", 0.8f, "ステージセレクト: スティック入力閾値"};

    inline static ConfigVar<float> cfg_RotateSpeed{"StageSelect.Rotation", "RotateSpeed", 6.0f, "ステージセレクト: 回転追従速度"};


    inline static ConfigVar<int> cfg_StarMaxCount{"StageSelect.Stars", "MaxCount", 24, "ステージセレクト: 流れ星最大数"};
    inline static ConfigVar<float> cfg_StarSpawnMinSeconds{"StageSelect.Stars", "SpawnMinSeconds", 0.35f, "ステージセレクト: 流れ星スポーン間隔最小(秒)"};
    inline static ConfigVar<float> cfg_StarSpawnMaxSeconds{"StageSelect.Stars", "SpawnMaxSeconds", 1.10f, "ステージセレクト: 流れ星スポーン間隔最大(秒)"};
    inline static ConfigVar<float> cfg_StarLifeMinSeconds{"StageSelect.Stars", "LifeMinSeconds", 2.0f, "ステージセレクト: 流れ星寿命最小(秒)"};
    inline static ConfigVar<float> cfg_StarLifeMaxSeconds{"StageSelect.Stars", "LifeMaxSeconds", 4.0f, "ステージセレクト: 流れ星寿命最大(秒)"};
    inline static ConfigVar<float> cfg_StarPosMinX{"StageSelect.Stars", "PosMinX", -12.0f, "ステージセレクト: 流れ星初期X最小"};
    inline static ConfigVar<float> cfg_StarPosMaxX{"StageSelect.Stars", "PosMaxX", 12.0f, "ステージセレクト: 流れ星初期X最大"};
    inline static ConfigVar<float> cfg_StarPosMinY{"StageSelect.Stars", "PosMinY", 3.0f, "ステージセレクト: 流れ星初期Y最小"};
    inline static ConfigVar<float> cfg_StarPosMaxY{"StageSelect.Stars", "PosMaxY", 10.0f, "ステージセレクト: 流れ星初期Y最大"};
    inline static ConfigVar<float> cfg_StarPosMinZ{"StageSelect.Stars", "PosMinZ", -12.0f, "ステージセレクト: 流れ星初期Z最小"};
    inline static ConfigVar<float> cfg_StarPosMaxZ{"StageSelect.Stars", "PosMaxZ", 12.0f, "ステージセレクト: 流れ星初期Z最大"};
    inline static ConfigVar<float> cfg_StarVelMinX{"StageSelect.Stars", "VelMinX", -10.0f, "ステージセレクト: 流れ星速度X最小"};
    inline static ConfigVar<float> cfg_StarVelMaxX{"StageSelect.Stars", "VelMaxX", -6.0f, "ステージセレクト: 流れ星速度X最大"};
    inline static ConfigVar<float> cfg_StarVelMinY{"StageSelect.Stars", "VelMinY", -20.0f, "ステージセレクト: 流れ星速度Y最小"};
    inline static ConfigVar<float> cfg_StarVelMaxY{"StageSelect.Stars", "VelMaxY", -2.0f, "ステージセレクト: 流れ星速度Y最大"};
    inline static ConfigVar<float> cfg_StarVelMinZ{"StageSelect.Stars", "VelMinZ", -4.0f, "ステージセレクト: 流れ星速度Z最小"};
    inline static ConfigVar<float> cfg_StarVelMaxZ{"StageSelect.Stars", "VelMaxZ", -1.5f, "ステージセレクト: 流れ星速度Z最大"};

    inline static ConfigVar<float> cfg_StarScaleSmall{"StageSelect.Stars", "ScaleSmall", 0.7f, "ステージセレクト: 流れ星スケール小"};
    inline static ConfigVar<float> cfg_StarScaleMedium{"StageSelect.Stars", "ScaleMedium", 0.9f, "ステージセレクト: 流れ星スケール中"};
    inline static ConfigVar<float> cfg_StarScaleBig{"StageSelect.Stars", "ScaleBig", 1.1f, "ステージセレクト: 流れ星スケール大"};
    inline static ConfigVar<int> cfg_StarMaxSpawnPerFrame{"StageSelect.Stars", "MaxSpawnPerFrame", 2, "ステージセレクト: 1フレームの流れ星生成上限"};

    inline static ConfigVar<float> cfg_StageNameWorldDigit{"StageSelect.UI.StageName", "WorldDigit", 0.0f, "ステージセレクト: StageNameファイル名のワールド数字（0=通常）"};
    inline static ConfigVar<float> cfg_StageNameProjectYOffset{"StageSelect.UI.StageName", "ProjectYOffset", 0.8f, "ステージセレクト: StageName投影Yオフセット"};
    inline static ConfigVar<float> cfg_StageNameProjectXOffset{"StageSelect.UI.StageName", "ProjectXOffset", 1.0f, "ステージセレクト: StageName投影Xオフセット"};

    inline static ConfigVar<float> cfg_StageNameRefDist{"StageSelect.UI.StageName", "RefDist_v2", 2.0f, "ステージセレクト: StageName 基準距離 (スケール1.0になる距離)"};
    inline static ConfigVar<float> cfg_StageNameScaleMax{"StageSelect.UI.StageName", "ScaleMax_v2", 3.0f, "ステージセレクト: StageName 最大スケール制限"};
    inline static ConfigVar<float> cfg_StageNameScaleMin{"StageSelect.UI.StageName", "ScaleMin_v2", 0.0f, "ステージセレクト: StageName 最小スケール制限"};
    inline static ConfigVar<float> cfg_StageNameScalePower{"StageSelect.UI.StageName", "ScalePower", 5.0f, "ステージセレクト: StageName スケール変化の乗数 (大きいほど急に変化)"};

    inline static ConfigVar<float> cfg_CameraFovDegrees{"StageSelect.Camera", "FovDegrees", 90.0f, "ステージセレクト: カメラFOV(度)"};
    inline static ConfigVar<float> cfg_CameraNear{"StageSelect.Camera", "Near", 0.1f, "ステージセレクト: カメラNear"};
    inline static ConfigVar<float> cfg_CameraFar{"StageSelect.Camera", "Far", 10000.0f, "ステージセレクト: カメラFar"};
    inline static ConfigVar<float> cfg_CameraPosX{"StageSelect.Camera", "PosX", 8.0f, "ステージセレクト: カメラ位置X"};
    inline static ConfigVar<float> cfg_CameraPosY{"StageSelect.Camera", "PosY", 1.5f, "ステージセレクト: カメラ位置Y"};
    inline static ConfigVar<float> cfg_CameraPosZ{"StageSelect.Camera", "PosZ", 0.0f, "ステージセレクト: カメラ位置Z"};

    inline static ConfigVar<float> cfg_CameraTransitionForwardStartDist{"StageSelect.CameraTransition.Forward", "StartDist", -40.0f, "ステージセレクト: Forward遷移 開始距離"};
    inline static ConfigVar<float> cfg_CameraTransitionForwardTargetDistRatio{"StageSelect.CameraTransition.Forward", "TargetDistRatio", 0.5f, "ステージセレクト: Forward遷移 ターゲット距離比"};
    inline static ConfigVar<float> cfg_CameraTransitionForwardStartFovDegrees{"StageSelect.CameraTransition.Forward", "StartFovDegrees", 60.0f, "ステージセレクト: Forward遷移 開始FOV(度)"};
    inline static ConfigVar<float> cfg_CameraTransitionForwardVerticalOffset{"StageSelect.CameraTransition.Forward", "VerticalOffset", -2.0f, "ステージセレクト: Forward遷移 垂直オフセット"};
    inline static ConfigVar<float> cfg_CameraTransitionSideMaxAngleDegrees{"StageSelect.CameraTransition.Side", "MaxAngleDegrees", 120.0f, "ステージセレクト: Left/Right遷移 最大回転角(度)"};

    inline static ConfigVar<float> cfg_CameraZoomDurationSeconds{"StageSelect.CameraZoom", "DurationSeconds", 0.3f, "ステージセレクト: ズーム遷移時間(秒)"};
    inline static ConfigVar<float> cfg_CameraZoomMoveRatioPerSecond{"StageSelect.CameraZoom", "MoveRatioPerSecond", 0.5f, "ステージセレクト: ズーム移動係数"};
    inline static ConfigVar<float> cfg_CameraZoomFovDeltaPerSecond{"StageSelect.CameraZoom", "FovDeltaPerSecond", -0.1f, "ステージセレクト: ズームFOV変化/秒(rad)"};

    inline static ConfigVar<float> cfg_WorldUISlideParallax{"StageSelect.UI.World", "SlideParallax", 0.5f, "ステージセレクト: WorldUIスライドパララックス"};

    // Existing
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

    // World max stages (runtime-configured)
    struct max_stages {
        int Stage_Num;
        int Serial;
    };

    inline static max_stages ms[4] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    inline static int s_lastSelected[4] = {1, 1, 1, 1};

    /**
     * @brief コンストラクタ
     * @param worldNumber ワールド番号 (1-4)
     */
    StageSelectScene(int worldNumber)
        : worldNumber_(worldNumber) {
        RefreshMaxStage();
    }

    void RefreshMaxStage() {
        const int maxWorld = std::clamp(cfg_WorldCount.Get(), 1, 4);
        worldNumber_ = std::clamp(worldNumber_, 1, maxWorld);

        int maxStage = 1;
        switch (worldNumber_) {
            case 1:
                maxStage = cfg_MaxStagesWorld1.Get();
                break;
            case 2:
                maxStage = cfg_MaxStagesWorld2.Get();
                break;
            case 3:
                maxStage = cfg_MaxStagesWorld3.Get();
                break;
            case 4:
                maxStage = cfg_MaxStagesWorld4.Get();
                break;
            default:
                maxStage = cfg_MaxStagesWorld1.Get();
                break;
        }
        maxStage_ = std::max(maxStage, 1);
        if (worldNumber_ >= 1 && worldNumber_ <= 4) {
            ms[worldNumber_ - 1].Stage_Num = maxStage_;
        }
    }

    void OnEnter(World &world) override {
        Entity dirLightEntity = world.Create().With<DirectionalLight>().Build();
        ownedEntities_.push_back(dirLightEntity);
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
        RenderingSystem::GetInstance().SetAmbientLight(
            {cfg_AmbientR.Get(), cfg_AmbientG.Get(), cfg_AmbientB.Get()},
            cfg_AmbientIntensity.Get());
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

        RefreshMaxStage();

        // ワールド移動時のステージ番号初期化
        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            stats.worldCount = worldNumber_;
            stats.Normalize(maxStage_, worldNumber_);
            DEBUGLOG("[StageSelect] Before flags world=" + std::to_string(stats.worldCount) +
                     " select=" + std::to_string(stats.selectStage) +
                     " current=" + std::to_string(stats.currentStage) +
                     " IsClearBack=" + std::to_string(stats.IsClearBack) +
                     " clearedThisStage=" + std::to_string(stats.clearedThisStage));
            if (stats.IsWorldBack) {
                stats.selectStage = maxStage_;
                stats.currentStage = stats.selectStage;
                stats.IsWorldBack = false;
            } else if (stats.IsWorldNext) {
                stats.selectStage = 1;
                stats.currentStage = stats.selectStage;
                stats.IsWorldNext = false;
            } else if (stats.IsClearBack) {
                if (stats.worldCount != worldNumber_) {
                    stats.selectStage = std::clamp(stats.selectStage, 1, maxStage_);
                    stats.currentStage = stats.selectStage;
                }
                stats.IsClearBack = false;
            }
            stats.currentRoom = 1;
            stats.clearedThisStage = false;
            stats.goalTransitioning = false;
            stats.requestAdvance = false;
            stats.pressedSwitch = false;
            stats.goalUnlocked = false;
            if (worldNumber_ >= 1 && worldNumber_ <= 4) {
                s_lastSelected[worldNumber_ - 1] = std::clamp(stats.currentStage, 1, maxStage_);
            }
            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage_);
            stats.currentStage = std::clamp(stats.currentStage, 1, maxStage_);
            g_LastStageProgress = stats;
            DEBUGLOG("[StageSelect] After init world=" + std::to_string(stats.worldCount) +
                     " select=" + std::to_string(stats.selectStage) +
                     " current=" + std::to_string(stats.currentStage) +
                     " clearedThisStage=" + std::to_string(stats.clearedThisStage) +
                     " g_Last world=" + std::to_string(g_LastStageProgress.worldCount) +
                     " g_Last select=" + std::to_string(g_LastStageProgress.selectStage) +
                     " g_Last clearedThisStage=" + std::to_string(g_LastStageProgress.clearedThisStage));
        });

        stickRightPrev_ = false;
        stickLeftPrev_ = false;
        dpadRightPrev_ = false;
        dpadLeftPrev_ = false;

        // カメラ初期化
        baseFovY_ = DirectX::XMConvertToRadians(cfg_CameraFovDegrees.Get());
        cameraNear_ = cfg_CameraNear.Get();
        cameraFar_ = cfg_CameraFar.Get();
        cameraPosition_ = {cfg_CameraPosX.Get(), cfg_CameraPosY.Get(), cfg_CameraPosZ.Get()};

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
        inputProtectionTimer_ = 0.5f;

        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            const int stage = std::clamp(stats.selectStage, 1, maxStage_);
            const float step = DirectX::XM_2PI / static_cast<float>(std::max(maxStage_, 1));
            targetAngle_ = -step * static_cast<float>(stage - 1);
            currentAngle_ = targetAngle_;
            skyboxYawDeg_ = DirectX::XMConvertToDegrees(currentAngle_);
            if (worldNumber_ >= 1 && worldNumber_ <= 4) {
                s_lastSelected[worldNumber_ - 1] = stage;
            }
            DEBUGLOG("[StageSelect] Init camera angle world=" + std::to_string(worldNumber_) +
                     " stage=" + std::to_string(stage) +
                     " angle=" + std::to_string(currentAngle_));
        });

        if (cfg_DirLightEnabled.Get()) {
            Entity dirLight = world.Create().With<DirectionalLight>().Build();
            if (auto *light = world.TryGet<DirectionalLight>(dirLight)) {
                light->direction = {cfg_DirLightX.Get(), cfg_DirLightY.Get(), cfg_DirLightZ.Get()};
                light->color = {cfg_DirLightR.Get(), cfg_DirLightG.Get(), cfg_DirLightB.Get(), std::max(0.0f, cfg_DirLightIntensity.Get())};
            }
            ownedEntities_.push_back(dirLight);
        }

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
        std::string basePath = "Assets/Models/SelectObj_ISS/Station/" + worldName + "/station";

        int stageCount = maxStage_;
        if (worldNumber_ >= 1 && worldNumber_ <= 4) {
            int idx = worldNumber_ - 1;
            if (ms[idx].Stage_Num > 0) {
                stageCount = ms[idx].Stage_Num;
            }
        }

        std::string fallbackPath = cfg_StationFallbackModelPath.Get();
        const float radius = cfg_StationRadius.Get();
        for (int i = 0; i < stageCount; ++i) {
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

        //2DUI
        UITransform FadeAnimation;
        FadeAnimation.position = {0.0f, 0.0f};
        FadeAnimation.size = {cfg_FadeSizeW.Get(), cfg_FadeSizeH.Get()};
        FadeAnimation.anchor = {0.0f, 0.0f};
        FadeAnimation.pivot = {0.0f, 0.0f};

        const std::wstring fadePath = Utf8ToWide(cfg_FadeTexturePath.Get());
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

        shootingStars_.clear();
        starSpawnTimer_ = 0.0f;
        nextStarSpawn_ = RandFloat(cfg_StarSpawnMinSeconds.Get(), cfg_StarSpawnMaxSeconds.Get());
        lastStageNameStage_ = -1;

        isFading = false;
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        if (inputProtectionTimer_ > 0.0f) {
            inputProtectionTimer_ -= deltaTime;
            if (inputProtectionTimer_ < 0.0f) {
                inputProtectionTimer_ = 0.0f;
            }
            world.Tick(deltaTime);
            return;
        }

        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_) {
                sys.input_ = &input;
            }
        });

        UpdateWorldTransitionFade(world);
        if (worldFadeState_ != WorldFadeState::None) {
            world.Tick(deltaTime);
            return;
        }

        if (isFading) {
            world.Tick(deltaTime);
            if (auto *anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_)) {
                if (anim->isFinished) {
                        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                            std::string nextScene = "Game";
                            world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
                                if (worldNumber_ == 1 && sp.selectStage == 1) {
                                    nextScene = "Stage1IntroVideo";
                                }

                            sp.currentStage = sp.selectStage;
                            sp.currentRoom = 1;
                            sp.requestAdvance = false;
                            sp.goalTransitioning = false;
                            sp.clearedThisStage = false;
                            sp.pressedSwitch = false;
                            sp.goalUnlocked = false;
                            DEBUGLOG("[StageSelect] Transition to Game: world=" + std::to_string(sp.worldCount) +
                                     " currentStage=" + std::to_string(sp.currentStage) +
                                     " currentRoom=" + std::to_string(sp.currentRoom) +
                                     " clearedThisStage=" + std::to_string(sp.clearedThisStage) +
                                     " goalTransitioning=" + std::to_string(sp.goalTransitioning));
                        });
                        manager->ChangeScene(nextScene.c_str(), world);
                    }
                }
            }
            return;
        }

        // ゲームシーンへの遷移 (Enter / ×ボタン)
        bool trigger = input.GetKeyDown(VK_RETURN);
        GamepadSystem *padsystem = ServiceLocator::TryGet<GamepadSystem>();
        if (!isTransitioning_) {
            if (padsystem &&
                padsystem->GetAnyButtonDown({GamepadSystem::Button_A})) {
                trigger = true;
                SOUND_SYS.PlaySE(cfg_EnterMP3Pass,false);
            }
            if (trigger) {
                isTransitioning_ = true;
                zoomTimer_ = 0.0f;
                DEBUGLOG("StageSelect Camera Zoom Start!");
                //StartFadeInNormal(world);
            }
        } else {
            UpdateCameraZoom(world, deltaTime);
            /* StartFadeInNormal(world);*/
        }

        UpdateSkyboxTransform(world);
        UpdateSkyboxTexture(world);
        UpdateShootingStars(deltaTime);
        EffekseerManager::GetInstance().Update();

        // ステージ選択処理
        TransitionDirection requestWorldTransition = TransitionDirection::None;
        world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
            bool rightPressed = input.GetKeyDown(VK_RIGHT);
            bool leftPressed = input.GetKeyDown(VK_LEFT);

            if (padsystem) {
                float gx = padsystem->GetLeftStickX();
                bool dpadRightNow = padsystem->GetButton(padsystem->Button_DPad_Right);
                bool dpadLeftNow = padsystem->GetButton(padsystem->Button_DPad_Left);

                const float stickThreshold = cfg_StickThreshold.Get();
                bool stickRightNow = gx > stickThreshold;
                bool stickLeftNow = gx < -stickThreshold;

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

            World *wptr = &world;
            bool dpadStartNow = padsystem->GetButton(padsystem->Button_B);
            if (dpadStartNow) {
                SOUND_SYS.PlaySE(cfg_EnterMP3Pass,false);
                    if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
                        manager->ChangeScene("Title", *wptr);
                    }
           }

            if (rightPressed) {
                if (stats.selectStage < maxStage_) {
                    stats.selectStage++;
                    targetAngle_ -= DirectX::XM_2PI / maxStage_;
                    SOUND_SYS.PlaySE(cfg_SelectMP3Pass,true);
                    if (worldNumber_ >= 1 && worldNumber_ <= 4) {
                        s_lastSelected[worldNumber_ - 1] = stats.selectStage;
                    }
                } else if (stats.selectStage == maxStage_) {
                    if (stats.worldCount != cfg_WorldCount.Get()) {
                        SOUND_SYS.PlaySE(cfg_SelectMP3Pass,true);
                        // 次のワールドへ
                        stats.IsWorldBack = false;
                        stats.IsWorldNext = true;
                        requestWorldTransition = TransitionDirection::Right;
                    }
                }
            }
            if (leftPressed) {
                if (stats.selectStage > 1) {
                    stats.selectStage--;
                    targetAngle_ += DirectX::XM_2PI / maxStage_;
                    SOUND_SYS.PlaySE(cfg_SelectMP3Pass,true);
                    if (worldNumber_ >= 1 && worldNumber_ <= 4) {
                        s_lastSelected[worldNumber_ - 1] = stats.selectStage;
                    }
                } else {
                    // 前のワールドへ
                    if (worldNumber_ > 1) {
                        stats.IsWorldNext = false;
                        stats.IsWorldBack = true;
                        requestWorldTransition = TransitionDirection::Left;
                    }
                }
            }

            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage_);

            stats.selectStage = std::clamp(stats.selectStage, 1, maxStage_);
            
            // UpdateStageNameTexture removed (now handled at init/static strings)
        });

        if (requestWorldTransition == TransitionDirection::Right) {
            GoToNextWorld(world);
            return;
        }
        if (requestWorldTransition == TransitionDirection::Left) {
            GoToPrevWorld(world);
            return;
        }

        // 回転アニメーション
        rotateSpeed_ = cfg_RotateSpeed.Get();
        currentAngle_ += (targetAngle_ - currentAngle_) * deltaTime * rotateSpeed_;
        skyboxYawDeg_ = DirectX::XMConvertToDegrees(currentAngle_);
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
                    const float startDist = cfg_CameraTransitionForwardStartDist.Get();
                    const float currentDist = startDist * (1.0f - easedProgress);
                    renderCamera.position.z += currentDist;
                    renderCamera.target.z += currentDist * cfg_CameraTransitionForwardTargetDistRatio.Get();

                    const float startFov = DirectX::XMConvertToRadians(cfg_CameraTransitionForwardStartFovDegrees.Get());
                    const float endFov = baseFovY_;
                    renderCamera.fovY = startFov + (endFov - startFov) * easedProgress;

                    const float verticalOffset = cfg_CameraTransitionForwardVerticalOffset.Get() * (1.0f - easedProgress);
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
                    float maxAngle = DirectX::XMConvertToRadians(cfg_CameraTransitionSideMaxAngleDegrees.Get());
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
        float screenHeight = gfx ? static_cast<float>(gfx->Height()) : 720.0f;
        float uiOffsetX = -transitionOffset * screenWidth;

        SceneManager *manager = ServiceLocator::TryGet<SceneManager>();
        TransitionDirection sceneTransitionDir = TransitionDirection::None;
        TransitionPhase sceneTransitionPhase = TransitionPhase::None;
        float sceneTransitionProgress = 0.0f;
        float worldUISlideOffsetX = 0.0f;
        bool isWorldSwitchTransition = false;
        if (manager && manager->IsTransitioning()) {
            sceneTransitionDir = manager->GetTransitionDirection();
            sceneTransitionPhase = manager->GetTransitionPhase();
            sceneTransitionProgress = manager->GetTransitionProgress();
            if (sceneTransitionDir == TransitionDirection::Left || sceneTransitionDir == TransitionDirection::Right) {
                isWorldSwitchTransition = true;
                worldUISlideOffsetX = -manager->GetTransitionOffset() * screenWidth;
            }
        }

        UpdateSkyboxTexture(world);
        if (world.IsAlive(worldUIEntity_)) {
            if (auto *tr = world.TryGet<UITransform>(worldUIEntity_)) {
                if (isWorldSwitchTransition) {
                    tr->position = {worldUIBasePos_.x + worldUISlideOffsetX, worldUIBasePos_.y};
                } else {
                    tr->position = {worldUIBasePos_.x - uiOffsetX * 0.5f, worldUIBasePos_.y};
                }
            }
        }

        if (isWorldSwitchTransition && world.IsAlive(fadeEntity_)) {
            auto *img = world.TryGet<UIImage>(fadeEntity_);
            auto *anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_);
            if (img && anim) {
                const int maxFrame = std::max(anim->frameCount - 1, 0);
                const float t = std::clamp(sceneTransitionProgress, 0.0f, 1.0f);
                int frame = 0;
                if (sceneTransitionPhase == TransitionPhase::SlideOut) {
                    frame = static_cast<int>(t * static_cast<float>(maxFrame) + 0.5f);
                } else {
                    frame = static_cast<int>((1.0f - t) * static_cast<float>(maxFrame) + 0.5f);
                }
                frame = std::clamp(frame, 0, maxFrame);

                anim->isPlaying = false;
                anim->isFinished = false;
                anim->currentFrame = frame;
                if (anim->uv.size() != static_cast<size_t>(std::max(anim->frameCount, 0))) {
                    anim->UpdateUV();
                }
                if (!anim->uv.empty()) {
                    img->uvRect = anim->uv[frame];
                }
                img->opacity = 1.0f;
            }
        }

        UpdateStageNameFollow(world, renderCamera, uiOffsetX, screenWidth, screenHeight);

        renderer.Render(world, renderCamera);
        EffekseerManager::GetInstance().Draw(renderCamera);
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
        StopShootingStars();
        if (auto *resMgr = ServiceLocator::TryGet<ResourceManager>()) {
            resMgr->Clear();
        }
        if (auto *texMgr = ServiceLocator::TryGet<TextureManager>()) {
            texMgr->Shutdown();
            texMgr->Init(ServiceLocator::Get<GfxDevice>());
        }
        ModelLoader::ClearTextureCache();
        for (const auto &e : ownedEntities_) {
            DestroyEntityHierarchy(world, e);
        }
        ownedEntities_.clear();

        for (const auto &e : objectOwnedEntities_) {
            DestroyEntityHierarchy(world, e);
        }
        objectOwnedEntities_.clear();

        if (world.IsAlive(worldUIEntity_)) {
             DestroyEntityHierarchy(world, worldUIEntity_);
             worldUIEntity_ = {};
        }
        
        for (const auto &e : stageNameEntities_) {
            if (world.IsAlive(e)) {
                DestroyEntityHierarchy(world, e);
            }
        }
        stageNameEntities_.clear();

        textSystem_.Shutdown();
        imageSystem_.Shutdown();
        isFading = false;
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

    std::vector<Entity> stageNameEntities_{};
    Entity worldUIEntity_{};
    DirectX::XMFLOAT2 worldUIBasePos_{0.0f, 0.0f};
    Entity fadeEntity_{};
    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    Camera camera_{};

    // Camera params
    float baseFovY_ = 90.0f;
    float cameraNear_ = 0.1f;
    float cameraFar_ = 10000.0f;
    DirectX::XMFLOAT3 baseUp_ = {0.0f, 1.0f, 0.0f};
    DirectX::XMFLOAT3 baseTarget_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 cameraPosition_ = {8.0f, 1.5f, 0.0f};

    // Animation
    float currentAngle_ = 0.0f;
    float targetAngle_ = 0.0f;
    float rotateSpeed_ = 6.0f;
    bool isFading = false;
    WorldFadeState worldFadeState_ = WorldFadeState::None;
    TransitionDirection pendingWorldTransitionDir_ = TransitionDirection::None;

    float skyboxYawDeg_ = 0.0f;

    std::vector<Entity> ownedEntities_{};
    std::vector<Entity> objectOwnedEntities_;
    Entity skyboxEntity_{};
    TextureManager::TextureHandle skyboxTexture_ = TextureManager::INVALID_TEXTURE;
    bool skyboxTextureApplied_ = false;

    struct ShootingStar {
        int handle = -1;
        DirectX::XMFLOAT3 pos{0.0f, 0.0f, 0.0f};
        DirectX::XMFLOAT3 vel{0.0f, 0.0f, 0.0f};
        float age = 0.0f;
        float life = 1.0f;
    };

    std::vector<ShootingStar> shootingStars_{};
    float starSpawnTimer_ = 0.0f;
    float nextStarSpawn_ = 0.6f;
    std::mt19937 starRng_{std::random_device{}()};

    int lastStageNameStage_ = -1;
    DirectX::XMFLOAT2 stageNameBaseSize_{0.0f, 0.0f};

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
        skyboxYawDeg_ = DirectX::XMConvertToDegrees(currentAngle_);
        Transform transform{
            {camera_.position.x, camera_.position.y, camera_.position.z},
            {0.0f, skyboxYawDeg_, 0.0f},
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
            t->rotation = {0.0f, -skyboxYawDeg_, 0.0f};
            t->scale = {scale, scale, scale};
        }
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

    float RandFloat(float minValue, float maxValue) {
        if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
            return 0.0f;
        }
        if (minValue > maxValue) {
            std::swap(minValue, maxValue);
        }
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(starRng_);
    }

    int RandInt(int minValue, int maxValue) {
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(starRng_);
    }

    void SpawnShootingStar() {
        if (shootingStars_.size() >= static_cast<size_t>(std::max(cfg_StarMaxCount.Get(), 0))) {
            return;
        }

        const int kind = RandInt(0, 2);
        const char *effectName = (kind == 0) ? "StarSmall" : (kind == 1) ? "StarMedium"
                                                                     : "StarBig";

        DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
        if (kind == 0) {
            scale = {cfg_StarScaleSmall.Get(), cfg_StarScaleSmall.Get(), cfg_StarScaleSmall.Get()};
        } else if (kind == 1) {
            scale = {cfg_StarScaleMedium.Get(), cfg_StarScaleMedium.Get(), cfg_StarScaleMedium.Get()};
        } else {
            scale = {cfg_StarScaleBig.Get(), cfg_StarScaleBig.Get(), cfg_StarScaleBig.Get()};
        }

        ShootingStar star;
        star.pos = {RandFloat(cfg_StarPosMinX.Get(), cfg_StarPosMaxX.Get()), RandFloat(cfg_StarPosMinY.Get(), cfg_StarPosMaxY.Get()), RandFloat(cfg_StarPosMinZ.Get(), cfg_StarPosMaxZ.Get())};
        star.vel = {RandFloat(cfg_StarVelMinX.Get(), cfg_StarVelMaxX.Get()), RandFloat(cfg_StarVelMinY.Get(), cfg_StarVelMaxY.Get()), RandFloat(cfg_StarVelMinZ.Get(), cfg_StarVelMaxZ.Get())};
        star.life = RandFloat(cfg_StarLifeMinSeconds.Get(), cfg_StarLifeMaxSeconds.Get());

        auto handleOpt = EffekseerManager::GetInstance().PlayEffectSafe(effectName, star.pos, scale, false);
        if (!handleOpt) {
            return;
        }
        star.handle = *handleOpt;
        shootingStars_.push_back(star);
    }

    void UpdateShootingStars(float dt) {
        if (dt <= 0.0f) {
            return;
        }

        starSpawnTimer_ += dt;
        int spawnCountThisFrame = 0;
        const int spawnLimit = std::max(cfg_StarMaxSpawnPerFrame.Get(), 0);
        while (starSpawnTimer_ >= nextStarSpawn_ && spawnCountThisFrame < spawnLimit) {
            starSpawnTimer_ -= nextStarSpawn_;
            nextStarSpawn_ = RandFloat(cfg_StarSpawnMinSeconds.Get(), cfg_StarSpawnMaxSeconds.Get());
            SpawnShootingStar();
            ++spawnCountThisFrame;
        }

        auto &efk = EffekseerManager::GetInstance();
        for (auto it = shootingStars_.begin(); it != shootingStars_.end();) {
            it->age += dt;
            it->pos.x += it->vel.x * dt;
            it->pos.y += it->vel.y * dt;
            it->pos.z += it->vel.z * dt;

            if (it->handle >= 0) {
                efk.SetEffectPosition(it->handle, it->pos);
            }

            if (it->age >= it->life) {
                if (it->handle >= 0) {
                    efk.StopEffectHandle(it->handle);
                }
                it = shootingStars_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void StopShootingStars() {
        auto &efk = EffekseerManager::GetInstance();
        for (auto &star : shootingStars_) {
            if (star.handle >= 0) {
                efk.StopEffectHandle(star.handle);
            }
        }
        shootingStars_.clear();
    }


    void UpdateStageNameFollow(World &world, const Camera &camera, float uiOffsetX, float screenWidth, float screenHeight) {
        if (stageNameEntities_.empty() || objectOwnedEntities_.empty() || screenWidth <= 0.0f || screenHeight <= 0.0f) {
            return;
        }

        // 全ステージ分のUI位置・スケール更新
        int currSelectStage = 1;
        world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
             if (sp.worldCount == worldNumber_) {
                 currSelectStage = sp.selectStage;
             }
        });

        for (size_t i = 0; i < stageNameEntities_.size(); ++i) {
            Entity uiEntity = stageNameEntities_[i];
            
            // Visibility Check: Only show selected stage
            // i is 0-based index, stage is 1-based.
            if ((static_cast<int>(i) + 1) != currSelectStage) {
                if (auto *img = world.TryGet<UIImage>(uiEntity)) {
                    img->opacity = 0.0f; // Hide
                }
                continue;
            }
            if (!world.IsAlive(uiEntity)) continue;

            // 対応するStationオブジェクトを取得
            // objectOwnedEntities_ の構造に依存するが、Stationは最初の方に追加されているため、
            // ステージ数とインデックスが一致していると仮定する。
            // CreateObjectで追加された順序がステージ順であればOK。
            // (CreateObjectはLoadでi=0から順に呼ばれていることを確認済み)
            if (i >= objectOwnedEntities_.size()) break;

            Entity station = objectOwnedEntities_[i];
            auto *stationTr = world.TryGet<Transform>(station);
            if (!stationTr) continue;

            DirectX::XMFLOAT3 p = stationTr->position;
            p.y += cfg_StageNameProjectYOffset.Get();
            p.x += cfg_StageNameProjectXOffset.Get();

            DirectX::XMVECTOR proj = DirectX::XMVector3Project(
                DirectX::XMLoadFloat3(&p),
                0.0f, 0.0f, screenWidth, screenHeight,
                0.0f, 1.0f,
                camera.GetProjectionMatrix(),
                camera.GetViewMatrix(),
                DirectX::XMMatrixIdentity());

            float sx = DirectX::XMVectorGetX(proj);
            float sy = DirectX::XMVectorGetY(proj);
            float sz = DirectX::XMVectorGetZ(proj);

            auto *img = world.TryGet<UIImage>(uiEntity);
            if (img) {
                img->opacity = (sz >= 0.0f && sz <= 1.0f) ? 1.0f : 0.0f;
            }
            if (sz < 0.0f || sz > 1.0f) {
                continue;
            }

            auto *uiTr = world.TryGet<UITransform>(uiEntity);
            if (!uiTr) continue;

            uiTr->anchor = {0.0f, 0.0f};
            uiTr->pivot = {0.0f, 0.0f};
            uiTr->position = {sx - uiOffsetX, sy};

            // 距離によるスケール計算
            DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&camera.position);
            DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&p);
            float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(camPos, targetPos)));

            const float refDist = cfg_StageNameRefDist.Get();
            const float scaleMax = cfg_StageNameScaleMax.Get();
            const float scaleMin = cfg_StageNameScaleMin.Get();
            const float scalePower = cfg_StageNameScalePower.Get();

            float currentScale = 1.0f;
            if (dist > 0.001f) {
                // Easing: Scale = pow(Ref / Dist, Power)
                float ratio = refDist / dist;
                currentScale = std::pow(ratio, scalePower);
            }
            currentScale = std::clamp(currentScale, scaleMin, scaleMax);

            // BaseSize init check (per entity logic ideally, but sharing member for now since they are same texture usually)
            // But texture might differ? Size should be from texture.
            // If we use same base size for all, it's fine.
            if (stageNameBaseSize_.x <= 0.0f || stageNameBaseSize_.y <= 0.0f) {
                 stageNameBaseSize_ = uiTr->size;
                 if (stageNameBaseSize_.x <= 0.0f) stageNameBaseSize_ = {621.0f, 207.0f}; 
            }

            if (stageNameBaseSize_.x > 0.0f && stageNameBaseSize_.y > 0.0f) {
                uiTr->size = {stageNameBaseSize_.x * currentScale, stageNameBaseSize_.y * currentScale};
            }
            
            // Log only for the first stage (selected or not, just index 0) to avoid spam
            if (i == 0) {
                 static int logCounter = 0;
                 if (logCounter++ % 60 == 0) {
                      DEBUGLOG("UI Scale Debug(Stg1): Dist=" + std::to_string(dist) + " Scale=" + std::to_string(currentScale) + " Size=" + std::to_string(uiTr->size.x));
                 }
            }
        }
    }

    void CreateObject(World &world, const DirectX::XMFLOAT3 &position, const std::string &modelPath) {
        const float stationScale = cfg_StationScale.Get();
        DirectX::XMFLOAT3 diffPos = position;
        diffPos.y += 10;
        diffPos.z += 10;
        Transform transform{position, {0.0f, 0.0f, 0.0f}, {stationScale, stationScale, stationScale}};
        ObjectPos pos;
        pos.basepos = position;

        PointLight light{
            DirectX::XMFLOAT3{cfg_StationPointLightR.Get(), cfg_StationPointLightG.Get(), cfg_StationPointLightB.Get()},
            cfg_StationPointLightIntensity.Get(),
            cfg_StationPointLightRange.Get()};
        light.SetAttenuation(cfg_StationPointLightAttenConstant.Get(), cfg_StationPointLightAttenLinear.Get(), cfg_StationPointLightAttenQuadratic.Get());

        auto builder = world.Create().With<Transform>(transform).With<ObjectPos>(pos);
        Entity e_light = world.Create().With<Transform>(diffPos).With<PointLight>(light);
        ownedEntities_.push_back(e_light);

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

    void BeginWorldTransitionFade(World &world, TransitionDirection direction) {
        if (direction == TransitionDirection::None || worldFadeState_ != WorldFadeState::None || !world.IsAlive(fadeEntity_)) {
            return;
        }
        pendingWorldTransitionDir_ = direction;
        worldFadeState_ = WorldFadeState::FadeOut;
        StartSpriteFade(world, fadeEntity_, 1, false);
    }

    void UpdateWorldTransitionFade(World &world) {
        if (worldFadeState_ == WorldFadeState::None || !world.IsAlive(fadeEntity_)) {
            return;
        }
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_)) {
            if (!anim->isPlaying && anim->isFinished) {
                TransitionDirection dir = pendingWorldTransitionDir_;
                pendingWorldTransitionDir_ = TransitionDirection::None;
                worldFadeState_ = WorldFadeState::None;
                if (dir == TransitionDirection::Right) {
                    GoToNextWorld(world);
                } else if (dir == TransitionDirection::Left) {
                    GoToPrevWorld(world);
                }
            }
        }
    }

    void GoToNextWorld(World &world) {
        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            std::string nextScene = "World" + std::to_string(worldNumber_ + 1) + "_StageSelect";

            const int maxWorld = std::clamp(cfg_WorldCount.Get(), 1, 4);
            if (worldNumber_ >= maxWorld) {
                // 現状維持（何もしないorループ）
            } else {
                // 次のワールドへ: 右方向にスライド（進む）
                manager->ChangeSceneWithTransition(nextScene.c_str(), world, TransitionDirection::Right);
            }
        }
    }

    void GoToPrevWorld(World &world) {
        if (worldNumber_ <= 1)
            return; // World1以前はない

        if (auto *manager = ServiceLocator::TryGet<SceneManager>()) {
            std::string prevScene = "World" + std::to_string(worldNumber_ - 1) + "_StageSelect";
            // 前のワールドへ: 左方向にスライド（戻る）
            manager->ChangeSceneWithTransition(prevScene.c_str(), world, TransitionDirection::Left);
        }
    }

    void UpdateCameraZoom(World &world, float deltaTime) {
        //遷移の時間
        const float duration = std::max(cfg_CameraZoomDurationSeconds.Get(), 0.0001f);
        zoomTimer_ += deltaTime;

        float progress = std::min(zoomTimer_ / duration, 1.0f);

        //ターゲットに向かってベクトルを計算し、カメラの位置を近づける
        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera_.position);
        DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camera_.target);
        DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(target, pos);

        pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, cfg_CameraZoomMoveRatioPerSecond.Get() * deltaTime));
        DirectX::XMStoreFloat3(&camera_.position, pos);

        //視野角
        camera_.Zoom(cfg_CameraZoomFovDeltaPerSecond.Get() * deltaTime);
        camera_.Update();
        //シーン遷移
        if (progress >= 1.0f) {
            StartFadeInNormal(world);
            isFading = true;
            return;
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
    float inputProtectionTimer_ = 0.0f;

    bool stickRightPrev_ = false;
    bool stickLeftPrev_ = false;
    bool dpadRightPrev_ = false;
    bool dpadLeftPrev_ = false;

    // UI creation methods defined in StageUI.cpp
    void CreateTextStageNoFormats();
    void CreateTextNormalFormats();
    void CreateStageSelectUI(World &world);
};
