/**
 * @file GameStats.h
 * @brief ゲームの統計情報を定義するヘッダーファイル
 */
#pragma once

#include "components/Component.h"
#include<vector>

/**
 * @struct GameStats
 * @brief ゲームの統計情報
 */
struct GameStatus : IComponent {
 int score =0;
 int enemiesDefeated =0;
 bool StartChack = false;     //スタートできるか判定
 float StartCountDown =0.0f; //カウントダウン用
 float elapsedTime = 0.0f;
 bool isPaused = false;
 bool timerRunning = false;   //タイマー計測中か
 bool waitingForPlayerMove = true; //動き出すまでタイマーを停止
};