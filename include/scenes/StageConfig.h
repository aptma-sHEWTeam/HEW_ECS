/**
 * @file StageFactory.h
 * @brief ステージオブジェクト生成用のConfigVar定義
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include <string>
#include "config/ConfigVar.h"

// =========================================
// ステージ関連ConfigVars
// =========================================

// プレイヤー設定
inline ConfigVar<float> cfg_PlayerScale{"Game", "PlayerScale", 0.8f};
inline ConfigVar<float> cfg_PlayerR{"Game", "PlayerColorR", 0.0f};
inline ConfigVar<float> cfg_PlayerG{"Game", "PlayerColorG", 0.0f};
inline ConfigVar<float> cfg_PlayerB{"Game", "PlayerColorB", 1.0f};
inline ConfigVar<float> cfg_PlayerStartY{"Game", "PlayerStartY", 5.0f};
inline ConfigVar<float> cfg_PlayerHeight{"Game", "PlayerHeight", 2.0f};

// 床設定
inline ConfigVar<float> cfg_FloorR{"Game", "FloorColorR", 0.5f};
inline ConfigVar<float> cfg_FloorG{"Game", "FloorColorG", 0.5f};
inline ConfigVar<float> cfg_FloorB{"Game", "FloorColorB", 0.5f};
inline ConfigVar<float> cfg_FloorYOffset{"Game", "FloorYOffset", -2.0f};
inline ConfigVar<float> cfg_FloorThickness{"Game", "FloorThickness", 0.2f};

// スタート地点設定
inline ConfigVar<float> cfg_StartR{"Game", "StartColorR", 0.0f};
inline ConfigVar<float> cfg_StartG{"Game", "StartColorG", 0.0f};
inline ConfigVar<float> cfg_StartB{"Game", "StartColorB", 1.0f};

// ゴール地点設定
inline ConfigVar<float> cfg_GoalR{"Game", "GoalColorR", 1.0f};
inline ConfigVar<float> cfg_GoalG{"Game", "GoalColorG", 1.0f};
inline ConfigVar<float> cfg_GoalB{"Game", "GoalColorB", 0.0f};

// 壁設定
inline ConfigVar<float> cfg_WallR{"Game", "WallColorR", 1.0f};
inline ConfigVar<float> cfg_WallG{"Game", "WallColorG", 1.0f};
inline ConfigVar<float> cfg_WallB{"Game", "WallColorB", 1.0f};

inline ConfigVar<float> cfg_FloorWallR{"Game", "FloorWallColorR", 0.5f};
inline ConfigVar<float> cfg_FloorWallG{"Game", "FloorWallColorG", 0.5f};
inline ConfigVar<float> cfg_FloorWallB{"Game", "FloorWallColorB", 0.5f};
inline ConfigVar<float> cfg_WallSize{"Game", "WallSize", 3.0f};

// FBXパス設定
inline ConfigVar<std::string> cfg_PlayerFBXPass{"Player", "PlayerFBXPass", "Assets/Models/Player/obj_player.fbx"};
inline ConfigVar<std::string> cfg_FloorFBXPass{"Game", "FloorFBXPass", "Assets/Models/StageObj/Ground/obj_ground.fbx"};
inline ConfigVar<std::string> cfg_WallFBXPass{"Game", "WallFBXPass", "Assets/Models/StageObj/Wall/obj_wall.fbx"};

// UI設定
inline ConfigVar<float> cfg_UICountPosX{"UI", "CountPosX", 20.0f};
inline ConfigVar<float> cfg_UICountPosY{"UI", "CountPosY", 170.0f};
inline ConfigVar<float> cfg_UICountW{"UI", "CountWidth", 200.0f};
inline ConfigVar<float> cfg_UICountH{"UI", "CountHeight", 40.0f};
inline ConfigVar<float> cfg_UICountR{"UI", "CountColorR", 0.0f};
inline ConfigVar<float> cfg_UICountG{"UI", "CountColorG", 1.0f};
inline ConfigVar<float> cfg_UICountB{"UI", "CountColorB", 1.0f};

inline ConfigVar<std::string> cfg_RoomPath{"UI", "RoomPNGPass", "Assets/Textures/Count.png"};

// 衝突設定
inline ConfigVar<float> cfg_CollisionCellSize{"Game", "CollisionCellSize", 20.0f};

// ゲーム設定
inline ConfigVar<float> cfg_LimitTime{"Game", "LimitTime", 10.0f};

// スタート発光設定
inline ConfigVar<float> cfg_StartEmissiveR{"Game", "StartEmissiveR", 0.0f};
inline ConfigVar<float> cfg_StartEmissiveG{"Game", "StartEmissiveG", 0.5f};
inline ConfigVar<float> cfg_StartEmissiveB{"Game", "StartEmissiveB", 1.0f};
inline ConfigVar<float> cfg_StartEmissiveIntensity{"Game", "StartEmissiveIntensity", 1.5f};
inline ConfigVar<float> cfg_StartPulseMin{"Game", "StartPulseMin", 4.0f};
inline ConfigVar<float> cfg_StartPulseMax{"Game", "StartPulseMax", 10.0f};
inline ConfigVar<float> cfg_StartPulseSpeed{"Game", "StartPulseSpeed", 1.0f};

// ゴール発光設定
inline ConfigVar<float> cfg_GoalEmissiveR{"Game", "GoalEmissiveR", 1.0f};
inline ConfigVar<float> cfg_GoalEmissiveG{"Game", "GoalEmissiveG", 0.8f};
inline ConfigVar<float> cfg_GoalEmissiveB{"Game", "GoalEmissiveB", 0.0f};
inline ConfigVar<float> cfg_GoalEmissiveIntensity{"Game", "GoalEmissiveIntensity", 2.0f};
inline ConfigVar<float> cfg_GoalPulseMin{"Game", "GoalPulseMin", 4.0f};
inline ConfigVar<float> cfg_GoalPulseMax{"Game", "GoalPulseMax", 10.0f};
inline ConfigVar<float> cfg_GoalPulseSpeed{"Game", "GoalPulseSpeed", 1.0f};

// ライト範囲設定
inline ConfigVar<float> cfg_StartLightRange{"Game", "StartLightRange", 5.0f};
inline ConfigVar<float> cfg_GoalLightRange{"Game", "GoalLightRange", 8.0f};
