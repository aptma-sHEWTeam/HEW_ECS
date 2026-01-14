/**
 * @file StageUI.cpp
 * @brief StageScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "scenes/World1_StageSelect.h"
#include "scenes/World2_StageSelect.h"
#include "scenes/World3_StageSelect.h"
#include "scenes/World4_StageSelect.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation など

inline static ConfigVar<float> cfg_FadeFrameTime{"Fade.Out", "FadeFrameTime", 0.01f, "フェードアウト1フレームの時間（秒）"};

void World1_StageSlectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}
void World1_StageSlectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}


void World1_StageSlectScene::CreateStageSelectUI(World &world) {
   
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

    //WorldNo
    UITransform worldnoTextTr;
    worldnoTextTr.position = {500.0f, 100.0f};
    worldnoTextTr.size = {300.0f, 50.0f};
    worldnoTextTr.anchor = {0.0f, 0.0f};
    worldnoTextTr.pivot = {0.0f, 0.0f};

    UIText worldnoText{L"World1"};
    worldnoText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    worldnoText.formatId = "hud";

    Entity worldnoTextEntity = world.Create()
                                  .With<UITransform>(worldnoTextTr)
                                  .With<UIText>(worldnoText)
                                  .Build();

    ownedEntities_.push_back(worldnoTextEntity);
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

void World2_StageSelectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}

void World2_StageSelectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}

void World2_StageSelectScene::CreateStageSelectUI(World &world) {

    //StageNo
    UITransform CountTransform;
    CountTransform.position = {1000.0f,500.0f};
    CountTransform.size = {200.0f, 40.0f};
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

     //WorldNo
    UITransform worldnoTextTr;
    worldnoTextTr.position = {500.0f, 100.0f};
    worldnoTextTr.size = {300.0f, 50.0f};
    worldnoTextTr.anchor = {0.0f, 0.0f};
    worldnoTextTr.pivot = {0.0f, 0.0f};

    UIText worldnoText{L"World2"};
    worldnoText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    worldnoText.formatId = "hud";

    Entity worldnoTextEntity = world.Create()
                                   .With<UITransform>(worldnoTextTr)
                                   .With<UIText>(worldnoText)
                                   .Build();

    ownedEntities_.push_back(worldnoTextEntity);

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
}

void World3_StageSelectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}

void World3_StageSelectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}

void World3_StageSelectScene::CreateStageSelectUI(World &world) {

    //StageNo
    UITransform CountTransform;
    CountTransform.position = {1000.0f, 500.0f};
    CountTransform.size = {200.0f, 40.0f};
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

     //WorldNo
    UITransform worldnoTextTr;
    worldnoTextTr.position = {500.0f, 100.0f};
    worldnoTextTr.size = {300.0f, 50.0f};
    worldnoTextTr.anchor = {0.0f, 0.0f};
    worldnoTextTr.pivot = {0.0f, 0.0f};

    UIText worldnoText{L"World3"};
    worldnoText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    worldnoText.formatId = "hud";

    Entity worldnoTextEntity = world.Create()
                                   .With<UITransform>(worldnoTextTr)
                                   .With<UIText>(worldnoText)
                                   .Build();

    ownedEntities_.push_back(worldnoTextEntity);

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
}

void World4_StageSelectScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 24.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}

void World4_StageSelectScene::CreateTextStageNoFormats() {
    TextSystem::TextFormat hud;
    hud.fontSize = 45.0f;
    hud.fontFamily = L"メイリオ";
    hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hud);
}

void World4_StageSelectScene::CreateStageSelectUI(World &world) {

    //StageNo
    UITransform CountTransform;
    CountTransform.position = {1000.0f, 500.0f};
    CountTransform.size = {200.0f, 40.0f};
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

    //WorldNo
    UITransform worldnoTextTr;
    worldnoTextTr.position = {500.0f, 100.0f};
    worldnoTextTr.size = {300.0f, 50.0f};
    worldnoTextTr.anchor = {0.0f, 0.0f};
    worldnoTextTr.pivot = {0.0f, 0.0f};

    UIText worldnoText{L"World4"};
    worldnoText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    worldnoText.formatId = "hud";

    Entity worldnoTextEntity = world.Create()
                                   .With<UITransform>(worldnoTextTr)
                                   .With<UIText>(worldnoText)
                                   .Build();

    ownedEntities_.push_back(worldnoTextEntity);

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
}
