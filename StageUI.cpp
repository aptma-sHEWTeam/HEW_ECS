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

void StageSlectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}
void StageSlectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}


void StageSlectScene::CreateStageSelectUI(World &world) {
   
    //StageNo
    UITransform CountTransform;
    CountTransform.position = {cfg_UICountPosX,cfg_UICountPosY};
    CountTransform.size = {cfg_UICountW, cfg_UICountH};
    CountTransform.anchor = {0.0f, 0.0f};
    CountTransform.pivot = {0.0f, 0.0f};

    UIText CountText{L""};
    CountText.color = {cfg_UICountR, cfg_UICountG, cfg_UICountB, 1.0f};
    CountText.formatId = "hud";

    Entity e = world.Create()
                   .With<UITransform>(CountTransform)
                   .With<UIText>(CountText)
                   .Build();

    StageSelectEntity_ = e;
    ownedEntities_.push_back(e);

    //enter
     UITransform circleTextTr;
    circleTextTr.position = {120.0f, 600.0f};
    circleTextTr.size = {300.0f, 50.0f};
    circleTextTr.anchor = {0.0f, 0.0f};
    circleTextTr.pivot = {0.0f, 0.0f};

    UIText circleText{L"enter"};
    circleText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    circleText.formatId = "hud";

    Entity circleTextEntity = world.Create()
                                  .With<UITransform>(circleTextTr)
                                  .With<UIText>(circleText)
                                  .Build();

    ownedEntities_.push_back(circleTextEntity);

    //title
    UITransform crossTextTr;
    crossTextTr.position = {120.0f, 650.0f};
    crossTextTr.size = {300.0f, 50.0f};
    crossTextTr.anchor = {0.0f, 0.0f};
    crossTextTr.pivot = {0.0f, 0.0f};

    UIText crossText{L"title"};
    crossText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    crossText.formatId = "hud";

    Entity crossTextEntity = world.Create()
                                 .With<UITransform>(crossTextTr)
                                 .With<UIText>(crossText)
                                 .Build();

    ownedEntities_.push_back(crossTextEntity);

    //宇宙船の表示
    /* UITransform selecttestImgTr;
     selecttestImgTr.position = {350.0f, 50.0f};
     selecttestImgTr.size = {600.0f, 600.0f};
     selecttestImgTr.anchor = {0.0f, 0.0f};
     selecttestImgTr.pivot = {0.0f, 0.0f};

     UIImage selecttestImg{L"./Assets/Textures/StageUI/Select1.png"};
     selecttestImg.opacity = 1.0f;
     selecttestImg.keepAspect = true;

     Entity selecttestImageEntity = world.Create()
                                        .With<UITransform>(selecttestImgTr)
                                        .With<UIImage>(selecttestImg)
                                        .Build();

     ownedEntities_.push_back(selecttestImageEntity);*/

     //〇×の表示
     UITransform circleImgTr;
     circleImgTr.position = {-50.0f, 540.0f};
     circleImgTr.size = {200.0f, 200.0f};
     circleImgTr.anchor = {0.0f, 0.0f};
     circleImgTr.pivot = {0.0f, 0.0f};

     UIImage circleImg{L"./Assets/Textures/StageUI/maru.png"};
     circleImg.opacity = 1.0f;
     circleImg.keepAspect = true;

     Entity circleImageEntity = world.Create()
                                    .With<UITransform>(circleImgTr)
                                    .With<UIImage>(circleImg)
                                    .Build();

     ownedEntities_.push_back(circleImageEntity);

     UITransform crossImgTr;
     crossImgTr.position = {-50.0f, 590.0f};
     crossImgTr.size = {200.0f, 200.0f};
     crossImgTr.anchor = {0.0f, 0.0f};
     crossImgTr.pivot = {0.0f, 0.0f};

     UIImage crossImg{L"./Assets/Textures/StageUI/batu.png"};
     crossImg.opacity = 1.0f;
     crossImg.keepAspect = true;

     Entity crossImageEntity = world.Create()
                                   .With<UITransform>(crossImgTr)
                                   .With<UIImage>(crossImg)
                                   .Build();

     ownedEntities_.push_back(crossImageEntity);
}
