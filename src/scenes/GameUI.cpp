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
#include "config/ConfigVar.h"
#include "animation/Animation.h" // cfg_ChargingFade / SpriteSheetAnimation など
#include "animation/AnimationTools.h"
#include "animation/AnimationConfig.h"

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

    TextSystem::TextFormat roomNumberFormat = hudFormat;
    roomNumberFormat.fontFamily = L"Kinkakuji-Normal";
    roomNumberFormat.weight = DWRITE_FONT_WEIGHT_BOLD;
    textSystem_.CreateTextFormat("roomNumber", roomNumberFormat);
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

    UITransform starttimeTransform;
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

    ownedEntities_.push_back(starttime);

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
    pauseTransform.position = {0.0f, -180.0f};
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

    auto createPauseButton = [&](const std::wstring &label, float yOffset) {
        UITransform tr;
        tr.position = {0.0f, yOffset};
        tr.size = {0.0f, 0.0f};
        tr.anchor = {0.5f, 0.5f};
        tr.pivot = {0.5f, 0.5f};

        UIText txt{label};
        txt.color = {1.0f, 1.0f, 1.0f, 1.0f};
        txt.formatId = "button";

        UIButton btn;
        btn.enabled = false;

        Entity e = world.Create().With<UITransform>(tr).With<UIButton>(btn).With<UIText>(txt).Build();
        ownedEntities_.push_back(e);
        return e;
    };

    Entity pauseResumeBtn = createPauseButton(L"再開", 40.0f);
    Entity pauseRetryBtn = createPauseButton(L"リトライ", 110.0f);
    Entity pauseTitleBtn = createPauseButton(L"タイトルへ", 180.0f);
    Entity pauseQuitBtn = createPauseButton(L"終了", 250.0f);

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
        //  updater->numEntity_ = numEntity;
        updater->timeImageEntity_ = timerImageEntity;
        updater->startplayer_ = starttime;
        updater->warningOverlayEntity_ = warningOverlayEntity;
        updater->warningTextEntity_ = warningTextEntity;

        updater->pauseMenuPanelEntity_ = pauseMenuPanelEntity;
        updater->pauseResumeButtonEntity_ = pauseResumeBtn;
        updater->pauseRetryButtonEntity_ = pauseRetryBtn;
        updater->pauseTitleButtonEntity_ = pauseTitleBtn;
        updater->pauseQuitButtonEntity_ = pauseQuitBtn;
        updater->pauseMenuButtonSize_ = {360.0f, 60.0f};
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
                if (auto *mgr = ServiceLocator::TryGet<SceneManager>()) {
                    mgr->ChangeScene("Title", *wptr);
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
    fade.keepAspect = false;
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
    deathFade.keepAspect = false;
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
    chargeOverlay.keepAspect = false;
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
