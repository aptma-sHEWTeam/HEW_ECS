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

inline static ConfigVar<float> cfg_FadeFrameTime{"Fade.Out", "FadeFrameTime", 0.01f, "フェードアウト1フレームの時間（秒）"};

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
    titleLogoImgTr.position = {300.0f,10.0f};
    titleLogoImgTr.size = {700.0f, 700.0f};
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
}