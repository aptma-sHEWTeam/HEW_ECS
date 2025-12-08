/*****************************************************************//**
 * \file   MouseCoordinate.h
 * \brief　毎フレームマウスの座標更新・表示
 *
 *
 * \author 飯島英菜・亀多彩日
 * \date   11/4
 *********************************************************************/
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include"input/InputSystem.h"
#include"components/Component.h"
#include"ecs/World.h"
#include<cstdint>
#include<cstring>

#include<dwrite.h>
#include<d2d1.h>
#include<wrl.h>
#include<sstream>

struct MouseCoordinate : Behaviour
{
    void OnStart(World& w, Entity self) override
    {
        MouseX = GetInput().GetMouseX();
        MouseY = GetInput().GetMouseY();
        MouseDeltaX = GetInput().GetMouseDeltaX();
        MouseDeltaY = GetInput().GetMouseDeltaY();
    }

    /**
     * @brief 毎フレーム座標更新
     *
     *
     */
    void OnUpdate(World& w, Entity self,float dt) override
    {
        MouseX = GetInput().GetMouseX();
        MouseY = GetInput().GetMouseY();
        MouseDeltaX = GetInput().GetMouseDeltaX();
        MouseDeltaY = GetInput().GetMouseDeltaY();
    }

    /**
     * @brief　毎フレームの描画
     *
     */
    void OnRender()
    {

    }

    int MouseX = 0; //マウスX座標
    int MouseY = 0; //マウスY座標
    int MouseDeltaX = 0;    //マウスX座標のデータ
    int MouseDeltaY = 0;    //マウスY座標のデータ
};
