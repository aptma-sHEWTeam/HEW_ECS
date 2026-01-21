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

    // StageNo
    UITransform CountTransform;
    CountTransform.position = {cfg_UICountPosX, cfg_UICountPosY};
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

    // WorldNo
    UITransform worldnoTextTr;
    worldnoTextTr.position = {500.0f, 100.0f};
    worldnoTextTr.size = {300.0f, 50.0f};
    worldnoTextTr.anchor = {0.0f, 0.0f};
    worldnoTextTr.pivot = {0.0f, 0.0f};

    std::wstring worldName;
    switch (worldNumber_) {
        case 1:
            worldName = L"World1";
            break;
        case 2:
            worldName = L"World2";
            break;
        case 3:
            worldName = L"World3";
            break;
        case 4:
            worldName = L"World4";
            break;
        default:
            worldName = L"Unknown World";
            break;
    }

    UIText worldnoText{worldName};
    worldnoText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    worldnoText.formatId = "hud";

    Entity worldnoTextEntity = world.Create()
                                   .With<UITransform>(worldnoTextTr)
                                   .With<UIText>(worldnoText)
                                   .Build();

    ownedEntities_.push_back(worldnoTextEntity);

    // Enter
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

    // Title
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

    // 〇×の表示 (All Worlds)
    UITransform circleImgTr;
    circleImgTr.position = {-50.0f, 540.0f};
    circleImgTr.size = {200.0f, 200.0f};
    circleImgTr.anchor = {0.0f, 0.0f};
    circleImgTr.pivot = {0.0f, 0.0f};

    UIImage circleImg{L"Assets/Textures/StageUI/maru.png"};
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

    UIImage crossImg{L"Assets/Textures/StageUI/batu.png"};
    crossImg.opacity = 1.0f;
    crossImg.keepAspect = true;

    Entity crossImageEntity = world.Create()
                                  .With<UITransform>(crossImgTr)
                                  .With<UIImage>(crossImg)
                                  .Build();

    ownedEntities_.push_back(crossImageEntity);
}
