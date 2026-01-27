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
        initialStage = std::clamp(progress.selectStage, 1, maxStage_);
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

    // StageName
    UITransform stageNameTr;
    stageNameTr.position = {cfg_StageSelectUI_StageNamePosX.Get(), cfg_StageSelectUI_StageNamePosY.Get()};
    stageNameTr.size = {cfg_StageSelectUI_StageNameSizeW.Get(), cfg_StageSelectUI_StageNameSizeH.Get()};
    stageNameTr.anchor = {cfg_StageSelectUI_StageNameAnchorX.Get(), cfg_StageSelectUI_StageNameAnchorY.Get()};
    stageNameTr.pivot = {cfg_StageSelectUI_StageNamePivotX.Get(), cfg_StageSelectUI_StageNamePivotY.Get()};

    std::wstring stageNamePath = L"Assets/Textures/UI/StageName/stagename";
    stageNamePath += std::to_wstring(worldNumber_);
    stageNamePath += std::to_wstring(static_cast<int>(cfg_StageNameWorldDigit.Get()));
    stageNamePath += std::to_wstring(initialStage);
    stageNamePath += L".png";

    UIImage stageNameImg{stageNamePath};
    stageNameImg.opacity = 1.0f;
    stageNameImg.keepAspect = true;

    Entity stageNameEntity = world.Create()
                                 .With<UITransform>(stageNameTr)
                                 .With<UIImage>(stageNameImg)
                                 .Build();

    StageSelectEntity_ = stageNameEntity;
    ownedEntities_.push_back(stageNameEntity);

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
