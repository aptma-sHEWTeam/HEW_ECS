/*****************************************************************//**
 * \file   CountUIComponent.h
 * \brief  2D 開始時のUIカウントダウンとそれに伴うプレイヤーフリーズの実装
 * 
 * \author 亀多彩日
 * \date   11/18
 * 
 * 3.2.1.Goを描画→消去　カウントしている間はプレイヤー・タイマー停止
 *********************************************************************/
#pragma once
#include "pch.h"
#include "scenes/Game.h"
#include "systems/UISystem.h"
#include "graphics/TextSystem.h"
#include "components/UIComponents.h"
#include <DirectXMath.h>
#include <string>
#include <functional>