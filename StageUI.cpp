/**
 * @file StageUI.cpp
 * @brief StageScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "scenes/StageSelect.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation など

inline static ConfigVar<float> cfg_FadeFrameTime{"Fade.Out", "FadeFrameTime", 0.01f, "フェードアウト1フレームの時間（秒）"};

void StageSlectScene::CreateTextFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 50.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}

void StageSlectScene::CreateStageSelectUI(World &world) {
   
    UITransform CountTransform;
    CountTransform.position = {cfg_UICountPosX,cfg_UICountPosY};
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

    //宇宙船の表示
    UITransform selecttestImgTr;
    selecttestImgTr.position = {350.0f, 50.0f};
    selecttestImgTr.size = {600.0f, 600.0f};
    selecttestImgTr.anchor = {0.0f, 0.0f};
    selecttestImgTr.pivot = {0.0f, 0.0f};

    UIImage selecttestImg{L"./Assets/Textures/Time/SelectTest.png"};
    selecttestImg.opacity = 1.0f;
    selecttestImg.keepAspect = true;

    Entity selecttestImageEntity = world.Create()
                                       .With<UITransform>(selecttestImgTr)
                                       .With<UIImage>(selecttestImg)
                                       .Build();

     ownedEntities_.push_back(selecttestImageEntity);
}
