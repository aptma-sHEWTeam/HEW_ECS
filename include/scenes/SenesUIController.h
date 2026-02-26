#pragma once

#include "components/Component.h"
#include "components/UIComponents.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include "ecs/Entity.h"
#include "ecs/World.h"
#include "systems/UISystem.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include "components/PlayerComponents.h"
#include "config/ConfigVar.h"

/**
 * @struct GameUIUpdater
 * @brief ゲームUIを更新するシステム
 */
struct GameUIUpdater : Behaviour {
    Entity scoreTextEntity_;
    Entity timeTextEntity_;
    Entity fpsTextEntity_;
    Entity pauseTextEntity_;
    Entity stageTextEntity_[2];
    Entity timerBackgroundEntity_;
    Entity timeImageEntity_;
    Entity timerUiEntity_;
    Entity numEntity_;
    int cachedStage_ = -1;
    int cachedRoomCount_ = 1;
    int cachedWorldCount = 1;
    Entity startplayer_;
    Entity startChargeBarEntity_;
    Entity warningOverlayEntity_;
    //Entity warningTextEntity_;
    float warningBlinkTimer_ = 0.0f;
    Entity currDeathEntity_;

    Entity pauseMenuPanelEntity_;
    Entity pauseResumeButtonEntity_;
    Entity pauseRetryButtonEntity_;
    Entity pauseTitleButtonEntity_;
    Entity pauseStageSelectButtonEntity_;
    Entity pauseOptionsButtonEntity_;
    Entity pauseQuitButtonEntity_;

    Entity pauseLineEntity_;
    Entity pauseSelectIndicatorEntity_; // 新規追加

    DirectX::XMFLOAT2 pauseMenuButtonSize_{360.0f, 60.0f};
    DirectX::XMFLOAT2 pauseTitleImgSize_{400.0f, 100.0f};
    DirectX::XMFLOAT2 pauseLineImgSize_{15.0f, 350.0f};
    DirectX::XMFLOAT2 pauseSelectImgSize_{320.0f, 40.0f}; // select.pngのサイズ指定
    inline static ConfigVar<float> cfg_GameUIWarningThresholdSeconds{"Game.UI.Warning", "ThresholdSeconds", 3.0f, "ゲームUI: 警告表示を開始する残り時間(秒)"};
    inline static ConfigVar<float> cfg_GameUIWarningPeriod{"Game.UI.Warning", "BlinkPeriod", 0.6f, "ゲームUI: 警告点滅周期(秒)"};
    inline static ConfigVar<float> cfg_GameUIWarningOnTime{"Game.UI.Warning", "BlinkOnTime", 0.2f, "ゲームUI: 警告点滅の点灯時間(秒)"};
    inline static ConfigVar<float> cfg_GameUIWarningOverlayAlpha{"Game.UI.Warning", "OverlayAlpha", 0.35f, "ゲームUI: 警告オーバーレイアルファ"};
    inline static ConfigVar<float> cfg_GameUIStartChargeMaxTime{"Game.UI.StartChargeBar", "MaxChargeTime", 0.6f, "ゲームUI: チャージバー最大時間(秒)"};
    inline static ConfigVar<float> cfg_GameUIStartChargeMaxWidth{"Game.UI.StartChargeBar", "MaxWidth", 250.0f, "ゲームUI: チャージバー最大幅"};
    inline static ConfigVar<float> cfg_GameUIPauseSelectOffsetX{"Game.UI.Pause.Select", "HoverOffsetX", 20.0f, "ゲームUI: Pauseセレクト表示Xオフセット"};
    inline static ConfigVar<float> cfg_GameUIPauseSelectOffsetY{"Game.UI.Pause.Select", "HoverOffsetY", 45.0f, "ゲームUI: Pauseセレクト表示Yオフセット"};
    inline static ConfigVar<float> cfg_GameUIPauseNonPauseOpacity{"Game.UI.Pause", "NonPauseOpacity", 0.35f, "ゲームUI: ポーズ中の非ポーズUI不透明度"};
    std::vector<Entity> pauseDimmableEntities_;
    bool pauseDimApplied_ = false;
    std::unordered_map<Entity, float> pauseBaseImageOpacity_;
    std::unordered_map<Entity, float> pauseBaseTextAlpha_;
    std::unordered_map<Entity, float> pauseBaseTextOutlineAlpha_;
    std::unordered_map<Entity, float> pauseBasePanelAlpha_;

    void OnUpdate(World &w, Entity self, float dt) override {
        w.ForEach<GameStatus>([&](Entity e, GameStatus &stats) {
            // 経過時間の更新
            if (!stats.isPaused && stats.timerRunning) {
                stats.elapsedTime = std::max(0.0f, stats.elapsedTime - dt);

                if (stats.elapsedTime <= 0.0f) {
                    stats.elapsedTime = 0.0f;
                    stats.timerRunning = false;
                    stats.isDead = true;
                    stats.resetDone = false;
                }
            }
            if (stats.fadeFinished && !stats.resetDone) {
                stats.elapsedTime = cfg_LimitTime;
                stats.timerRunning = true;

                stats.isDead = false;
                stats.resetDone = true;
                stats.fadeFinished = false;
            }
            //スタート時間の更新
            if (stats.StartCountDown > 0.0f) {
                stats.StartCountDown = std::max(0.0f, stats.StartCountDown - dt);
            }

            if (!stats.StartChack) {

                stats.waitingForPlayerMove = true;
                // stats.timerRunning = false;
            }

            //スコア表示の更新
            if (auto *scoreText = w.TryGet<UIText>(scoreTextEntity_)) {
                std::wstringstream ss;
                ss << L"Score: " << stats.score;
                scoreText->text = ss.str();
            }

            //経過時間表示の更新
            if (auto *timeText = w.TryGet<UIText>(timeTextEntity_)) {
                std::wstringstream ss;

                int seconds = static_cast<int>(stats.elapsedTime);
                int milliseconds = static_cast<int>((stats.elapsedTime - seconds) * 100);

                ss << L" " << std::setw(2) << std::setfill(L'0') << seconds
                   << L":" << std::setw(2) << std::setfill(L'0') << milliseconds;
                timeText->text = ss.str();

                /* int minutes = static_cast<int>(stats.elapsedTime) / 60;
                int seconds = static_cast<int>(stats.elapsedTime) % 60;
                ss << L" " << std::setw(2) << std::setfill(L'0') << minutes
                   << L":" << std::setw(2) << std::setfill(L'0') << seconds;
                timeText->text = ss.str();*/
            }

            // 現在のデス数の更新
            if (auto *deathText = w.TryGet<UIText>(currDeathEntity_)) {
                deathText->text = std::to_wstring(stats.deathCount);
            }

            const bool shouldWarn = IsTimeWarningActive(stats.elapsedTime, stats.timerRunning, stats.isPaused);
            if (shouldWarn) {
                warningBlinkTimer_ += dt;
            } else {
                warningBlinkTimer_ = 0.0f;
            }

            const float warningPeriod = cfg_GameUIWarningPeriod.Get();
            const float warningOnTime = cfg_GameUIWarningOnTime.Get();
            while (warningBlinkTimer_ >= warningPeriod) {
                warningBlinkTimer_ -= warningPeriod;
            }
            const bool blinkOn = shouldWarn && (warningBlinkTimer_ < warningOnTime);

            if (auto *warningPanel = w.TryGet<UIPanel>(warningOverlayEntity_)) {
                warningPanel->visible = blinkOn;
                warningPanel->color = {1.0f, 0.0f, 0.0f, blinkOn ? cfg_GameUIWarningOverlayAlpha.Get() : 0.0f};
            }

          /*  if (auto *warningText = w.TryGet<UIText>(warningTextEntity_)) {
                warningText->text = L"";
            }*/

           /* if (auto *warningText = w.TryGet<UIText>(warningTextEntity_)) {
                warningText->text = shouldWarn ? L"WARNING!" : L"";
            }*/

            if (auto *startImage = w.TryGet<UIImage>(startplayer_)) {
                if (stats.StartChack == true) {
                    startImage->opacity = 0.0f;
                }
            }

             // Start Charge Bar Update
             w.ForEach<PlayerMovement>([&](Entity, PlayerMovement& pm) {
                if (auto* barTr = w.TryGet<UITransform>(startChargeBarEntity_)) {
                    if (stats.StartChack) {
                        barTr->size.x = 0.0f;
                    } else {
                        const float maxTime = cfg_GameUIStartChargeMaxTime.Get();
                        float progress = std::clamp(pm.chargeTimer / maxTime, 0.0f, 1.0f);
                        const float maxWidth = cfg_GameUIStartChargeMaxWidth.Get();
                        barTr->size.x = maxWidth * progress;
                    }
                }
            });
            //スタートのカウントダウン時間の更新
            /*if (auto *starttimeText = w.TryGet<UIText>(startplayer_)) {
                std::wstringstream ss;
                float seconds = stats.StartCountDown;
                if (seconds < 0)
                    seconds = 0;
                ss << L"ChargeStart!";
                starttimeText->text = ss.str();

                if (stats.StartChack == true) {
                    starttimeText->text = L"";
                }
            }*/

            //fps表示の更新
            if (auto *fpsText = w.TryGet<UIText>(fpsTextEntity_)) {
                float fps = (dt > 0.0f) ? (1.0f / dt) : 0.0f;
                std::wstringstream ss;
                ss << L"FPS: " << std::fixed << std::setprecision(1) << fps;
                fpsText->text = ss.str();
            }

            //ポーズ表示の更新
            if (auto *pauseImg = w.TryGet<UIImage>(pauseTextEntity_)) {
                auto *pauseTransform = w.TryGet<UITransform>(pauseTextEntity_);
                if (pauseTransform) {
                    if (stats.isPaused) {
                        pauseImg->opacity = 1.0f;
                        pauseTransform->size = pauseTitleImgSize_;
                    } else {
                        pauseImg->opacity = 0.0f;
                        pauseTransform->size = {0.0f, 0.0f};
                    }
                }
            }

            // 縦線の更新
            if (auto *lineImg = w.TryGet<UIImage>(pauseLineEntity_)) {
                if (auto *lineTr = w.TryGet<UITransform>(pauseLineEntity_)) {
                    if (stats.isPaused) {
                        lineImg->opacity = 1.0f;
                        lineTr->size = pauseLineImgSize_;
                    } else {
                        lineImg->opacity = 0.0f;
                        lineTr->size = {0.0f, 0.0f};
                    }
                }
            }

            const bool showPauseMenu = stats.isPaused;
            if (auto *panel = w.TryGet<UIPanel>(pauseMenuPanelEntity_)) {
                panel->visible = showPauseMenu;
            }

            bool anyHovered = false;
            auto updateButtonVisible = [&](Entity buttonEntity, const std::wstring& normalPath, const std::wstring& hoverPath) {
                if (auto *tr = w.TryGet<UITransform>(buttonEntity)) {
                    tr->size = showPauseMenu ? pauseMenuButtonSize_ : DirectX::XMFLOAT2{0.0f, 0.0f};
                }
                if (auto *btn = w.TryGet<UIButton>(buttonEntity)) {
                    btn->enabled = showPauseMenu;
                }
                if (auto *img = w.TryGet<UIImage>(buttonEntity)) {
                    img->opacity = showPauseMenu ? 1.0f : 0.0f;
                    
                    if (showPauseMenu) {
                        bool isHoveredOrPressed = false;
                        if (auto *btnInfo = w.TryGet<UIButton>(buttonEntity)) {
                            isHoveredOrPressed = (btnInfo->state == UIButton::State::Hovered || btnInfo->state == UIButton::State::Pressed);
                        }
                        
                        // 画像切り替え
                        img->filePath = isHoveredOrPressed ? hoverPath : normalPath;
                        
                        if (isHoveredOrPressed) {
                            anyHovered = true;
                            // selectインジケーター位置更新
                            if (auto *selectTr = w.TryGet<UITransform>(pauseSelectIndicatorEntity_)) {
                                if (auto *btnTr = w.TryGet<UITransform>(buttonEntity)) {
                                    selectTr->position = {btnTr->position.x + cfg_GameUIPauseSelectOffsetX.Get(), btnTr->position.y + cfg_GameUIPauseSelectOffsetY.Get()};
                                }
                            }
                        }
                    }
                }
            };

            // オプションと終了は無効化されているため何もしないか、仮のパスを与える
            updateButtonVisible(pauseResumeButtonEntity_, L"./Assets/Textures/UI/PausedUI/BackGame1.png", L"./Assets/Textures/UI/PausedUI/BackGame.png");
            updateButtonVisible(pauseRetryButtonEntity_, L"./Assets/Textures/UI/PausedUI/Retray1.png", L"./Assets/Textures/UI/PausedUI/Retray.png");
            updateButtonVisible(pauseStageSelectButtonEntity_, L"./Assets/Textures/UI/PausedUI/StageSelect1.png", L"./Assets/Textures/UI/PausedUI/StageSelect.png");
            updateButtonVisible(pauseTitleButtonEntity_, L"./Assets/Textures/UI/PausedUI/BackTitle1.png", L"./Assets/Textures/UI/PausedUI/BackTitle.png");
            
            // 選択インジケーターの表示更新
            if (auto *selectImg = w.TryGet<UIImage>(pauseSelectIndicatorEntity_)) {
                if (auto *selectTr = w.TryGet<UITransform>(pauseSelectIndicatorEntity_)) {
                    if (showPauseMenu && anyHovered) {
                        selectImg->opacity = 1.0f;
                        selectTr->size = pauseSelectImgSize_;
                    } else {
                        selectImg->opacity = 0.0f;
                        selectTr->size = {0.0f, 0.0f};
                    }
                }
            }
            UpdateNonPauseUiOpacity(w, showPauseMenu);
        });

        // ステージの更新
        w.ForEach<StageProgress>([&](Entity e, StageProgress &sp) {
            RefreshRoomCount(sp.worldCount, sp.currentStage);

            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[0])) {
                std::wstringstream ss;
                ss << cachedRoomCount_;
                stageText->text = ss.str();
            }

            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[1])) {
                std::wstringstream ss;
                ss << sp.currentRoom;
                stageText->text = ss.str();
            }
        });

        // ルームの更新
    }

  private:
    void UpdateNonPauseUiOpacity(World &w, bool showPauseMenu) {
        if (showPauseMenu) {
            if (!pauseDimApplied_) {
                CaptureNonPauseUiBaseOpacity(w);
                pauseDimApplied_ = true;
            }
            ApplyNonPauseUiOpacity(w, std::clamp(cfg_GameUIPauseNonPauseOpacity.Get(), 0.0f, 1.0f));
            return;
        }
        if (pauseDimApplied_) {
            RestoreNonPauseUiOpacity(w);
            pauseBaseImageOpacity_.clear();
            pauseBaseTextAlpha_.clear();
            pauseBaseTextOutlineAlpha_.clear();
            pauseBasePanelAlpha_.clear();
            pauseDimApplied_ = false;
        }
    }

    void CaptureNonPauseUiBaseOpacity(World &w) {
        pauseBaseImageOpacity_.clear();
        pauseBaseTextAlpha_.clear();
        pauseBaseTextOutlineAlpha_.clear();
        pauseBasePanelAlpha_.clear();
        for (const Entity &entity : pauseDimmableEntities_) {
            if (auto *img = w.TryGet<UIImage>(entity)) {
                pauseBaseImageOpacity_[entity] = std::clamp(img->opacity, 0.0f, 1.0f);
            }
            if (auto *text = w.TryGet<UIText>(entity)) {
                pauseBaseTextAlpha_[entity] = std::clamp(text->color.w, 0.0f, 1.0f);
                pauseBaseTextOutlineAlpha_[entity] = std::clamp(text->outlineColor.w, 0.0f, 1.0f);
            }
            if (auto *panel = w.TryGet<UIPanel>(entity)) {
                pauseBasePanelAlpha_[entity] = std::clamp(panel->color.w, 0.0f, 1.0f);
            }
        }
    }

    void ApplyNonPauseUiOpacity(World &w, float opacityScale) {
        for (const Entity &entity : pauseDimmableEntities_) {
            if (auto *img = w.TryGet<UIImage>(entity)) {
                auto it = pauseBaseImageOpacity_.find(entity);
                if (it != pauseBaseImageOpacity_.end()) {
                    img->opacity = std::clamp(it->second * opacityScale, 0.0f, 1.0f);
                }
            }
            if (auto *text = w.TryGet<UIText>(entity)) {
                auto colorIt = pauseBaseTextAlpha_.find(entity);
                if (colorIt != pauseBaseTextAlpha_.end()) {
                    text->color.w = std::clamp(colorIt->second * opacityScale, 0.0f, 1.0f);
                }
                auto outlineIt = pauseBaseTextOutlineAlpha_.find(entity);
                if (outlineIt != pauseBaseTextOutlineAlpha_.end()) {
                    text->outlineColor.w = std::clamp(outlineIt->second * opacityScale, 0.0f, 1.0f);
                }
            }
            if (auto *panel = w.TryGet<UIPanel>(entity)) {
                auto it = pauseBasePanelAlpha_.find(entity);
                if (it != pauseBasePanelAlpha_.end()) {
                    panel->color.w = std::clamp(it->second * opacityScale, 0.0f, 1.0f);
                }
            }
        }
    }

    void RestoreNonPauseUiOpacity(World &w) {
        for (const Entity &entity : pauseDimmableEntities_) {
            if (auto *img = w.TryGet<UIImage>(entity)) {
                auto it = pauseBaseImageOpacity_.find(entity);
                if (it != pauseBaseImageOpacity_.end()) {
                    img->opacity = std::clamp(it->second, 0.0f, 1.0f);
                }
            }
            if (auto *text = w.TryGet<UIText>(entity)) {
                auto colorIt = pauseBaseTextAlpha_.find(entity);
                if (colorIt != pauseBaseTextAlpha_.end()) {
                    text->color.w = std::clamp(colorIt->second, 0.0f, 1.0f);
                }
                auto outlineIt = pauseBaseTextOutlineAlpha_.find(entity);
                if (outlineIt != pauseBaseTextOutlineAlpha_.end()) {
                    text->outlineColor.w = std::clamp(outlineIt->second, 0.0f, 1.0f);
                }
            }
            if (auto *panel = w.TryGet<UIPanel>(entity)) {
                auto it = pauseBasePanelAlpha_.find(entity);
                if (it != pauseBasePanelAlpha_.end()) {
                    panel->color.w = std::clamp(it->second, 0.0f, 1.0f);
                }
            }
        }
    }

    static bool IsTimeWarningActive(float elapsedTime, bool timerRunning, bool isPaused) {
        return timerRunning && !isPaused && elapsedTime > 0.0f &&
               elapsedTime <= cfg_GameUIWarningThresholdSeconds.Get();
    }

    static int CountRoomsInStage(int world, int stage) {
        int count = 0;
        while (ResolveStageRoomCsvPath(world, stage, count + 1)) {
            ++count;
        }
        return std::max(count, 1);
    }

    void RefreshRoomCount(int world, int stage) {
        if (cachedStage_ == stage)
            return;
        cachedStage_ = stage;
        if (cachedWorldCount == world)
            cachedWorldCount = world;
        cachedRoomCount_ = CountRoomsInStage(world, stage);
    }

};
