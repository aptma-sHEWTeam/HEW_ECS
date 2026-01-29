/**
 * @file StageUI.cpp
 * @brief StageSelectScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "scenes/StageSelectScene.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h"

inline static ConfigVar<float> cfg_StageSelectUI_WorldPosX{"StageSelect.UI.World", "PosX", 0.0f, "ステージセレクト: WorldUI 位置X(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldPosY{"StageSelect.UI.World", "PosY", 30.0f, "ステージセレクト: WorldUI 位置Y(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldSizeW{"StageSelect.UI.World", "Width", 600.0f, "ステージセレクト: WorldUI 幅(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldSizeH{"StageSelect.UI.World", "Height", 140.0f, "ステージセレクト: WorldUI 高さ(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldAnchorX{"StageSelect.UI.World", "AnchorX", 0.5f, "ステージセレクト: WorldUI AnchorX"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldAnchorY{"StageSelect.UI.World", "AnchorY", 0.0f, "ステージセレクト: WorldUI AnchorY"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldPivotX{"StageSelect.UI.World", "PivotX", 0.5f, "ステージセレクト: WorldUI PivotX"};
inline static ConfigVar<float> cfg_StageSelectUI_WorldPivotY{"StageSelect.UI.World", "PivotY", 0.0f, "ステージセレクト: WorldUI PivotY"};

inline static ConfigVar<float> cfg_StageSelectUI_StageNamePosX{"StageSelect.UI.StageName", "PosX", 480.0f, "ステージセレクト: StageName 位置X(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNamePosY{"StageSelect.UI.StageName", "PosY", 270.0f, "ステージセレクト: StageName 位置Y(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNameSizeW{"StageSelect.UI.StageName", "Width", 621.0f, "ステージセレクト: StageName 幅(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNameSizeH{"StageSelect.UI.StageName", "Height", 207.0f, "ステージセレクト: StageName 高さ(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNameAnchorX{"StageSelect.UI.StageName", "AnchorX", 0.0f, "ステージセレクト: StageName AnchorX"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNameAnchorY{"StageSelect.UI.StageName", "AnchorY", 0.0f, "ステージセレクト: StageName AnchorY"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNamePivotX{"StageSelect.UI.StageName", "PivotX", 0.5f, "ステージセレクト: StageName PivotX"};
inline static ConfigVar<float> cfg_StageSelectUI_StageNamePivotY{"StageSelect.UI.StageName", "PivotY", 0.5f, "ステージセレクト: StageName PivotY"};

inline static ConfigVar<float> cfg_StageSelectUI_ButtonSizeW{"StageSelect.UI.Button", "Width", 303.0f, "ステージセレクト: Button 幅(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_ButtonSizeH{"StageSelect.UI.Button", "Height", 60.0f, "ステージセレクト: Button 高さ(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_EnterBtnPosX{"StageSelect.UI.Button.Enter", "PosX", 60.0f, "ステージセレクト: EnterButton 位置X(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_EnterBtnPosY{"StageSelect.UI.Button.Enter", "PosY", 570.0f, "ステージセレクト: EnterButton 位置Y(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_TitleBtnPosX{"StageSelect.UI.Button.Title", "PosX", 60.0f, "ステージセレクト: TitleButton 位置X(px)"};
inline static ConfigVar<float> cfg_StageSelectUI_TitleBtnPosY{"StageSelect.UI.Button.Title", "PosY", 645.0f, "ステージセレクト: TitleButton 位置Y(px)"};

inline static ConfigVar<float> cfg_FadeFrameTime{"Fade.Out", "FadeFrameTime", 0.01f, "フェードアウト1フレームの時間（秒）"};

void StageSelectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}

void StageSelectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}

void StageSelectScene::CreateStageSelectUI(World &world) {

    int initialStage = 1;
    world.ForEach<StageProgress>([&](Entity, StageProgress &progress) {
        progress.Normalize(maxStage_, worldNumber_);
        initialStage = std::clamp(progress.selectStage, 1, maxStage_);
        DEBUGLOG("[StageSelect] UI init world=" + std::to_string(progress.worldCount) +
                 " select=" + std::to_string(progress.selectStage) +
                 " current=" + std::to_string(progress.currentStage) +
                 " g_Last world=" + std::to_string(g_LastStageProgress.worldCount) +
                 " g_Last select=" + std::to_string(g_LastStageProgress.selectStage));
    });
    if (worldNumber_ >= 1 && worldNumber_ <= 4) {
        initialStage = std::clamp(s_lastSelected[worldNumber_ - 1], 1, maxStage_);
    }

    world.ForEach<StageProgress>([&](Entity, StageProgress &progress) {
        progress.selectStage = initialStage;
        progress.currentStage = initialStage;
    });

    // WorldNo
    UITransform worldImgTr;
    worldImgTr.position = {cfg_StageSelectUI_WorldPosX.Get(), cfg_StageSelectUI_WorldPosY.Get()};
    worldImgTr.size = {cfg_StageSelectUI_WorldSizeW.Get(), cfg_StageSelectUI_WorldSizeH.Get()};
    worldImgTr.anchor = {cfg_StageSelectUI_WorldAnchorX.Get(), cfg_StageSelectUI_WorldAnchorY.Get()};
    worldImgTr.pivot = {cfg_StageSelectUI_WorldPivotX.Get(), cfg_StageSelectUI_WorldPivotY.Get()};

    worldUIBasePos_ = worldImgTr.position;

    std::wstring worldPath = L"Assets/Textures/UI/WorldUI/world";
    worldPath += std::to_wstring(worldNumber_);
    worldPath += L".png";

    UIImage worldImg{worldPath};
    worldImg.opacity = 1.0f;
    worldImg.keepAspect = true;

    Entity worldImageEntity = world.Create()
                                  .With<UITransform>(worldImgTr)
                                  .With<UIImage>(worldImg)
                                  .Build();
    ownedEntities_.push_back(worldImageEntity);
    worldUIEntity_ = worldImageEntity;

    // StageSter (Decorations - Drawn behind StageName)
    stageSterEntities_.clear();
    // Order: 4 (Back) -> 3 -> 2 -> 1 (Front, behind UI)
    for (int i = 4; i >= 1; --i) {
        UITransform sterTr;
        sterTr.position = {cfg_StageSelectUI_StageNamePosX.Get(), cfg_StageSelectUI_StageNamePosY.Get()};
        sterTr.size = {800.0f, 800.0f}; // Base size, overridden by Update logic
        sterTr.anchor = {0.5f, 0.5f}; // Center aligned
        sterTr.pivot = {0.5f, 0.5f};

        std::wstring path = L"Assets/Textures/UI/StageSter/stagester";
        path += std::to_wstring(i);
        path += L".png";

        UIImage sterImg{path};
        sterImg.opacity = 0.0f; // Start hidden, updated in OnUpdate
        sterImg.keepAspect = true;

        Entity e = world.Create()
                       .With<UITransform>(sterTr)
                       .With<UIImage>(sterImg)
                       .Build();
        
        stageSterEntities_.push_back(e); // Index 0 = Ster4, Index 3 = Ster1
    }

    // StageName (Create for all stages)
    stageNameEntities_.clear();
    stageNameBaseSize_ = {cfg_StageSelectUI_StageNameSizeW.Get(), cfg_StageSelectUI_StageNameSizeH.Get()};

    for (int i = 1; i <= maxStage_; ++i) {
        UITransform stageNameTr;
        stageNameTr.position = {cfg_StageSelectUI_StageNamePosX.Get(), cfg_StageSelectUI_StageNamePosY.Get()};
        stageNameTr.size = stageNameBaseSize_;
        stageNameTr.anchor = {cfg_StageSelectUI_StageNameAnchorX.Get(), cfg_StageSelectUI_StageNameAnchorY.Get()};
        stageNameTr.pivot = {cfg_StageSelectUI_StageNamePivotX.Get(), cfg_StageSelectUI_StageNamePivotY.Get()};

        std::wstring path = L"Assets/Textures/UI/StageName/stagename";
        path += std::to_wstring(worldNumber_);
        path += std::to_wstring(static_cast<int>(cfg_StageNameWorldDigit.Get()));
        path += std::to_wstring(i);
        path += L".png";

        UIImage stageNameImg{path};
        stageNameImg.opacity = 1.0f; 
        stageNameImg.keepAspect = true;

        Entity e = world.Create()
                       .With<UITransform>(stageNameTr)
                       .With<UIImage>(stageNameImg)
                       .Build();

        stageNameEntities_.push_back(e);
    }

    // Button UI
    const DirectX::XMFLOAT2 btnSize{cfg_StageSelectUI_ButtonSizeW.Get(), cfg_StageSelectUI_ButtonSizeH.Get()};

    UITransform enterBtnTr;
    enterBtnTr.position = {cfg_StageSelectUI_EnterBtnPosX.Get(), cfg_StageSelectUI_EnterBtnPosY.Get()};
    enterBtnTr.size = btnSize;
    enterBtnTr.anchor = {0.0f, 0.0f};
    enterBtnTr.pivot = {0.0f, 0.0f};

    UIImage enterBtnImg{L"Assets/Textures/UI/StageUI/stageui1.png"};
    enterBtnImg.opacity = 1.0f;
    enterBtnImg.keepAspect = true;

    ownedEntities_.push_back(world.Create().With<UITransform>(enterBtnTr).With<UIImage>(enterBtnImg).Build());
    
    // Store base size for scaling (Already set in loop above)
    // stageNameBaseSize_ = stageNameTr.size; 

    UITransform titleBtnTr;
    titleBtnTr.position = {cfg_StageSelectUI_TitleBtnPosX.Get(), cfg_StageSelectUI_TitleBtnPosY.Get()};
    titleBtnTr.size = btnSize;
    titleBtnTr.anchor = {0.0f, 0.0f};
    titleBtnTr.pivot = {0.0f, 0.0f};

    UIImage titleBtnImg{L"Assets/Textures/UI/StageUI/stageui2.png"};
    titleBtnImg.opacity = 1.0f;
    titleBtnImg.keepAspect = true;

    ownedEntities_.push_back(world.Create().With<UITransform>(titleBtnTr).With<UIImage>(titleBtnImg).Build());
}
