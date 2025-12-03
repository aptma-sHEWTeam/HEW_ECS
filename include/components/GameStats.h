/**
 * @file GameStats.h
 * @brief ゲームの統計情報を定義するヘッダーファイル
 */
#pragma once

#include "components/Component.h"

/**
 * @struct GameStats
 * @brief ゲームの統計情報
 */
struct GameStatus : IComponent {
 int score =0;
 int enemiesDefeated =0;
 float elapsedTime =0.0f;
 bool isPaused = false;
};