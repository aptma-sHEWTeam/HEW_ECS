/**
 * @file TitleUI.cpp
 * @brief TitleScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "scenes/Title.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation など



void TitleScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 100.0f;
    normal.fontFamily = L"メイリオ";
    normal.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", normal);
}

void TitleScene::CreateTitleSelectUI(World &world) {

    //title
    //UITransform crossTextTr;
    //crossTextTr.position = {500.0f, 100.0f};
    //crossTextTr.size = {300.0f, 50.0f};
    //crossTextTr.anchor = {0.0f, 0.0f};
    //crossTextTr.pivot = {0.0f, 0.0f};

    //UIText crossText{L"title"};
    //crossText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    //crossText.formatId = "hud";

    //Entity crossTextEntity = world.Create()
    //                             .With<UITransform>(crossTextTr)
    //                             .With<UIText>(crossText)
    //                             .Build();

    //ownedEntities_.push_back(crossTextEntity);

    //タイトルロゴ
    UITransform titleLogoImgTr;
    titleLogoImgTr.position = {50.0f,-100.0f};
    titleLogoImgTr.size = {700.0f, 500.0f};
    titleLogoImgTr.anchor = {0.0f,0.0f};
    titleLogoImgTr.pivot = {0.0f, 0.0f};

    UIImage titleLogoImg{L"./Assets/Textures/UI/TitleUI/titleLogo.png"};
    titleLogoImg.opacity = 1.0f;
    titleLogoImg.keepAspect = true;

    Entity titleLogoImageEntity = world.Create()
                                   .With<UITransform>(titleLogoImgTr)
                                   .With<UIImage>(titleLogoImg)
                                   .Build();

    ownedEntities_.push_back(titleLogoImageEntity);
   

    //スタート
    UITransform startImgTr;
    startImgTr.position = {50.0f, 400.0f};
    startImgTr.size = {300.0f, 300.0f};
    startImgTr.anchor = {0.0f, 0.0f};
    startImgTr.pivot = {0.0f, 0.0f};

    UIImage startImg{L"./Assets/Textures/UI/TitleUI/title4.png "};
    UIImage startselectImg{L"./Assets/Textures/UI/TitleUI/title3.png "};
    startImg.opacity = 1.0f;
    startImg.keepAspect = true;

    Entity startImageEntity = world.Create()
                                      .With<UITransform>(startImgTr)
                                      .With<UIImage>(startImg)
                                      .Build();

    ownedEntities_.push_back(startImageEntity);

    //リスタート
    UITransform restartImgTr;
    restartImgTr.position = {50.0f, 460.0f};
    restartImgTr.size = {300.0f, 300.0f};
    restartImgTr.anchor = {0.0f, 0.0f};
    restartImgTr.pivot = {0.0f, 0.0f};

    UIImage restartImg{L"./Assets/Textures/UI/TitleUI/title6.png"};
    UIImage restartselectImg{L"./Assets/Textures/UI/TitleUI/title5.png"};
    restartImg.opacity = 1.0f;
    restartImg.keepAspect = true;

    Entity restartImageEntity = world.Create()
                                  .With<UITransform>(restartImgTr)
                                  .With<UIImage>(restartImg)
                                  .Build();

    ownedEntities_.push_back(restartImageEntity);

     //終了
    UITransform exitImgTr;
    exitImgTr.position = {50.0f, 520.0f};
    exitImgTr.size = {300.0f, 300.0f};
    exitImgTr.anchor = {0.0f, 0.0f};
    exitImgTr.pivot = {0.0f, 0.0f};

    UIImage exitImg{L"./Assets/Textures/UI/TitleUI/title2.png"};
    UIImage exitselectImg{L"./Assets/Textures/UI/TitleUI/title1.png"};
    exitImg.opacity = 1.0f;
    exitImg.keepAspect = true;

    Entity exitImageEntity = world.Create()
                                  .With<UITransform>(exitImgTr)
                                  .With<UIImage>(exitImg)
                                  .Build();

    ownedEntities_.push_back(exitImageEntity);
}
