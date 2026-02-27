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
inline static ConfigVar<float> cfg_GameUI_HudFontSize{"Game.UI.TextFormat", "HudFontSize", 24.0f, "ゲームUI: HUDフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_PauseFontSize{"Game.UI.TextFormat", "PauseFontSize", 72.0f, "ゲームUI: Pauseフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_ButtonFontSize{"Game.UI.TextFormat", "ButtonFontSize", 20.0f, "ゲームUI: Buttonフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_PanelFontSize{"Game.UI.TextFormat", "PanelFontSize", 200.0f, "ゲームUI: Panelフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_TitleFontSize{"Game.UI.TextFormat", "TitleFontSize", 20.0f, "ゲームUI: Titleフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_YouFontSize{"Game.UI.TextFormat", "YouFontSize", 36.0f, "ゲームUI: Youフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_NumFontSize{"Game.UI.TextFormat", "NumFontSize", 60.0f, "ゲームUI: Numフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_RoomNumberFontSize{"Game.UI.TextFormat", "RoomNumberFontSize", 55.0f, "ゲームUI: RoomNumberフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_RedTextFontSize{"Game.UI.TextFormat", "RedTextFontSize", 30.0f, "ゲームUI: RedTextフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_TopNumFontSize{"Game.UI.TextFormat", "TopNumFontSize", 55.0f, "ゲームUI: TopNumフォントサイズ"};
inline static ConfigVar<float> cfg_GameUI_RankFontSize{"Game.UI.TextFormat", "RankFontSize", 35.0f, "ゲームUI: Rankフォントサイズ"};

inline static ConfigVar<float> cfg_GameUI_TimePosX{"Game.UI.TimeText", "PosX", 78.0f, "ゲームUI: 時間表示X"};
inline static ConfigVar<float> cfg_GameUI_TimePosY{"Game.UI.TimeText", "PosY", 68.0f, "ゲームUI: 時間表示Y"};
inline static ConfigVar<float> cfg_GameUI_TimeWidth{"Game.UI.TimeText", "Width", 1000.0f, "ゲームUI: 時間表示幅"};
inline static ConfigVar<float> cfg_GameUI_TimeHeight{"Game.UI.TimeText", "Height", 80.0f, "ゲームUI: 時間表示高さ"};
inline static ConfigVar<float> cfg_GameUI_TimeFontSize{"Game.UI.TimeText", "FontSize", 36.0f, "ゲームUI: 時間表示フォントサイズ"};

inline static ConfigVar<float> cfg_GameUI_TimerPosX{"Game.UI.TimerImage", "PosX", -869.0f, "ゲームUI: タイマー画像X"};
inline static ConfigVar<float> cfg_GameUI_TimerPosY{"Game.UI.TimerImage", "PosY", -6.0f, "ゲームUI: タイマー画像Y"};
inline static ConfigVar<float> cfg_GameUI_TimerWidth{"Game.UI.TimerImage", "Width", 2000.0f, "ゲームUI: タイマー画像幅"};
inline static ConfigVar<float> cfg_GameUI_TimerHeight{"Game.UI.TimerImage", "Height", 200.0f, "ゲームUI: タイマー画像高さ"};
inline static ConfigVar<int> cfg_GameUI_TimerFrameCount{"Game.UI.TimerImage", "FrameCount", 131, "ゲームUI: タイマー画像フレーム数"};
inline static ConfigVar<float> cfg_GameUI_TimerFrameTime{"Game.UI.TimerImage", "FrameTime", 0.1f, "ゲームUI: タイマー画像フレーム時間"};
inline static ConfigVar<int> cfg_GameUI_TimerColumns{"Game.UI.TimerImage", "Columns", 16, "ゲームUI: タイマー画像列数"};
inline static ConfigVar<int> cfg_GameUI_TimerRows{"Game.UI.TimerImage", "Rows", 16, "ゲームUI: タイマー画像行数"};

inline static ConfigVar<float> cfg_GameUI_WarningTextPosY{"Game.UI.WarningText", "PosY", -140.0f, "ゲームUI: 警告テキストY"};
inline static ConfigVar<float> cfg_GameUI_WarningTextWidth{"Game.UI.WarningText", "Width", 800.0f, "ゲームUI: 警告テキスト幅"};
inline static ConfigVar<float> cfg_GameUI_WarningTextHeight{"Game.UI.WarningText", "Height", 120.0f, "ゲームUI: 警告テキスト高さ"};

inline static ConfigVar<float> cfg_GameUI_StartImagePosX{"Game.UI.StartImage", "PosX", 520.0f, "ゲームUI: スタート画像X"};
inline static ConfigVar<float> cfg_GameUI_StartImagePosY{"Game.UI.StartImage", "PosY", 85.0f, "ゲームUI: スタート画像Y"};
inline static ConfigVar<float> cfg_GameUI_StartImageWidth{"Game.UI.StartImage", "Width", 250.0f, "ゲームUI: スタート画像幅"};
inline static ConfigVar<float> cfg_GameUI_StartImageHeight{"Game.UI.StartImage", "Height", 250.0f, "ゲームUI: スタート画像高さ"};

inline static ConfigVar<float> cfg_GameUI_ChargeBarPosX{"Game.UI.StartChargeBar", "PosX", 520.0f, "ゲームUI: チャージバーX"};
inline static ConfigVar<float> cfg_GameUI_ChargeBarPosY{"Game.UI.StartChargeBar", "PosY", 270.0f, "ゲームUI: チャージバーY"};
inline static ConfigVar<float> cfg_GameUI_ChargeBarHeight{"Game.UI.StartChargeBar", "Height", 17.0f, "ゲームUI: チャージバー高さ"};

inline static ConfigVar<float> cfg_GameUI_FpsPosX{"Game.UI.Debug.Fps", "PosX", -20.0f, "ゲームUI: FPS表示X"};
inline static ConfigVar<float> cfg_GameUI_FpsPosY{"Game.UI.Debug.Fps", "PosY", 20.0f, "ゲームUI: FPS表示Y"};
inline static ConfigVar<float> cfg_GameUI_FpsWidth{"Game.UI.Debug.Fps", "Width", 200.0f, "ゲームUI: FPS表示幅"};
inline static ConfigVar<float> cfg_GameUI_FpsHeight{"Game.UI.Debug.Fps", "Height", 40.0f, "ゲームUI: FPS表示高さ"};

inline static ConfigVar<float> cfg_GameUI_RoomImageRightMargin{"Game.UI.RoomImage", "RightMargin", 420.0f, "ゲームUI: 部屋番号画像 右マージン"};
inline static ConfigVar<float> cfg_GameUI_RoomImagePosY{"Game.UI.RoomImage", "PosY", 42.0f, "ゲームUI: 部屋番号画像Y"};
inline static ConfigVar<float> cfg_GameUI_RoomImageWidth{"Game.UI.RoomImage", "Width", 240.0f, "ゲームUI: 部屋番号画像幅"};
inline static ConfigVar<float> cfg_GameUI_RoomImageHeight{"Game.UI.RoomImage", "Height", 96.0f, "ゲームUI: 部屋番号画像高さ"};

inline static ConfigVar<float> cfg_GameUI_StageCounterRightMargin{"Game.UI.StageCounter", "RightMargin", 40.0f, "ゲームUI: ステージカウンタ 右マージン"};
inline static ConfigVar<float> cfg_GameUI_StageCounterPosY{"Game.UI.StageCounter", "PosY", 65.0f, "ゲームUI: ステージカウンタY"};
inline static ConfigVar<float> cfg_GameUI_StageCounterWidth{"Game.UI.StageCounter", "Width", 210.0f, "ゲームUI: ステージカウンタ幅"};
inline static ConfigVar<float> cfg_GameUI_StageCounterHeight{"Game.UI.StageCounter", "Height", 100.0f, "ゲームUI: ステージカウンタ高さ"};
inline static ConfigVar<float> cfg_GameUI_StageCounterSecondOffsetX{"Game.UI.StageCounter", "SecondOffsetX", 90.0f, "ゲームUI: ステージカウンタ2つ目のXオフセット"};
inline static ConfigVar<float> cfg_GameUI_StageCounterSecondOffsetY{"Game.UI.StageCounter", "SecondOffsetY", -8.0f, "ゲームUI: ステージカウンタ2つ目のYオフセット"};
inline static ConfigVar<float> cfg_GameUI_StageCounterColorR{"Game.UI.StageCounter", "ColorR", 0.0f, "ゲームUI: ステージカウンタ色R"};
inline static ConfigVar<float> cfg_GameUI_StageCounterColorG{"Game.UI.StageCounter", "ColorG", 0.5f, "ゲームUI: ステージカウンタ色G"};
inline static ConfigVar<float> cfg_GameUI_StageCounterColorB{"Game.UI.StageCounter", "ColorB", 1.0f, "ゲームUI: ステージカウンタ色B"};
inline static ConfigVar<float> cfg_GameUI_StageCounterOutline{"Game.UI.StageCounter", "OutlineThickness", 1.0f, "ゲームUI: ステージカウンタアウトライン太さ"};

inline static ConfigVar<float> cfg_GameUI_PauseTitlePosX{"Game.UI.Pause.Title", "PosX", 470.0f, "ゲームUI: PauseタイトルX"};
inline static ConfigVar<float> cfg_GameUI_PauseTitlePosY{"Game.UI.Pause.Title", "PosY", -20.0f, "ゲームUI: PauseタイトルY"};
inline static ConfigVar<float> cfg_GameUI_PauseLinePosX{"Game.UI.Pause.Line", "PosX", 60.0f, "ゲームUI: PauseラインX"};
inline static ConfigVar<float> cfg_GameUI_PauseLinePosY{"Game.UI.Pause.Line", "PosY", 120.0f, "ゲームUI: PauseラインY"};
inline static ConfigVar<float> cfg_GameUI_PauseButtonPosX{"Game.UI.Pause.Button", "PosX", 70.0f, "ゲームUI: PauseボタンX"};
inline static ConfigVar<float> cfg_GameUI_PauseButtonStepY{"Game.UI.Pause.Button", "StepY", 80.0f, "ゲームUI: PauseボタンY間隔"};
inline static ConfigVar<float> cfg_GameUI_PausePanelAlpha{"Game.UI.Pause.Panel", "Alpha", 0.55f, "ゲームUI: Pause背景アルファ"};
inline static ConfigVar<float> cfg_GameUI_PauseMenuButtonWidth{"Game.UI.Pause.Button", "Width", 300.0f, "ゲームUI: Pauseボタン幅"};
inline static ConfigVar<float> cfg_GameUI_PauseMenuButtonHeight{"Game.UI.Pause.Button", "Height", 50.0f, "ゲームUI: Pauseボタン高さ"};
inline static ConfigVar<float> cfg_GameUI_PauseTitleImageWidth{"Game.UI.Pause.Title", "Width", 360.0f, "ゲームUI: Pauseタイトル幅"};
inline static ConfigVar<float> cfg_GameUI_PauseTitleImageHeight{"Game.UI.Pause.Title", "Height", 90.0f, "ゲームUI: Pauseタイトル高さ"};
inline static ConfigVar<float> cfg_GameUI_PauseLineImageWidth{"Game.UI.Pause.Line", "Width", 15.0f, "ゲームUI: Pauseライン幅"};
inline static ConfigVar<float> cfg_GameUI_PauseLineImageHeight{"Game.UI.Pause.Line", "Height", 350.0f, "ゲームUI: Pauseライン高さ"};
inline static ConfigVar<float> cfg_GameUI_PauseSelectImageWidth{"Game.UI.Pause.Select", "Width", 320.0f, "ゲームUI: Pauseセレクト画像幅"};
inline static ConfigVar<float> cfg_GameUI_PauseSelectImageHeight{"Game.UI.Pause.Select", "Height", 40.0f, "ゲームUI: Pauseセレクト画像高さ"};

inline static ConfigVar<float> cfg_GameUI_ClearTextPosY{"Game.UI.ClearText", "PosY", 140.0f, "ゲームUI: クリアテキストY"};
inline static ConfigVar<float> cfg_GameUI_ClearTextWidth{"Game.UI.ClearText", "Width", 900.0f, "ゲームUI: クリアテキスト幅"};
inline static ConfigVar<float> cfg_GameUI_ClearTextHeight{"Game.UI.ClearText", "Height", 140.0f, "ゲームUI: クリアテキスト高さ"};

inline static ConfigVar<float> cfg_GameUI_EndRankYouBaseX{"Game.UI.EndRank.You", "BaseX", 50.0f, "ゲームUI: EndRank You基準X"};
inline static ConfigVar<float> cfg_GameUI_EndRankYouBaseY{"Game.UI.EndRank.You", "BaseY", 200.0f, "ゲームUI: EndRank You基準Y"};
inline static ConfigVar<float> cfg_GameUI_EndRankYouWidth{"Game.UI.EndRank.You", "Width", 80.0f, "ゲームUI: EndRank You幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankYouHeight{"Game.UI.EndRank.You", "Height", 40.0f, "ゲームUI: EndRank You高さ"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentNumOffsetX{"Game.UI.EndRank.You", "CurrentNumOffsetX", 120.0f, "ゲームUI: EndRank 現在デス数Xオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentNumOffsetY{"Game.UI.EndRank.You", "CurrentNumOffsetY", 0.0f, "ゲームUI: EndRank 現在デス数Yオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentNumWidth{"Game.UI.EndRank.You", "CurrentNumWidth", 120.0f, "ゲームUI: EndRank 現在デス数幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentNumHeight{"Game.UI.EndRank.You", "CurrentNumHeight", 100.0f, "ゲームUI: EndRank 現在デス数高さ"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentDeathOffsetX{"Game.UI.EndRank.You", "CurrentDeathOffsetX", 170.0f, "ゲームUI: EndRank death画像Xオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentDeathOffsetY{"Game.UI.EndRank.You", "CurrentDeathOffsetY", 5.0f, "ゲームUI: EndRank death画像Yオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentDeathWidth{"Game.UI.EndRank.You", "CurrentDeathWidth", 120.0f, "ゲームUI: EndRank death画像幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankCurrentDeathHeight{"Game.UI.EndRank.You", "CurrentDeathHeight", 35.0f, "ゲームUI: EndRank death画像高さ"};

inline static ConfigVar<float> cfg_GameUI_EndRankRightMargin{"Game.UI.EndRank.Top", "RightMargin", 200.0f, "ゲームUI: EndRank右マージン"};
inline static ConfigVar<float> cfg_GameUI_EndRankBaseY{"Game.UI.EndRank.Top", "BaseY", 220.0f, "ゲームUI: EndRank基準Y"};
inline static ConfigVar<float> cfg_GameUI_EndRankStepY{"Game.UI.EndRank.Top", "StepY", 70.0f, "ゲームUI: EndRank行間隔"};
inline static ConfigVar<float> cfg_GameUI_EndRankSuffixWidth{"Game.UI.EndRank.Top", "SuffixWidth", 60.0f, "ゲームUI: EndRank順位画像幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankSuffixHeight{"Game.UI.EndRank.Top", "SuffixHeight", 50.0f, "ゲームUI: EndRank順位画像高さ"};
inline static ConfigVar<float> cfg_GameUI_EndRankTopNumOffsetX{"Game.UI.EndRank.Top", "TopNumOffsetX", 120.0f, "ゲームUI: EndRank上位数値Xオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankTopNumOffsetY{"Game.UI.EndRank.Top", "TopNumOffsetY", -10.0f, "ゲームUI: EndRank上位数値Yオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankTopNumWidth{"Game.UI.EndRank.Top", "TopNumWidth", 80.0f, "ゲームUI: EndRank上位数値幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankTopNumHeight{"Game.UI.EndRank.Top", "TopNumHeight", 60.0f, "ゲームUI: EndRank上位数値高さ"};
inline static ConfigVar<float> cfg_GameUI_EndRankTopNumOutline{"Game.UI.EndRank.Top", "TopNumOutlineThickness", 1.0f, "ゲームUI: EndRank上位数値アウトライン太さ"};
inline static ConfigVar<float> cfg_GameUI_EndRankDeathOffsetX{"Game.UI.EndRank.Top", "DeathOffsetX", 160.0f, "ゲームUI: EndRank death画像Xオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankDeathOffsetY{"Game.UI.EndRank.Top", "DeathOffsetY", 15.0f, "ゲームUI: EndRank death画像Yオフセット"};
inline static ConfigVar<float> cfg_GameUI_EndRankDeathWidth{"Game.UI.EndRank.Top", "DeathWidth", 100.0f, "ゲームUI: EndRank death画像幅"};
inline static ConfigVar<float> cfg_GameUI_EndRankDeathHeight{"Game.UI.EndRank.Top", "DeathHeight", 30.0f, "ゲームUI: EndRank death画像高さ"};

inline static ConfigVar<float> cfg_GameUI_ChargeOverlayFrameTime{"Game.UI.Fade", "ChargeOverlayFrameTime", 0.5f, "ゲームUI: チャージ暗転フレーム時間"};

void GameScene::CreateTextFormats() {
    TextSystem::TextFormat hudFormat;
    hudFormat.fontSize = cfg_GameUI_HudFontSize.Get();
    hudFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    hudFormat.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    textSystem_.CreateTextFormat("hud", hudFormat);

    TextSystem::TextFormat pauseFormat;
    pauseFormat.fontSize = cfg_GameUI_PauseFontSize.Get();
    pauseFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    pauseFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
    pauseFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    textSystem_.CreateTextFormat("pause", pauseFormat);

    TextSystem::TextFormat buttonFormat;
    buttonFormat.fontSize = cfg_GameUI_ButtonFontSize.Get();
    buttonFormat.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
    buttonFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    textSystem_.CreateTextFormat("button", buttonFormat);

    TextSystem::TextFormat panelFormat;
    panelFormat.fontSize = cfg_GameUI_PanelFontSize.Get();
    textSystem_.CreateTextFormat("panel", panelFormat);

    TextSystem::TextFormat titleFormat;
    titleFormat.fontSize = cfg_GameUI_TitleFontSize.Get();
    titleFormat.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    titleFormat.style = DWRITE_FONT_STYLE_ITALIC;
    titleFormat.alignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
    titleFormat.paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    textSystem_.CreateTextFormat("title", titleFormat);

    // デス数・ランキング表示用
    TextSystem::TextFormat youFmt;
    youFmt.fontSize = cfg_GameUI_YouFontSize.Get();
    youFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    textSystem_.CreateTextFormat("you", youFmt);

    TextSystem::TextFormat numFmt;
    numFmt.fontSize = cfg_GameUI_NumFontSize.Get();
    numFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
    numFmt.alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
    textSystem_.CreateTextFormat("num", numFmt);

    TextSystem::TextFormat roomNumFmt;
    roomNumFmt.fontSize = cfg_GameUI_RoomNumberFontSize.Get();
    roomNumFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
    roomNumFmt.alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
    textSystem_.CreateTextFormat("roomNumber", roomNumFmt);

    TextSystem::TextFormat redFmt;
    redFmt.fontSize = cfg_GameUI_RedTextFontSize.Get();
    redFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
    textSystem_.CreateTextFormat("redText", redFmt);

    TextSystem::TextFormat topNumFmt;
    topNumFmt.fontSize = cfg_GameUI_TopNumFontSize.Get();
    topNumFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
    topNumFmt.alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
    textSystem_.CreateTextFormat("topNum", topNumFmt);

    TextSystem::TextFormat rankFmt;
    rankFmt.fontSize = cfg_GameUI_RankFontSize.Get();
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
    std::vector<Entity> pauseDimmableEntities;

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
    timeTransform.position = {cfg_GameUI_TimePosX.Get(), cfg_GameUI_TimePosY.Get()};
    timeTransform.size = {cfg_GameUI_TimeWidth.Get(), cfg_GameUI_TimeHeight.Get()};
    timeTransform.anchor = {0.0f, 0.0f};
    timeTransform.pivot = {0.0f, 0.0f};

    UIText timeText{L"時間: 00:00"};
    timeText.color = {1.0f, 0.0f, 0.0f, 1.0f};
    timeText.formatId = "hud";
    timeText.fontSize = cfg_GameUI_TimeFontSize.Get();

    Entity timeEntity = world.Create()
                            .With<UITransform>(timeTransform)
                            .With<UIText>(timeText)
                            .Build();
    ownedEntities_.push_back(timeEntity);
    pauseDimmableEntities.push_back(timeEntity);

    UITransform timerImgTr;
    timerImgTr.position = {cfg_GameUI_TimerPosX.Get(), cfg_GameUI_TimerPosY.Get()};
    timerImgTr.size = {cfg_GameUI_TimerWidth.Get(), cfg_GameUI_TimerHeight.Get()};
    timerImgTr.anchor = {0.0f, 0.0f};
    timerImgTr.pivot = {0.0f, 0.0f};

    UIImage timerImg{L"./Assets/Textures/Time/Timer_Sprite.png"};
    timerImg.opacity = 1.0f;
    timerImg.keepAspect = true;

    SpriteSheetAnimation timerAnime;
    timerAnime.frameCount = cfg_GameUI_TimerFrameCount.Get();
    timerAnime.frameTime = cfg_GameUI_TimerFrameTime.Get();
    timerAnime.columns = cfg_GameUI_TimerColumns.Get();
    timerAnime.rows = cfg_GameUI_TimerRows.Get();

    timerAnime.isLooping = true;
    timerAnime.isPlaying = true;
    Entity timerImageEntity = world.Create()
                                  .With<UITransform>(timerImgTr)
                                  .With<UIImage>(timerImg)
                                  .With<UVAnimation>()
                                  .With<SpriteSheetAnimation>(timerAnime)
                                  .Build();

    ownedEntities_.push_back(timerImageEntity);
    pauseDimmableEntities.push_back(timerImageEntity);

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
    warningTextTransform.position = {0.0f, cfg_GameUI_WarningTextPosY.Get()};
    warningTextTransform.size = {cfg_GameUI_WarningTextWidth.Get(), cfg_GameUI_WarningTextHeight.Get()};
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
    startImgTr.position = {cfg_GameUI_StartImagePosX.Get(), cfg_GameUI_StartImagePosY.Get()};
    startImgTr.size = {cfg_GameUI_StartImageWidth.Get(), cfg_GameUI_StartImageHeight.Get()};
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
    pauseDimmableEntities.push_back(startImageEntity);

    UITransform barTr;
    barTr.position = {cfg_GameUI_ChargeBarPosX.Get(), cfg_GameUI_ChargeBarPosY.Get()};
    barTr.size = {0.0f, cfg_GameUI_ChargeBarHeight.Get()};
    barTr.anchor = {0.0f, 0.0f};
    barTr.pivot = {0.0f, 0.0f};

    UIPanel barPanel;
    barPanel.color = {1.0f, 0.0f, 0.0f, 1.0f};

    Entity startChargeBarEntity = world.Create()
                                    .With<UITransform>(barTr)
                                    .With<UIPanel>(barPanel)
                                    .Build();
    ownedEntities_.push_back(startChargeBarEntity);
    pauseDimmableEntities.push_back(startChargeBarEntity);

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
    fpsTransform.position = {cfg_GameUI_FpsPosX.Get(), cfg_GameUI_FpsPosY.Get()};
    fpsTransform.size = {cfg_GameUI_FpsWidth.Get(), cfg_GameUI_FpsHeight.Get()};
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
    pauseDimmableEntities.push_back(fpsEntity);
#endif // !_DEBUG

    UITransform roomImgTr;
    roomImgTr.position = {screenWidth - cfg_GameUI_RoomImageRightMargin.Get(), cfg_GameUI_RoomImagePosY.Get()};
    roomImgTr.size = {cfg_GameUI_RoomImageWidth.Get(), cfg_GameUI_RoomImageHeight.Get()};
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
    pauseDimmableEntities.push_back(roomImageEntity);

    UIText stageText[2];

    stageText[0].text = {L"Room : 0"};
    stageText[0].color = {cfg_GameUI_StageCounterColorR.Get(), cfg_GameUI_StageCounterColorG.Get(), cfg_GameUI_StageCounterColorB.Get(), 1.0f};
    stageText[0].formatId = "roomNumber";
    stageText[0].outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
    stageText[0].outlineThickness = cfg_GameUI_StageCounterOutline.Get();
    stageText[0].fillTexturePath = L"./Assets/Textures/RoomNo/UI-2-color.png";

    stageText[1].text = {L"0"};
    stageText[1].color = {cfg_GameUI_StageCounterColorR.Get(), cfg_GameUI_StageCounterColorG.Get(), cfg_GameUI_StageCounterColorB.Get(), 1.0f};
    stageText[1].formatId = "roomNumber";
    stageText[1].outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
    stageText[1].outlineThickness = cfg_GameUI_StageCounterOutline.Get();
    stageText[1].fillTexturePath = L"./Assets/Textures/RoomNo/UI-2-color.png";

    UITransform stageTransform[2];
    stageTransform[0].position = {screenWidth - cfg_GameUI_StageCounterRightMargin.Get(), cfg_GameUI_StageCounterPosY.Get()};
    stageTransform[0].size = {cfg_GameUI_StageCounterWidth.Get(), cfg_GameUI_StageCounterHeight.Get()};
    stageTransform[0].anchor = {0.0f, 0.0f};
    stageTransform[0].pivot = {1.0f, 0.0f};

    stageTransform[1].position = {stageTransform[0].position.x - cfg_GameUI_StageCounterSecondOffsetX.Get(),
                                  stageTransform[0].position.y + cfg_GameUI_StageCounterSecondOffsetY.Get()};
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
        pauseDimmableEntities.push_back(stageEntity[i]);
    }

    UITransform pauseTransform;
    pauseTransform.position = {cfg_GameUI_PauseTitlePosX.Get(), cfg_GameUI_PauseTitlePosY.Get()};
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
    lineTr.position = {cfg_GameUI_PauseLinePosX.Get(), cfg_GameUI_PauseLinePosY.Get()};
    lineTr.size = {0.0f, 0.0f};       // isPaused時に展開
    lineTr.anchor = {0.0f, 0.5f};
    lineTr.pivot = {0.0f, 0.5f};
    UIImage lineImg{L"./Assets/Textures/UI/PausedUI/LeftLin.png"};
    lineImg.opacity = 0.0f;
    lineImg.keepAspect = true;
    lineImg.aspectAlignLeft = true;
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
    selectImg.aspectAlignLeft = true;
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
    pauseMenuPanel.color = {0.0f, 0.0f, 0.0f, cfg_GameUI_PausePanelAlpha.Get()};
    pauseMenuPanel.visible = false;
    pauseMenuPanel.drawBeforeImages = true;

    Entity pauseMenuPanelEntity = world.Create()
                                     .With<UITransform>(pauseMenuPanelTr)
                                     .With<UIPanel>(pauseMenuPanel)
                                     .Build();
    ownedEntities_.push_back(pauseMenuPanelEntity);

    auto createPauseButton = [&](const std::wstring &imagePath, float yOffset) {
        UITransform tr;
        tr.position = {cfg_GameUI_PauseButtonPosX.Get(), yOffset};
        tr.size = {0.0f, 0.0f};
        tr.anchor = {0.0f, 0.5f};
        tr.pivot = {0.0f, 0.5f};

        UIImage img{imagePath};
        img.keepAspect = true;
        img.aspectAlignLeft = true;
        img.opacity = 0.0f; // 初期は非表示

        UIButton btn;
        btn.enabled = false;
        btn.normalColor = {0.0f, 0.0f, 0.0f, 0.0f};
        btn.hoverColor = {0.0f, 0.0f, 0.0f, 0.0f};
        btn.pressedColor = {0.0f, 0.0f, 0.0f, 0.0f};
        btn.disabledColor = {0.0f, 0.0f, 0.0f, 0.0f};

        Entity e = world.Create().With<UITransform>(tr).With<UIButton>(btn).With<UIImage>(img).Build();
        ownedEntities_.push_back(e);
        return e;
    };

    const float pauseButtonStepY = cfg_GameUI_PauseButtonStepY.Get();
    Entity pauseResumeBtn = createPauseButton(L"./Assets/Textures/UI/PausedUI/BackGame.png", 0.0f);
    Entity pauseRetryBtn  = createPauseButton(L"./Assets/Textures/UI/PausedUI/Retray.png", pauseButtonStepY);
    Entity pauseSelectBtn = createPauseButton(L"./Assets/Textures/UI/PausedUI/StageSelect.png", pauseButtonStepY * 2.0f);
    Entity pauseTitleBtn  = createPauseButton(L"./Assets/Textures/UI/PausedUI/BackTitle.png", pauseButtonStepY * 3.0f);
    Entity pauseOptionsBtn = Entity(); // 無効化
    Entity pauseQuitBtn = Entity();    // 無効化

    // 追加: ステージクリア表示（最初は非表示＝空文字）
    UITransform clearTransform;
    clearTransform.position = {0.0f, cfg_GameUI_ClearTextPosY.Get()};
    clearTransform.size = {cfg_GameUI_ClearTextWidth.Get(), cfg_GameUI_ClearTextHeight.Get()};
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
    pauseDimmableEntities.push_back(clearEntity);
    stageClearTextEntity_ = clearEntity;

    // ==========================================
    // ランキング・デスカウントUI要素の追加
    // ==========================================
    
    // 【左側】 You (画像) + デス数 (白) + death (画像)
    const float youBaseX = cfg_GameUI_EndRankYouBaseX.Get();
    const float youBaseY = cfg_GameUI_EndRankYouBaseY.Get();

    UITransform youTr;
    youTr.position = {youBaseX, youBaseY};
    youTr.size = {cfg_GameUI_EndRankYouWidth.Get(), cfg_GameUI_EndRankYouHeight.Get()};
    UIImage youImg{L"./Assets/Textures/UI/EndRankUI/You.png"};
    youImg.opacity = 1.0f;
    youImg.keepAspect = true;
    Entity youEnt = world.Create().With<UITransform>(youTr).With<UIImage>(youImg).Build();
    ownedEntities_.push_back(youEnt);
    pauseDimmableEntities.push_back(youEnt);

    UITransform currDeathTr;
    currDeathTr.position = {youBaseX + cfg_GameUI_EndRankCurrentNumOffsetX.Get(), youBaseY + cfg_GameUI_EndRankCurrentNumOffsetY.Get()};
    currDeathTr.size = {cfg_GameUI_EndRankCurrentNumWidth.Get(), cfg_GameUI_EndRankCurrentNumHeight.Get()};
    currDeathTr.pivot = {1.0f, 0.5f};
    UIText currDeathText{L"0"};
    currDeathText.color = {1.0f, 1.0f, 1.0f, 1.0f};
    currDeathText.formatId = "num";
    Entity currDeathEnt = world.Create().With<UITransform>(currDeathTr).With<UIText>(currDeathText).Build();
    ownedEntities_.push_back(currDeathEnt);
    pauseDimmableEntities.push_back(currDeathEnt);

    UITransform currDeathRedTr;
    currDeathRedTr.position = {youBaseX + cfg_GameUI_EndRankCurrentDeathOffsetX.Get(), youBaseY + cfg_GameUI_EndRankCurrentDeathOffsetY.Get()};
    currDeathRedTr.size = {cfg_GameUI_EndRankCurrentDeathWidth.Get(), cfg_GameUI_EndRankCurrentDeathHeight.Get()};
    UIImage deathImg{L"./Assets/Textures/UI/EndRankUI/Death.png"};
    deathImg.opacity = 1.0f;
    deathImg.keepAspect = true;
    Entity currDeathRedEnt = world.Create().With<UITransform>(currDeathRedTr).With<UIImage>(deathImg).Build();
    ownedEntities_.push_back(currDeathRedEnt);
    pauseDimmableEntities.push_back(currDeathRedEnt);


    // 【右側】 Top 3 ランキング (1st 23 death ...)
    const float rankBaseX = screenWidth - cfg_GameUI_EndRankRightMargin.Get();
    const float rankBaseY = cfg_GameUI_EndRankBaseY.Get();
    const float yOffset = cfg_GameUI_EndRankStepY.Get();

    int pss = 0;
    world.ForEach<StageProgress>([&](Entity, StageProgress &sp) {
        pss = GetPssNumber(sp.worldCount, sp.currentStage);
    });
    if (pss <= 0) {
        pss = StageSave::GetLastSavedPss();
    }
    std::vector<int> topDeaths = StageSave::GetTopDeaths(pss);
    std::wstring topDeathLabels[3] = {L"-", L"-", L"-"};
    for (size_t i = 0; i < topDeaths.size() && i < 3; ++i) {
        topDeathLabels[i] = std::to_wstring(topDeaths[i]);
    }

    std::wstring suffixStrs[] = { L"./Assets/Textures/UI/StegeRankUI/1st.png", L"./Assets/Textures/UI/StegeRankUI/2nd.png", L"./Assets/Textures/UI/StegeRankUI/3rd.png" };

    for (size_t i = 0; i < 3; ++i) {
        float curY = rankBaseY + (i * yOffset);
        
        // 1st / 2nd / 3rd (画像)
        UITransform sufTr;
        sufTr.position = {rankBaseX, curY};
        sufTr.size = {cfg_GameUI_EndRankSuffixWidth.Get(), cfg_GameUI_EndRankSuffixHeight.Get()};
        UIImage sufImg{suffixStrs[i]};
        sufImg.opacity = 1.0f;
        sufImg.keepAspect = true;
        Entity sufEnt = world.Create().With<UITransform>(sufTr).With<UIImage>(sufImg).Build();
        ownedEntities_.push_back(sufEnt);
        pauseDimmableEntities.push_back(sufEnt);

        // 数値 (白・アウトライン付テキスト)
        UITransform topNumTr;
        topNumTr.position = {rankBaseX + cfg_GameUI_EndRankTopNumOffsetX.Get(), curY + cfg_GameUI_EndRankTopNumOffsetY.Get()};
        topNumTr.size = {cfg_GameUI_EndRankTopNumWidth.Get(), cfg_GameUI_EndRankTopNumHeight.Get()};
        topNumTr.pivot = {1.0f, 0.5f};
        UIText topNumText{topDeathLabels[i]};
        topNumText.color = {0.0f, 0.0f, 0.0f, 1.0f}; // 白字
        topNumText.formatId = "topNum";
        topNumText.outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
        topNumText.outlineThickness = cfg_GameUI_EndRankTopNumOutline.Get();
        Entity topNumEnt = world.Create().With<UITransform>(topNumTr).With<UIText>(topNumText).Build();
        ownedEntities_.push_back(topNumEnt);
        pauseDimmableEntities.push_back(topNumEnt);

        // death (画像)  //EndRankだけどゲーム内のデス表示GameDeath
        UITransform suffixRedTr;
        suffixRedTr.position = {rankBaseX + cfg_GameUI_EndRankDeathOffsetX.Get(), curY + cfg_GameUI_EndRankDeathOffsetY.Get()};
        suffixRedTr.size = {cfg_GameUI_EndRankDeathWidth.Get(), cfg_GameUI_EndRankDeathHeight.Get()};
        UIImage suffixRedImg{L"./Assets/Textures/UI/EndRankUI/Death.png"};
        suffixRedImg.opacity = 1.0f;
        suffixRedImg.keepAspect = true;
        Entity suffixRedEnt = world.Create().With<UITransform>(suffixRedTr).With<UIImage>(suffixRedImg).Build();
        ownedEntities_.push_back(suffixRedEnt);
        pauseDimmableEntities.push_back(suffixRedEnt);
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
        updater->pauseMenuButtonSize_ = {cfg_GameUI_PauseMenuButtonWidth.Get(), cfg_GameUI_PauseMenuButtonHeight.Get()};
        updater->pauseTitleImgSize_ = {cfg_GameUI_PauseTitleImageWidth.Get(), cfg_GameUI_PauseTitleImageHeight.Get()};
        updater->pauseLineImgSize_ = {cfg_GameUI_PauseLineImageWidth.Get(), cfg_GameUI_PauseLineImageHeight.Get()};
        updater->pauseSelectImgSize_ = {cfg_GameUI_PauseSelectImageWidth.Get(), cfg_GameUI_PauseSelectImageHeight.Get()};
        updater->pauseDimmableEntities_ = pauseDimmableEntities;
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
    FadeAnimation.size = {screenWidth, screenHeight};
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
    chargeDesc.frameTime = cfg_GameUI_ChargeOverlayFrameTime.Get();
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
