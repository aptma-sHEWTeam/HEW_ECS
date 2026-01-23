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
    Entity warningOverlayEntity_;
    Entity warningTextEntity_;
    float warningBlinkTimer_ = 0.0f;

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

            const bool shouldWarn = IsTimeWarningActive(stats.elapsedTime, stats.timerRunning, stats.isPaused);
            if (shouldWarn) {
                warningBlinkTimer_ += dt;
            } else {
                warningBlinkTimer_ = 0.0f;
            }

            const float warningPeriod = 0.6f;
            const float warningOnTime = 0.2f;
            while (warningBlinkTimer_ >= warningPeriod) {
                warningBlinkTimer_ -= warningPeriod;
            }
            const bool blinkOn = shouldWarn && (warningBlinkTimer_ < warningOnTime);

            if (auto *warningPanel = w.TryGet<UIPanel>(warningOverlayEntity_)) {
                warningPanel->visible = blinkOn;
                warningPanel->color = {1.0f, 0.0f, 0.0f, blinkOn ? 0.35f : 0.0f};
            }

            if (auto *warningText = w.TryGet<UIText>(warningTextEntity_)) {
                warningText->text = shouldWarn ? L"WARNING!" : L"";
            }

            //スタートのカウントダウン時間の更新
            if (auto *starttimeText = w.TryGet<UIText>(startplayer_)) {
                std::wstringstream ss;
                float seconds = stats.StartCountDown;
                if (seconds < 0)
                    seconds = 0;
                ss << L"Charge!";
                starttimeText->text = ss.str();

                if (stats.StartChack == true) {
                    starttimeText->text = L"";
                }
            }

            //fps表示の更新
            if (auto *fpsText = w.TryGet<UIText>(fpsTextEntity_)) {
                float fps = (dt > 0.0f) ? (1.0f / dt) : 0.0f;
                std::wstringstream ss;
                ss << L"FPS: " << std::fixed << std::setprecision(1) << fps;
                fpsText->text = ss.str();
            }

            //ポーズ表示の更新
            if (auto *pauseText = w.TryGet<UIText>(pauseTextEntity_)) {
                auto *pauseTransform = w.TryGet<UITransform>(pauseTextEntity_);
                if (pauseTransform) {
                    if (stats.isPaused) {
                        pauseText->text = L"PAUSED";
                        pauseTransform->size = {400.0f, 100.0f};
                    } else {
                        pauseText->text = L"";
                        pauseTransform->size = {0.0f, 0.0f};
                    }
                }
            }
        });

        // ステージの更新
        w.ForEach<StageProgress>([&](Entity e, StageProgress &sp) {
            RefreshRoomCount(sp.worldCount, sp.currentStage);

            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[0])) {
                std::wstringstream ss;
                // ss << 11;
                ss << cachedRoomCount_;
                //ss << L"Room : " << sp.currentRoom << L"/";
                stageText->text = ss.str();
            }

            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[1])) {
                std::wstringstream ss;
                // ss << 1;
                ss << sp.currentRoom;
                //  ss << cachedRoomCount_;
                stageText->text = ss.str();
            }
        });

        // ルームの更新
    }

  private:
    static constexpr float kWarningThresholdSeconds = 3.0f;

    static bool IsTimeWarningActive(float elapsedTime, bool timerRunning, bool isPaused) {
        return timerRunning && !isPaused && elapsedTime > 0.0f &&
               elapsedTime <= kWarningThresholdSeconds;
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
