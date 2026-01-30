/**
 * @file TitleUI.cpp
 * @brief TitleScene �� UI �\�z�ƃe�L�X�g�t�H�[�}�b�g����
 */
#include "pch.h"
#include "scenes/Game.h"
#include "scenes/Title.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation �Ȃ�



void TitleScene::CreateTextNormalFormats() {
    TextSystem::TextFormat normal;
    normal.fontSize = 100.0f;
    normal.fontFamily = L"���C���I";
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

    //�^�C�g�����S
    UITransform titleLogoImgTr;
    titleLogoImgTr.position = {50.0f,10.0f};
    titleLogoImgTr.size = {640.0f, 420.0f};
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
   

    //�X�^�[�g
    UITransform startImgTr;
    startImgTr.position = {50.0f, 440.0f};
    startImgTr.size = {300.0f, 300.0f};
    startImgTr.anchor = {0.0f, 0.0f};
    startImgTr.pivot = {0.0f, 0.0f};

     UIImage startImg{selectPaths[0]};
     startImg.opacity = 1.0f;
    startImg.keepAspect = true;

    Entity startImageEntity = world.Create()
                                      .With<UITransform>(startImgTr)
                                      .With<UIImage>(startImg)
                                      .Build();

    ownedEntities_.push_back(startImageEntity);
    baseMenuEntity_[0] = startImageEntity;

    //���X�^�[�g
    UITransform restartImgTr;
    restartImgTr.position = {50.0f, 520.0f};
    restartImgTr.size = {300.0f, 300.0f};
    restartImgTr.anchor = {0.0f, 0.0f};
    restartImgTr.pivot = {0.0f, 0.0f};

    UIImage restartImg{selectPaths[1]};
    restartImg.opacity = 1.0f;
    restartImg.keepAspect = true;

    Entity restartImageEntity = world.Create()
                                  .With<UITransform>(restartImgTr)
                                  .With<UIImage>(restartImg)
                                  .Build();

    ownedEntities_.push_back(restartImageEntity);
    baseMenuEntity_[1] = restartImageEntity;

     //�I��
    UITransform exitImgTr;
    exitImgTr.position = {50.0f, 600.0f};
    exitImgTr.size = {300.0f, 300.0f};
    exitImgTr.anchor = {0.0f, 0.0f};
    exitImgTr.pivot = {0.0f, 0.0f};

    UIImage exitImg{selectPaths[2]};
    exitImg.opacity = 1.0f;
    exitImg.keepAspect = true;

    Entity exitImageEntity = world.Create()
                                  .With<UITransform>(exitImgTr)
                                  .With<UIImage>(exitImg)
                                  .Build();

    //�e�G���e�B�e�B���i�[
    ownedEntities_.push_back(exitImageEntity);
    baseMenuEntity_[2] = exitImageEntity;

    menuEntity_[0] = world.Create()
                         .With<UITransform>(startImgTr)
                         .With<UIImage>(startImg)
                         .Build();
    ownedEntities_.push_back(menuEntity_[0]);

     menuEntity_[1] = world.Create()
                          .With<UITransform>(restartImgTr)
                          .With<UIImage>(restartImg)
                          .Build();
    ownedEntities_.push_back(menuEntity_[1]);

     menuEntity_[2] = world.Create()
                          .With<UITransform>(exitImgTr)
                          .With<UIImage>(exitImg)
                          .Build();
    ownedEntities_.push_back(menuEntity_[2]);

    for (int i = 0; i < 3; ++i) {
        if (auto *hoverImg = world.TryGet<UIImage>(menuEntity_[i])) {
            hoverImg->filePath = normalPaths[i];
            hoverImg->opacity = (i == currentSelect) ? 1.0f : 0.0f;
        }
        if (auto *baseImg = world.TryGet<UIImage>(baseMenuEntity_[i])) {
            baseImg->opacity = (i == currentSelect) ? 0.0f : 1.0f;
        }
    }
}

