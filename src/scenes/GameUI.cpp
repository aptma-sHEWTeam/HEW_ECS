/**
 * @file GameUI.cpp
 * @brief GameScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "animation/Animation.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"


void GameScene::CreateTextFormats() {
    TextSystem::TextFormat hudFormat;
    hudFormat.fontSize = 24.0f;
    hudFormat.fontFamily = L"メイリオ";
    hudFormat.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hudFormat);

    TextSystem::TextFormat pauseFormat;
    pauseFormat.fontSize = 72.0f;
    pauseFormat.fontFamily = L"メイリオ";
    pauseFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
    pauseFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    textSystem_.CreateTextFormat("pause", pauseFormat);

    TextSystem::TextFormat buttonFormat;
    buttonFormat.fontSize = 20.0f;
    buttonFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
    buttonFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    textSystem_.CreateTextFormat("button", buttonFormat);

    TextSystem::TextFormat panelFormat;
    panelFormat.fontSize = 200.0f;
    textSystem_.CreateTextFormat("panel", panelFormat);

    TextSystem::TextFormat titleFormat;
    titleFormat.fontSize = 20.0f;
    titleFormat.fontFamily = L"メイリオ";
    titleFormat.style = DWRITE_FONT_STYLE_ITALIC;
    titleFormat.alignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
    titleFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    textSystem_.CreateTextFormat("title", titleFormat);
}

void GameScene::CreateUI(World &world, float screenWidth, float screenHeight) {
    Entity canvas = world.Create()
                        .With<UICanvas>()
                        .Build();
    ownedEntities_.push_back(canvas);

    Entity uiRenderSystem = world.Create()
                                .With<UIRenderSystem>()
                                .Build();
    if (auto *renderSys = world.TryGet<UIRenderSystem>(uiRenderSystem)) {
        renderSys->SetTextSystem(&textSystem_);
        renderSys->SetImageSystem(&imageSystem_);
        renderSys->SetScreenSize(screenWidth, screenHeight);
    }
    ownedEntities_.push_back(uiRenderSystem);

    Entity uiInteractionSystem = world.Create()
                                     .With<UIInteractionSystem>()
                                     .Build();
    if (auto *interactionSys = world.TryGet<UIInteractionSystem>(uiInteractionSystem)) {
        interactionSys->SetScreenSize(screenWidth, screenHeight);
    }
    ownedEntities_.push_back(uiInteractionSystem);

   /* UITransform scoreTransform;
    scoreTransform.position = {20.0f, 20.0f};
    scoreTransform.size = {300.0f, 40.0f};
    scoreTransform.anchor = {0.0f, 0.0f};
    scoreTransform.pivot = {0.0f, 0.0f};

    UIText scoreText{L"スコア: 0"};
    scoreText.color = {1.0f, 1.0f, 0.0f, 1.0f};
    scoreText.formatId = "hud";

    Entity scoreEntity = world.Create()
                             .With<UITransform>(scoreTransform)
                             .With<UIText>(scoreText)
                             .Build();
    ownedEntities_.push_back(scoreEntity);*/

    UITransform timeTransform;
    timeTransform.position = {20.0f, 70.0f};
    timeTransform.size = {1000.0f, 80.0f};
    timeTransform.anchor = {0.0f, 0.0f};
    timeTransform.pivot = {0.0f, 0.0f};

    UIText timeText{L"時間: 00:00"};
    timeText.color = {1.0f, 0.0f, 0.0f, 1.0f};
    timeText.formatId = "hud";
    timeText.fontSize = 36.0f;

    Entity timeEntity = world.Create()
                            .With<UITransform>(timeTransform)
                            .With<UIText>(timeText)
                            .Build();
    ownedEntities_.push_back(timeEntity);

      UITransform timerUiTr;
    timerUiTr.position = {-900.0f, -8.0f};
    timerUiTr.size = {2000.0f, 200.0f};
    timerUiTr.anchor = {0.0f, 0.0f};
    timerUiTr.pivot = {0.0f, 0.0f};

    UIImage timerUiImg{L"./Assets/Textures/Time/UI_timer.png"};
    timerUiImg.opacity = 1.0f;
    timerUiImg.keepAspect = true;

    Entity timerUiEntity = world.Create()
                               .With<UITransform>(timerUiTr)
                               .With<UIImage>(timerUiImg)
                               .Build();

    ownedEntities_.push_back(timerUiEntity);

    // タイマー背景
    UITransform timerBgTr;
    timerBgTr.position = {-900.0f, -8.0f};
    timerBgTr.size = {2000.0f, 200.0f};
    timerBgTr.anchor = {0.0f, 0.0f};
    timerBgTr.pivot = {0.0f, 0.0f};

    // 画像
    UIImage timerBgImg;
   // timerBgImg.filePath = {L"./Assets/Textures/Time/Timer_Sprite.png"};
    timerBgImg.keepAspect = true;
    timerBgImg.opacity = 1.0f;

    // 背景エンティティ作成
    Entity timerBackgroundEntity = world.Create()
                                       .With<UITransform>(timerBgTr)
                                       .With<UIImage>(timerBgImg)
                                       .Build();

    ownedEntities_.push_back(timerBackgroundEntity);

    UITransform timerImgTr;
    timerImgTr.position = {-900.0f, -8.0f};
    timerImgTr.size = {2000.0f, 200.0f};
    timerImgTr.anchor = {0.0f, 0.0f};
    timerImgTr.pivot = {0.0f, 0.0f};

    UIImage timerImg{L"./Assets/Textures/Time/timer/timer_00000.png"};
    timerImg.opacity = 1.0f;
    timerImg.keepAspect = true;

    Entity timerImageEntity = world.Create()
                                  .With<UITransform>(timerImgTr)
                                  .With<UIImage>(timerImg)
                                  .Build();

    ownedEntities_.push_back(timerImageEntity);


#ifdef _DEBUG
    UITransform fpsTransform;
    fpsTransform.position = {-20.0f, 20.0f};
    fpsTransform.size = {200.0f, 40.0f};
    fpsTransform.anchor = {1.0f, 0.0f};
    fpsTransform.pivot = {1.0f, 0.0f};

    UIText fpsText{L"FPS: 0.0"};
    fpsText.color = {0.0f, 1.0f, 0.0f, 1.0f};
    fpsText.formatId = "hud";

    Entity fpsEntity = world.Create()
                           .With<UITransform>(fpsTransform)
                           .With<UIText>(fpsText)
                           .Build();
    ownedEntities_.push_back(fpsEntity);
#endif // !_DEBUG


    UIText stageText[2];

    stageText[0].text = {L"Room : 0/"};
    stageText[0].color = {1.0f, 0.5f, 0.0f, 1.0f};
    stageText[0].formatId = "hud";
    float stagetextSize0 = 3.9f * sizeof(stageText[0].text);

    stageText[1].text = {L"0"};
    stageText[1].color = {1.0f, 0.5f, 0.0f, 1.0f};
    stageText[1].formatId = "hud";

    UITransform stageTransform[2];
    stageTransform[0].position = {800.0f, 110.0f};
    stageTransform[0].size = {130.0f, 40.0f};
    stageTransform[0].anchor = {0.0f, 0.0f};
    stageTransform[0].pivot = {1.0f, 0.0f};

    stageTransform[1].position = {stageTransform[0].position.x + stagetextSize0, stageTransform[0].position.y};
    stageTransform[1].size = stageTransform[0].size;
    stageTransform[1].anchor = stageTransform[0].anchor;
    stageTransform[1].pivot = stageTransform[0].pivot;

    Entity stageEntity[2];

    for (int i = 0; i < 2; i++) {
        stageEntity[i] = world.Create()
                             .With<UITransform>(stageTransform[i])
                             .With<UIText>(stageText[i])
                             .Build();
        ownedEntities_.push_back(stageEntity[i]);
    }

    UITransform pauseTransform;
    pauseTransform.position = {0.0f, 0.0f};
    pauseTransform.size = {0.0f, 0.0f};
    pauseTransform.anchor = {0.5f, 0.5f};
    pauseTransform.pivot = {0.5f, 0.5f};

    UIText pauseText{L""};
    pauseText.color = {1.0f, 0.0f, 0.0f, 1.0f};
    pauseText.formatId = "pause";

    Entity pauseEntity = world.Create()
                             .With<UITransform>(pauseTransform)
                             .With<UIText>(pauseText)
                             .Build();
    ownedEntities_.push_back(pauseEntity);

    Entity uiUpdater = world.Create()
                           .With<GameUIUpdater>()
                           .Build();
    if (auto *updater = world.TryGet<GameUIUpdater>(uiUpdater)) {
        //updater->scoreTextEntity_ = scoreEntity;
        updater->timeTextEntity_ = timeEntity;
#ifdef _DEBUG
        updater->fpsTextEntity_ = fpsEntity;
#endif // !_DEBUG
        updater->pauseTextEntity_ = pauseEntity;
        updater->stageTextEntity_[0] = stageEntity[0];
        updater->stageTextEntity_[1] = stageEntity[1];
        updater->timerBackgroundEntity_ = timerBackgroundEntity;
        updater->timeImageEntity_ = timerImageEntity;
        updater->timerUiEntity_ = timerUiEntity;
    }
    ownedEntities_.push_back(uiUpdater);

    UITransform UIanimation;
    UIanimation.position = {0.0f,0.0f};
    UIanimation.size = {1280.0f, 720.0f};
    UIanimation.anchor = {0.0f, 0.0f};
    UIanimation.pivot = {0.0f, 0.0f};

    UIImage ani{L"./Assets/Textures/Fade/tex_fadekorigori.png"};
    ani.opacity = 1.0f;
    ani.keepAspect = false;
    
    SpriteSheetAnimation anim;
    anim.frameCount = 18;
    anim.frameTime = 0.08f;
    anim.isPlaying = false;

   //Entity animation = world.Create()
   //                        .With<UITransform>(UIanimation)
   //                        .With<UIImage>(ani)
   //                        .With<UVAnimation>()
   //                        .With<SpriteSheetAnimation>(anim)
   //                        .Build();

    ownedEntities_.push_back(animation);
   fadeAnimationEntity_ = animation;
}
