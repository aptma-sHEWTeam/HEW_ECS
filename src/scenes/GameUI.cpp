/**
 * @file GameUI.cpp
 * @brief GameScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"

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

    UITransform scoreTransform;
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
    ownedEntities_.push_back(scoreEntity);

    UITransform timeTransform;
    timeTransform.position = {20.0f, 70.0f};
    timeTransform.size = {300.0f, 40.0f};
    timeTransform.anchor = {0.0f, 0.0f};
    timeTransform.pivot = {0.0f, 0.0f};

    UIText timeText{L"時間: 00:00"};
    timeText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    timeText.formatId = "hud";

    Entity timeEntity = world.Create()
                            .With<UITransform>(timeTransform)
                            .With<UIText>(timeText)
                            .Build();
    ownedEntities_.push_back(timeEntity);

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

    UITransform stageTransform;
    stageTransform.position = {150.0f, 120.0f};
    stageTransform.size = {130.0f, 40.0f};
    stageTransform.anchor = {0.0f, 0.0f};
    stageTransform.pivot = {1.0f, 0.0f};

    UIText stageText{L"FLOOR: 1"};
    stageText.color = {1.0f, 0.5f, 0.0f, 1.0f};
    stageText.formatId = "hud";

    Entity stageEntity = world.Create()
                             .With<UITransform>(stageTransform)
                             .With<UIText>(stageText)
                             .Build();
    ownedEntities_.push_back(stageEntity);

    UIText titleText[2];

    titleText[0].text     = {L"Fricker Game:"};
    titleText[0].color    = {1.0f, 0.0f, 1.0f, 1.0f};
    titleText[0].formatId = "title";
    float titletextSize0  = 3.9f * sizeof(titleText[0].text);

    titleText[1].text     = {L"Proto Type"};
    titleText[1].color    = {0.7f, 0.0f, 0.7f, 1.0f};
    titleText[1].formatId = "title";

    UITransform titleTransform[2];

    titleTransform[0].position = {800.0f, 60.0f};
    titleTransform[0].size     = {300.0f, 30.0f};
    titleTransform[0].anchor   = {0.0f,    0.0f};
    titleTransform[0].pivot    = {0.0f,    0.0f};

    titleTransform[1].position = {titleTransform[0].position.x + titletextSize0 , titleTransform[0].position.y};
    titleTransform[1].size     = titleTransform[0].size;
    titleTransform[1].anchor   = titleTransform[0].anchor;
    titleTransform[1].pivot    = titleTransform[0].pivot;

    for (int i = 0 ; i < 2; i++)
    {
        Entity titleEntity[2];

        titleEntity[i] = world.Create()
                                .With<UITransform>(titleTransform[i])
                                .With<UIText>(titleText[i])
                                .Build();
        ownedEntities_.push_back(titleEntity[i]);
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
        updater->scoreTextEntity_ = scoreEntity;
        updater->timeTextEntity_ = timeEntity;
        updater->fpsTextEntity_ = fpsEntity;
        updater->pauseTextEntity_ = pauseEntity;
        updater->stageTextEntity_ = stageEntity;
    }
    ownedEntities_.push_back(uiUpdater);
}
