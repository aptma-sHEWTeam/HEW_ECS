/**
 * @file GameUI.cpp
 * @brief GameScene の UI 構築とテキストフォーマット実装
 */
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/CountUIComponent.h"
#include "components/UIImageComponents.h"
#include "components/StageComponents.h"
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation など
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"
#include "graphics/StageSave.h"
#include <algorithm>

inline static ConfigVar<float> cfg_FadeFrameTime{"Fade.Out", "FadeFrameTime", 0.01f, "フェードアウト1フレームの時間（秒）"};

void GameScene::CreateTextFormats() {
    TextSystem::TextFormat hudFormat;
    hudFormat.fontSize = 24.0f;
    hudFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    hudFormat.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hudFormat);

    TextSystem::TextFormat pauseFormat;
    pauseFormat.fontSize = 72.0f;
    pauseFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
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
    titleFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    titleFormat.style = DWRITE_FONT_STYLE_ITALIC;
    titleFormat.alignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
    titleFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    textSystem_.CreateTextFormat("title", titleFormat);

    // デス数・ランキング表示用
    TextSystem::TextFormat youFmt;
    youFmt.fontSize = 45.0f;
    youFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    textSystem_.CreateTextFormat("you", youFmt);

    TextSystem::TextFormat numFmt;
    numFmt.fontSize = 75.0f;
    numFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
    textSystem_.CreateTextFormat("num", numFmt);

    TextSystem::TextFormat redFmt;
    redFmt.fontSize = 30.0f;
    redFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    textSystem_.CreateTextFormat("redText", redFmt);

    TextSystem::TextFormat topNumFmt;
    topNumFmt.fontSize = 55.0f;
    topNumFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
    textSystem_.CreateTextFormat("topNum", topNumFmt);

    TextSystem::TextFormat rankFmt;
    rankFmt.fontSize = 35.0f;
    rankFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    textSystem_.CreateTextFormat("rank", rankFmt);
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
    timeTransform.position = {78.0f, 68.0f};
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

    UITransform timerImgTr;
    timerImgTr.position = {-869.0f, -6.0f};
    timerImgTr.size = {2000.0f, 200.0f};
    timerImgTr.anchor = {0.0f, 0.0f};
    timerImgTr.pivot = {0.0f, 0.0f};

    UIImage timerImg{L"./Assets/Textures/Time/Timer_Sprite.png"};
    timerImg.opacity = 1.0f;
    timerImg.keepAspect = true;

    SpriteSheetAnimation timerAnime;
    timerAnime.frameCount = 131;
    timerAnime.frameTime = 0.1f;
    timerAnime.columns = 16;
    timerAnime.rows = 16;

    timerAnime.isLooping = true;
    timerAnime.isPlaying = true;
    Entity timerImageEntity = world.Create()
                                  .With<UITransform>(timerImgTr)
                                  .With<UIImage>(timerImg)
                                  .With<UVAnimation>()
                                  .With<SpriteSheetAnimation>(timerAnime)
                                  .Build();

    ownedEntities_.push_back(timerImageEntity);

    UITransform warningOverlayTransform;
    warningOverlayTransform.position = {0.0f, 0.0f};
    warningOverlayTransform.size = {screenWidth, screenHeight};
    warningOverlayTransform.anchor = {0.0f, 0.0f};
    warningOverlayTransform.pivot = {0.0f, 0.0f};

    UIPanel warningOverlay{{1.0f, 0.0f, 0.0f, 0.0f}};
    warningOverlay.visible = false;

    Entity warningOverlayEntity = world.Create()
                                      .With<UITransform>(warningOverlayTransform)
                                      .With<UIPanel>(warningOverlay)
                                      .Build();
    ownedEntities_.push_back(warningOverlayEntity);

    UITransform warningTextTransform;
    warningTextTransform.position = {0.0f, -140.0f};
    warningTextTransform.size = {800.0f, 120.0f};
    warningTextTransform.anchor = {0.5f, 0.5f};
    warningTextTransform.pivot = {0.5f, 0.5f};

    UIText warningText{L""};
    warningText.color = {1.0f, 1.0f, 0.2f, 1.0f};
    warningText.formatId = "pause";

    Entity warningTextEntity = world.Create()
                                   .With<UITransform>(warningTextTransform)
                                   .With<UIText>(warningText)
                                   .Build();
    ownedEntities_.push_back(warningTextEntity);

    UITransform startImgTr;
    startImgTr.position = {520.0f, 85.0f};
    startImgTr.size = {250.0f, 250.0f};
    startImgTr.anchor = {0.0f, 0.0f};
    startImgTr.pivot = {0.0f, 0.0f};

    UIImage startImg{L"Assets/Textures/UI/StageUI/charge.png"};
    startImg.opacity = 1.0f;
    startImg.keepAspect = true;

    Entity startImageEntity = world.Create()
                                  .With<UITransform>(startImgTr)
                                  .With<UIImage>(startImg)
                                  .Build();
    ownedEntities_.push_back(startImageEntity);

    UITransform barTr;
    barTr.position = {520.0f, 335.0f};
    barTr.size = {0.0f, 20.0f};
    barTr.anchor = {0.0f, 0.0f};
    barTr.pivot = {0.0f, 0.0f};

    UIPanel barPanel;
    barPanel.color = {1.0f, 0.0f, 0.0f, 1.0f};

    Entity startChargeBarEntity = world.Create()
                                    .With<UITransform>(barTr)
                                    .With<UIPanel>(barPanel)
                                    .Build();
    ownedEntities_.push_back(startChargeBarEntity);

    /* UITransform starttimeTransform;
    starttimeTransform.position = {600.0f, 250.0f};
    starttimeTransform.size = {300.0f, 300.0f};
    starttimeTransform.anchor = {0.0f, 0.0f};
    starttimeTransform.pivot = {0.0f, 0.0f};

    UIText starttimeText{L"rdy : 00:00"};
    starttimeText.color = {1.0f, 0.0f, 0.0f, 1.0f};
    starttimeText.formatId = "hud";
    starttimeText.fontSize = 36.0f;

    Entity starttime = world.Create()
                           .With<UITransform>(starttimeTransform)
                           .With<UIText>(starttimeText)
                           .Build();

    ownedEntities_.push_back(starttime);*/

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

    UITransform roomImgTr;
    roomImgTr.position = {1030.0f, 42.0f};
    roomImgTr.size = {200.0f, 80.0f};
    roomImgTr.anchor = {0.0f, 0.0f};
    roomImgTr.pivot = {0.0f, 0.0f};

    UIImage roomImg{L"./Assets/Textures/RoomNo/UI_Japanese_colored.png"};
    roomImg.opacity = 1.0f;
    roomImg.keepAspect = true;

    Entity roomImageEntity = world.Create()
                                 .With<UITransform>(roomImgTr)
                                 .With<UIImage>(roomImg)
                                 .Build();

    ownedEntities_.push_back(roomImageEntity);

    UIText stageText[2];

    stageText[0].text = {L"Room : 0"};
    stageText[0].color = {0.0f, 0.5f, 1.0f, 1.0f};
    stageText[0].formatId = "roomNumber";
    stageText[0].outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
    stageText[0].outlineThickness = 1.0f;
    stageText[0].fillTexturePath = L"./Assets/Textures/RoomNo/UI-2-color.png";
    float stagetextSize0 = 3.9f * sizeof(stageText[0].text);

    stageText[1].text = {L"0"};
    stageText[1].color = {0.0f, 0.5f, 1.0f, 1.0f};
    stageText[1].formatId = "roomNumber";
    stageText[1].outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
    stageText[1].outlineThickness = 1.0f;
    stageText[1].fillTexturePath = L"./Assets/Textures/RoomNo/UI-2-color.png";

    UITransform stageTransform[2];
    stageTransform[0].position = {1455.0f, 65.0};
    stageTransform[0].size = {210.0f, 100.0f};
    stageTransform[0].anchor = {0.0f, 0.0f};
    stageTransform[0].pivot = {1.0f, 0.0f};

    stageTransform[1].position = {stageTransform[0].position.x - 65.0f, stageTransform[0].position.y};
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
    pauseTransform.position = {60.0f, -120.0f}; // 左側の中央よりやや上
    pauseTransform.size = {0.0f, 0.0f}; // 表示時に拡大する
    pauseTransform.anchor = {0.0f, 0.5f};
    pauseTransform.pivot = {0.0f, 0.5f};

    UIImage pauseImg{L"./Assets/Textures/UI/PausedUI/PAUSED.png"};
    pauseImg.opacity = 0.0f;
    pauseImg.keepAspect = true;

    Entity pauseEntity = world.Create()
                             .With<UITransform>(pauseTransform)
                             .With<UIImage>(pauseImg)
                             .Build();
    ownedEntities_.push_back(pauseEntity);

    // 縦線 LeftLin.png
    UITransform lineTr;
    lineTr.position = {80.0f, 80.0f}; // ボタンの横
    lineTr.size = {0.0f, 0.0f};       // isPaused時に展開
    lineTr.anchor = {0.0f, 0.5f};
    lineTr.pivot = {0.0f, 0.5f};
    UIImage lineImg{L"./Assets/Textures/UI/PausedUI/LeftLin.png"};
    lineImg.opacity = 0.0f;
    lineImg.keepAspect = true;
    Entity lineEntity = world.Create()
                            .With<UITransform>(lineTr)
                            .With<UIImage>(lineImg)
                            .Build();
    ownedEntities_.push_back(lineEntity);

    // 選択インジケーター select.png
    UITransform selectTr;
    selectTr.position = {0.0f, 0.0f};
    selectTr.size = {0.0f, 0.0f}; 
    selectTr.anchor = {0.0f, 0.5f};
    selectTr.pivot = {0.0f, 0.5f};
    UIImage selectImg{L"./Assets/Textures/UI/PausedUI/select.png"};
    selectImg.opacity = 0.0f;
    selectImg.keepAspect = true;
    Entity selectEntity = world.Create()
                            .With<UITransform>(selectTr)
                            .With<UIImage>(selectImg)
                            .Build();
    ownedEntities_.push_back(selectEntity);

    UITransform pauseMenuPanelTr;
    pauseMenuPanelTr.position = {0.0f, 0.0f};
    pauseMenuPanelTr.size = {screenWidth, screenHeight};
    pauseMenuPanelTr.anchor = {0.5f, 0.5f};
    pauseMenuPanelTr.pivot = {0.5f, 0.5f};

    UIPanel pauseMenuPanel;
    pauseMenuPanel.color = {0.0f, 0.0f, 0.0f, 0.55f};
    pauseMenuPanel.visible = false;

    Entity pauseMenuPanelEntity = world.Create()
                                     .With<UITransform>(pauseMenuPanelTr)
                                     .With<UIPanel>(pauseMenuPanel)
                                     .Build();
    ownedEntities_.push_back(pauseMenuPanelEntity);

    auto createPauseButton = [&](const std::wstring &imagePath, float yOffset) {
        UITransform tr;
        tr.position = {120.0f, yOffset}; // 線の右側
        tr.size = {0.0f, 0.0f};
        tr.anchor = {0.0f, 0.5f};
        tr.pivot = {0.0f, 0.5f};

        UIImage img{imagePath};
        img.keepAspect = true;
        img.opacity = 0.0f; // 初期は非表示

        UIButton btn;
        btn.enabled = false;

        Entity e = world.Create().With<UITransform>(tr).With<UIButton>(btn).With<UIImage>(img).Build();
        ownedEntities_.push_back(e);
        return e;
    };

    Entity pauseResumeBtn = createPauseButton(L"./Assets/Textures/UI/PausedUI/BackGame.png", -20.0f);
    Entity pauseRetryBtn  = createPauseButton(L"./Assets/Textures/UI/PausedUI/Retray.png", 60.0f);
    Entity pauseSelectBtn = createPauseButton(L"./Assets/Textures/UI/PausedUI/StageSelect.png", 140.0f);
    Entity pauseTitleBtn  = createPauseButton(L"./Assets/Textures/UI/PausedUI/BackTitle.png", 220.0f);
    Entity pauseOptionsBtn = Entity(); // 無効化
    Entity pauseQuitBtn = Entity();    // 無効化

    // 追加: ステージクリア表示（最初は非表示＝空文字）
    UITransform clearTransform;
    clearTransform.position = {0.0f, 140.0f}; // 画面中央より少し下
    clearTransform.size = {900.0f, 140.0f};
    clearTransform.anchor = {0.5f, 0.5f};
    clearTransform.pivot = {0.5f, 0.5f};

    UIText clearText{L""};
    clearText.color = {1.0f, 1.0f, 0.2f, 1.0f};
    clearText.formatId = "pause"; // 大きい中央寄せフォントを流用

    Entity clearEntity = world.Create()
                             .With<UITransform>(clearTransform)
                             .With<UIText>(clearText)
                             .Build();
    ownedEntities_.push_back(clearEntity);
    stageClearTextEntity_ = clearEntity;

    // ==========================================
    // ランキング・デスカウントUI要素の追加
    // ==========================================
    
    // 【左側】 You (画像) + デス数 (白) + death (画像)
    float youBaseX = 120.0f;
    float youBaseY = 240.0f;

    UITransform youTr;
    youTr.position = {youBaseX, youBaseY};
    youTr.size = {120.0f, 60.0f}; // 画像サイズに合わせて調整可
    UIImage youImg{L"./Assets/Textures/UI/EndRankUI/You.png"};
    youImg.opacity = 1.0f;
    youImg.keepAspect = true;
    Entity youEnt = world.Create().With<UITransform>(youTr).With<UIImage>(youImg).Build();
    ownedEntities_.push_back(youEnt);

    UITransform currDeathTr;
    currDeathTr.position = {youBaseX + 130.0f, youBaseY - 30.0f};
    currDeathTr.size = {120.0f, 100.0f};
    UIText currDeathText{L"0"};
    currDeathText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    currDeathText.formatId = "num";
    Entity currDeathEnt = world.Create().With<UITransform>(currDeathTr).With<UIText>(currDeathText).Build();
    ownedEntities_.push_back(currDeathEnt);

    UITransform currDeathRedTr;
    currDeathRedTr.position = {youBaseX + 220.0f, youBaseY + 5.0f};
    currDeathRedTr.size = {160.0f, 50.0f}; // 画像サイズに合わせて調整可
    UIImage deathImg{L"./Assets/Textures/UI/EndRankUI/Death.png"};
    deathImg.opacity = 1.0f;
    deathImg.keepAspect = true;
    Entity currDeathRedEnt = world.Create().With<UITransform>(currDeathRedTr).With<UIImage>(deathImg).Build();
    ownedEntities_.push_back(currDeathRedEnt);


    // 【右側】 Top 3 ランキング (1st 23 death ...)
    float rankBaseX = screenWidth - 350.0f; // もっと右に寄せる
    float rankBaseY = 240.0f;
    float yOffset = 85.0f;

    int pss = 0;
    world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
        pss = GetPssNumber(sp.worldCount, sp.currentStage);
    });
    if (pss <= 0) {
        pss = StageSave::GetLastSavedPss();
    }
    std::vector<int> topDeaths = StageSave::GetTopDeaths(pss);
    std::wstring topDeathLabels[3] = {L"--", L"--", L"--"};
    for (size_t i = 0; i < topDeaths.size() && i < 3; ++i) {
        topDeathLabels[i] = std::to_wstring(topDeaths[i]);
    }

    std::wstring suffixStrs[] = { L"./Assets/Textures/UI/EndRankUI/1st.png", L"./Assets/Textures/UI/EndRankUI/2nd.png", L"./Assets/Textures/UI/EndRankUI/3rd.png" };

    for (size_t i = 0; i < 3; ++i) {
        float curY = rankBaseY + (i * yOffset);
        
        // 1st / 2nd / 3rd (画像)
        UITransform sufTr;
        sufTr.position = {rankBaseX, curY};
        sufTr.size = {80.0f, 60.0f};
        UIImage sufImg{suffixStrs[i]};
        sufImg.opacity = 1.0f;
        sufImg.keepAspect = true;
        Entity sufEnt = world.Create().With<UITransform>(sufTr).With<UIImage>(sufImg).Build();
        ownedEntities_.push_back(sufEnt);

        // 数値 (白・アウトライン付テキスト)
        UITransform topNumTr;
        topNumTr.position = {rankBaseX + 90.0f, curY - 25.0f};
        topNumTr.size = {120.0f, 80.0f};
        UIText topNumText{topDeathLabels[i]};
        topNumText.color = {1.0f, 1.0f, 1.0f, 1.0f}; // 白字
        topNumText.formatId = "topNum";
        topNumText.outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
        topNumText.outlineThickness = 1.0f;
        Entity topNumEnt = world.Create().With<UITransform>(topNumTr).With<UIText>(topNumText).Build();
        ownedEntities_.push_back(topNumEnt);

        // death (画像)
        UITransform suffixRedTr;
        suffixRedTr.position = {rankBaseX + 160.0f, curY + 5.0f};
        suffixRedTr.size = {160.0f, 50.0f};
        UIImage suffixRedImg{L"./Assets/Textures/UI/EndRankUI/Death.png"};
        suffixRedImg.opacity = 1.0f;
        suffixRedImg.keepAspect = true;
        Entity suffixRedEnt = world.Create().With<UITransform>(suffixRedTr).With<UIImage>(suffixRedImg).Build();
        ownedEntities_.push_back(suffixRedEnt);
    }
    // ==========================================

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
        updater->pauseLineEntity_ = lineEntity; 
        updater->pauseSelectIndicatorEntity_ = selectEntity; // 新規追加
        updater->stageTextEntity_[0] = stageEntity[0];
        updater->stageTextEntity_[1] = stageEntity[1];
        //  updater->numEntity_ = numEntity;
        updater->timeImageEntity_ = timerImageEntity;
        updater->startplayer_ = startImageEntity;
        updater->startChargeBarEntity_ = startChargeBarEntity;
        updater->warningOverlayEntity_ = warningOverlayEntity;
        updater->currDeathEntity_ = currDeathEnt;
        //updater->warningTextEntity_ = warningTextEntity;

        updater->pauseMenuPanelEntity_ = pauseMenuPanelEntity;
        updater->pauseResumeButtonEntity_ = pauseResumeBtn;
        updater->pauseRetryButtonEntity_ = pauseRetryBtn;
        updater->pauseTitleButtonEntity_ = pauseTitleBtn;
        updater->pauseStageSelectButtonEntity_ = pauseSelectBtn;
        updater->pauseOptionsButtonEntity_ = pauseOptionsBtn;
        updater->pauseQuitButtonEntity_ = pauseQuitBtn;
        updater->pauseMenuButtonSize_ = {300.0f, 50.0f}; // 画像ごとのサイズを大まかに
        updater->pauseTitleImgSize_ = {400.0f, 100.0f};  // PAUSEDのサイズ
        updater->pauseLineImgSize_ = {15.0f, 350.0f};    // LeftLinのサイズ
    }
    ownedEntities_.push_back(uiUpdater);

    {
        World *wptr = &world;

        if (auto *b = world.TryGet<UIButton>(pauseResumeBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; });
            };
        }
        if (auto *b = world.TryGet<UIButton>(pauseRetryBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; s.resetDone = false; });
                if (wptr->IsAlive(playerEntity_)) {
                    ResetPlayerToStart(*wptr, playerEntity_, true);
                }
                pendingRespawn_ = false;
                g_respawnPending = false;
                respawnTimer_ = 0.0f;
                deathFadeVisible_ = false;
                StartFadeInNormal(*wptr);
            };
        }
        if (auto *b = world.TryGet<UIButton>(pauseTitleBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; });
                BeginStageSelectFade("Title", *wptr);
            };
        }
        if (auto *b = world.TryGet<UIButton>(pauseOptionsBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; });
                if (auto *mgr = ServiceLocator::TryGet<SceneManager>()) {
                    mgr->ChangeSceneWithTransition("Options", *wptr, TransitionDirection::Right);
                }
            };
        }
        if (auto *b = world.TryGet<UIButton>(pauseSelectBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; });
                    if (auto *mgr = ServiceLocator::TryGet<SceneManager>()) {
                        int worldIndex = 1;
                        wptr->ForEach<StageProgress>([&](Entity, StageProgress &sp) { worldIndex = sp.worldCount; });
                        worldIndex = std::clamp(worldIndex, 1, 4);
                        std::string sceneName = "World" + std::to_string(worldIndex) + "_StageSelect";
                        BeginStageSelectFade(sceneName, *wptr);
                    }
            };
        }
        if (auto *b = world.TryGet<UIButton>(pauseQuitBtn)) {
            b->onClick = [this, wptr]() {
                wptr->ForEach<GameStatus>([](Entity, GameStatus &s) { s.isPaused = false; });
                PostQuitMessage(0);
            };
        }
    }

    /* UITransform UIanimation;
    UIanimation.position = {0.0f, -280.0f};
    UIanimation.size = {1280.0f, 1280.0f};
    UIanimation.anchor = {0.0f, 0.0f};
    UIanimation.pivot = {0.0f, 0.0f};

    UIImage ani{L"./Assets/Textures/Fade/tex_fadekorigori.png"};
    ani.opacity = 1.0f;
    ani.keepAspect = false;

    SpriteSheetAnimation anim;
    anim.frameCount = 18;
    anim.frameTime = 0.08f;
    anim.StartAnimation();
   Entity animation = world.Create()
                           .With<UITransform>(UIanimation)
                           .With<UIImage>(ani)
                           .With<UVAnimation>()
                           .With<SpriteSheetAnimation>(anim)
                           .Build();

    ownedEntities_.push_back(animation);
   fadeAnimationEntity_ = animation;*/
    float fadeFrameTime = cfg_FadeFrameTime;

    UITransform FadeAnimation;
    FadeAnimation.position = {0.0f, 0.0f};
    FadeAnimation.size = {1280.0f, 720.0f};
    FadeAnimation.anchor = {0.0f, 0.0f};
    FadeAnimation.pivot = {0.0f, 0.0f};

    UIImage fade{L"./Assets/Textures/Fade/tex_fade.png"};
    fade.opacity = 1.0f;
    fade.keepAspect = true;
    fade.aspectFill = true;
    fade.overlay = true;

    SpriteSheetDesc fadeDesc = SpriteSheetDesc::Grid(
        AnimationConfig::UI::FadeFrames,
        AnimationConfig::UI::FadeCols,
        fadeFrameTime,
        /*loop*/ false);
    fadeDesc.playOnStart = false;

    Entity fadeOutAnimation = world.Create()
                                  .With<UITransform>(FadeAnimation)
                                  .With<UIImage>(fade)
                                  .Build();
    AnimationTools::AddSpriteSheet(world, fadeOutAnimation, fadeDesc);

    ownedEntities_.push_back(fadeOutAnimation);
    fadeAnimationEntity_ = fadeOutAnimation;

    // 死亡専用フェードアウト
    // 元の tex_fadekorigori.png (77760x4320) は巨大すぎてパフォーマンスに悪影響があるため、
    // tex_fade.png (通常のフェード用) で代用する。
    UIImage deathFade{L"./Assets/Textures/Fade/tex_fadekorigori.png"};
    deathFade.opacity = 1.0f;
    deathFade.keepAspect = true;
    deathFade.aspectFill = true;
    deathFade.overlay = true;

    SpriteSheetDesc deathFadeDesc = SpriteSheetDesc::Grid(
        AnimationConfig::UI::DeathFadeFrames,
        AnimationConfig::UI::DeathFadeCols,
        AnimationConfig::UI::DeathFadeFrameTime,
        /*loop*/ false);
    deathFadeDesc.playOnStart = false;

    Entity deathFadeAnimation = world.Create()
                                    .With<UITransform>(FadeAnimation)
                                    .With<UIImage>(deathFade)
                                    .Build();
    AnimationTools::AddSpriteSheet(world, deathFadeAnimation, deathFadeDesc);

    ownedEntities_.push_back(deathFadeAnimation);
    deathFadeAnimationEntity_ = deathFadeAnimation;

    // チャージ中の軽い暗転オーバーレイ（アルファをコード側で制御）
    UIImage chargeOverlay{L"./Assets/Textures/Fade/tex_fade_Charging.png"};
    chargeOverlay.opacity = 0.0f;
    chargeOverlay.keepAspect = true;
    chargeOverlay.aspectFill = true;
    chargeOverlay.overlay = true;

    SpriteSheetDesc chargeDesc;
    chargeDesc.frameCount = 1;
    chargeDesc.columns = 1;
    chargeDesc.rows = 1;
    chargeDesc.frameTime = 0.5f;
    chargeDesc.loop = false;
    chargeDesc.playOnStart = false;

    Entity chargeOverlayEntity = world.Create()
                                     .With<UITransform>(FadeAnimation)
                                     .With<UIImage>(chargeOverlay)
                                     .Build();
    AnimationTools::AddSpriteSheet(world, chargeOverlayEntity, chargeDesc);
    ownedEntities_.push_back(chargeOverlayEntity);
    chargeOverlayEntity_ = chargeOverlayEntity;

    // 初期状態で確実に非表示
    if (auto *img = world.TryGet<UIImage>(chargeOverlayEntity_)) {
        img->opacity = 0.0f;
    }
}
