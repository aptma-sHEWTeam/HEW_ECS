/**
 * @file RoomUIComponents.h
 * @brief RoomUI用コンポーネント
 */
#pragma once

#include "components/Component.h"

struct RoomUI : IComponent {
    int currentRoomUVNo = 0; // 現在のルーム番号
    int wholeRoomUVNo   = 0; // ステージ全体のルーム数
    int RoomUVWidth = 0;     // ルームに使用するテクスチャの横幅
    int RoomUVHeight = 0;    // ルームに使用するテクスチャの縦幅
    int RoomPosU = 0;        // ルームに使用するテクスチャのU座標
    int RoomPosV = 0;        // ルームに使用するテクスチャのV座標
};
