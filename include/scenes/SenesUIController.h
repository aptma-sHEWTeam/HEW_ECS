#pragma once

#include "components/Component.h"
#include "components/UIComponents.h"
#include "components/GameStats.h"
#include "components/StageComponents.h"
#include "ecs/Entity.h"
#include "ecs/World.h"
#include "systems/UISystem.h"
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


    void OnUpdate(World &w, Entity self, float dt) override {
        w.ForEach<GameStats>([&](Entity e, GameStats &stats) {
            // 経過時間の更新
            if (!stats.isPaused) {
                stats.elapsedTime += dt;
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
                   << L":" << std::setw(2) << std::setfill(L'10') << seconds;
                timeText->text = ss.str();
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
            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[0])) {
                std::wstringstream ss;
                ss << L"Room : " << sp.currentStage << "/";
                stageText->text = ss.str();
            }

            if (auto *stageText = w.TryGet<UIText>(stageTextEntity_[1])) {
                std::wstringstream ss;
                ss << L"" << sp.selectStage;
                ss << L"ルーム: " << sp.currentStage;
                stageText->text = ss.str();
            }
        });

        // ルームの更新

    }
};