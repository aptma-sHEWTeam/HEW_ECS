/*****************************************************************//**
 * \file   CountUIComponent.h
 * \brief  開始時のUIカウントダウンとそれに伴うプレイヤーフリーズの実装
 * 
 * \author 亀多彩日
 * \date   11/18
 * 
 * 3.2.1.Go　描画→削除　カウント時プレイヤー等の動きを停止
 *********************************************************************/

#pragma once
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include <DirectXMath.h>
#include <string>
#include <functional>
