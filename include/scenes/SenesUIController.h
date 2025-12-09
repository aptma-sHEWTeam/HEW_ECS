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
    Entity startplayer_;
    

    void OnUpdate(World &w, Entity self, float dt) override {
        w.ForEach<GameStatus>([&](Entity e, GameStatus &stats) {
            // 経過時間の更新
            if (!stats.isPaused) {
                stats.elapsedTime -= dt;
            }

            //スタート時間の更新
            if (stats.StartCountDown > 0) {
                stats.StartCountDown -= dt;
            } 
            else {
                stats.StartChack = true;
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
                int minutes = static_cast<int>(stats.elapsedTime) / 60;
                int seconds = static_cast<int>(stats.elapsedTime) % 60;
                ss << L" " << std::setw(2) << std::setfill(L'0') << minutes
                   << L":" << std::setw(2) << std::setfill(L'0') << seconds;
                timeText->text = ss.str();
            }

            //スタートのカウントダウン時間の更新
            if (auto *starttimeText = w.TryGet<UIText>(startplayer_)) {
                std::wstringstream ss;
                float seconds = stats.StartCountDown;
                if (seconds < 0)
                    seconds = 0;
                ss << L"ready" << ceil(seconds);
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
            RefreshRoomCount(sp.currentStage);

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
    static int CountRoomsInStage(int stage) {
        int count = 0;
        while (ResolveStageRoomCsvPath(stage, count + 1)) {
            ++count;
        }
        return std::max(count, 1);
    }

    void RefreshRoomCount(int stage) {
        if (cachedStage_ == stage) return;
        cachedStage_ = stage;
        cachedRoomCount_ = CountRoomsInStage(stage);
    }
};
