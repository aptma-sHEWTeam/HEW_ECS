/*****************************************************************//**
 * \file   CountUIComponent.h
 * \brief  開始時のUIカウントダウンとそれに伴うプレイヤーフリーズの実装
 * 
 * \author 亀多彩日
 * \date   11/18
 *         11/19　作業一時停止　今後必要になったら再開
 * 
 * 3.2.1.Go　描画→削除　カウント時プレイヤー等の動きを停止
 *********************************************************************/

#pragma once
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/UIComponents.h"
#include "components/GameStats.h"
#include <string>
#include <functional>

// カウントダウン　UIコンポーネント
struct CountUI : IComponent {
    int framesPerStep    = 30; //3->2->1の切り替えフレーム数
    int goDurationFrames = 120;//Go表示のフレーム数

    //内部状態
    int currentStep  = 0; //0="3",1="2",2="1",3="GO" //数値＝"表示"
    int frameCounter = 0;
    bool active = true;

    //カウントダウン中にゲームを停止するか(既定　true)
    bool pauseGameDuringCountDown = true;

    //カウント完了時のコールバック
   // std::function<void(world &, Entity)> onFinished;

    CountUI() = default;
};

/**
 * CountUI を更新するシステム.
 * 
 * ※※GameSceneのマイフレーム更新処理内でCountUISystem::Update(world)を呼び出して
 */
struct CountUISystem {};




