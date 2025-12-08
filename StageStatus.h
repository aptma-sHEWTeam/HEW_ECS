#pragma once
/**
 * @file StageStatus.h
 * @brief ゲームの統計情報を定義するヘッダーファイル
 */
#pragma once

#include "components/Component.h"

/**
 * @struct GameStats
 * @brief ゲームの統計情報
 */
struct StageSelectStatus : IComponent {
    int StageCount = 1;
};
