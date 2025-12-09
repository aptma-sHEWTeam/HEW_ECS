/**
 * @file StageFactory.h
 * @brief �X�e�[�W�I�u�W�F�N�g�����p��ConfigVar��`
 * @author �R���z
 * @date 2025
 * @version 1.0
 */
#pragma once

#include <string>
#include "config/ConfigVar.h"

// =========================================
// �X�e�[�W�֘AConfigVars
// =========================================

// �v���C���[�ݒ�
inline ConfigVar<float> cfg_PlayerScale{"Stage.Player.Appearance", "PlayerScale", 0.8f};
inline ConfigVar<float> cfg_PlayerR{"Stage.Player.Appearance", "PlayerColorR", 0.0f};
inline ConfigVar<float> cfg_PlayerG{"Stage.Player.Appearance", "PlayerColorG", 0.0f};
inline ConfigVar<float> cfg_PlayerB{"Stage.Player.Appearance", "PlayerColorB", 1.0f};
inline ConfigVar<float> cfg_PlayerStartY{"Stage.Player.Appearance", "PlayerStartY", 5.0f};
inline ConfigVar<float> cfg_PlayerHeight{"Stage.Player.Appearance", "PlayerHeight", 2.0f};

// ���ݒ�
inline ConfigVar<float> cfg_FloorR{"Stage.Floor.Visual", "FloorColorR", 0.5f};
inline ConfigVar<float> cfg_FloorG{"Stage.Floor.Visual", "FloorColorG", 0.5f};
inline ConfigVar<float> cfg_FloorB{"Stage.Floor.Visual", "FloorColorB", 0.5f};
inline ConfigVar<float> cfg_FloorYOffset{"Stage.Floor.Geometry", "FloorYOffset", -2.0f};
inline ConfigVar<float> cfg_FloorThickness{"Stage.Floor.Geometry", "FloorThickness", 0.2f};

// �X�^�[�g�n�_�ݒ�
inline ConfigVar<float> cfg_StartR{"Stage.Start.Visual", "StartColorR", 0.0f};
inline ConfigVar<float> cfg_StartG{"Stage.Start.Visual", "StartColorG", 0.0f};
inline ConfigVar<float> cfg_StartB{"Stage.Start.Visual", "StartColorB", 1.0f};

// �S�[���n�_�ݒ�
inline ConfigVar<float> cfg_GoalR{"Stage.Goal.Visual", "GoalColorR", 1.0f};
inline ConfigVar<float> cfg_GoalG{"Stage.Goal.Visual", "GoalColorG", 1.0f};
inline ConfigVar<float> cfg_GoalB{"Stage.Goal.Visual", "GoalColorB", 0.0f};

// �ǐݒ�
inline ConfigVar<float> cfg_WallR{"Stage.Wall.Visual", "WallColorR", 1.0f};
inline ConfigVar<float> cfg_WallG{"Stage.Wall.Visual", "WallColorG", 1.0f};
inline ConfigVar<float> cfg_WallB{"Stage.Wall.Visual", "WallColorB", 1.0f};

// 環境光・平行光・ポイントライトデフォルト
inline ConfigVar<float> cfg_AmbientR{"Lighting.Ambient", "ColorR", 0.15f};
inline ConfigVar<float> cfg_AmbientG{"Lighting.Ambient", "ColorG", 0.15f};
inline ConfigVar<float> cfg_AmbientB{"Lighting.Ambient", "ColorB", 0.20f};
inline ConfigVar<float> cfg_AmbientIntensity{"Lighting.Ambient", "Intensity", 1.0f};
inline ConfigVar<bool> cfg_DirLightEnabled{"Lighting.Directional", "Enabled", true};
inline ConfigVar<float> cfg_DirLightX{"Lighting.Directional", "DirX", 0.577f};
inline ConfigVar<float> cfg_DirLightY{"Lighting.Directional", "DirY", -0.577f};
inline ConfigVar<float> cfg_DirLightZ{"Lighting.Directional", "DirZ", 0.577f};
inline ConfigVar<float> cfg_DirLightR{"Lighting.Directional", "ColorR", 1.0f};
inline ConfigVar<float> cfg_DirLightG{"Lighting.Directional", "ColorG", 1.0f};
inline ConfigVar<float> cfg_DirLightB{"Lighting.Directional", "ColorB", 1.0f};
inline ConfigVar<float> cfg_DirLightIntensity{"Lighting.Directional", "Intensity", 1.0f};
inline ConfigVar<float> cfg_PointLightConst{"Lighting.Point.Default", "Constant", 0.5f};
inline ConfigVar<float> cfg_PointLightLinear{"Lighting.Point.Default", "Linear", 0.04f};
inline ConfigVar<float> cfg_PointLightQuadratic{"Lighting.Point.Default", "Quadratic", 0.007f};
inline ConfigVar<float> cfg_PointLightRange{"Lighting.Point.Default", "Range", 10.0f};
inline ConfigVar<float> cfg_BakeLightSpacing{"Lighting.Bake", "Spacing", 6.0f};
inline ConfigVar<float> cfg_BakeLightHeight{"Lighting.Bake", "Height", 2.5f};
inline ConfigVar<float> cfg_BakeLightIntensity{"Lighting.Bake", "Intensity", 0.7f};
inline ConfigVar<float> cfg_BakeLightR{"Lighting.Bake", "ColorR", 0.9f};
inline ConfigVar<float> cfg_BakeLightG{"Lighting.Bake", "ColorG", 0.9f};
inline ConfigVar<float> cfg_BakeLightB{"Lighting.Bake", "ColorB", 0.9f};

inline ConfigVar<float> cfg_FloorWallR{"Stage.Wall.Visual", "FloorWallColorR", 0.5f};
inline ConfigVar<float> cfg_FloorWallG{"Stage.Wall.Visual", "FloorWallColorG", 0.5f};
inline ConfigVar<float> cfg_FloorWallB{"Stage.Wall.Visual", "FloorWallColorB", 0.5f};
inline ConfigVar<float> cfg_WallSize{"Stage.Wall.Geometry", "WallSize", 1.0f};

// FBX�p�X�ݒ�
inline ConfigVar<std::string> cfg_PlayerFBXPass{"Player.Asset", "PlayerFBXPass", "Assets/Models/Player/obj_player.fbx"};
inline ConfigVar<std::string> cfg_FloorFBXPass{"Stage.Floor.Asset", "FloorFBXPass", "Assets/Models/StageObj/Ground/floor_cube.fbx"};
inline ConfigVar<std::string> cfg_WallFBXPass{"Stage.Wall.Asset", "WallFBXPass", "Assets/Models/StageObj/Wall/obj_wall.fbx"};
inline ConfigVar<std::string> cfg_HalfWallFBXPass{"Stage.Wall.Asset", "WallFBXPass", "Assets/Models/StageObj/Wall/obj_wall_half.fbx"};
inline ConfigVar<std::string> cfg_StartFBXPass{"Stage.Start.Asset", "StartFBXPass", "Assets/Models/StageObj/Start/obj_start.fbx"};
inline ConfigVar<std::string> cfg_GoalFBXPass{"Stage.Goal.Asset", "GoalFBXPass", "Assets/Models/StageObj/Goal/obj_goal.fbx"};
inline ConfigVar<std::string> cfg_MovingObstacleFBXPass{"Stage.MovingObstacle.Asset", "MovingObstacleFBXPass", "Assets/Models/StageObj/Move/vehicle.fbx"};
inline ConfigVar<std::string> cfg_DashBoardFBXPass{"Stage.DashBoard.Asset", "DashBoardFBXPass", "Assets/Models/StageObj/SpeedUp/obj_speedup.fbx"};
inline ConfigVar<std::string> cfg_AObstacleFBXPass{"Stage.PC.Asset","AObstacle","Assets/Models/StageObj/PC/PC.fbx"};
inline ConfigVar<std::string> cfg_BObstacleFBXPass{"Stage.GasPipe.Asset", "BObstacleFBXPass", "Assets/Models/StageObj/GasPipe/obj_obstacleB.fbx"};
inline ConfigVar<std::string> cfg_CObstacleFBXPass{"Stage.Cube.Asset", "CObstacleFBXPass", "Assets/Models/StageObj/Cub/Cube.fbx"};


// UI�ݒ�
inline ConfigVar<float> cfg_UICountPosX{"UI.StageHUD.Counter", "CountPosX", 20.0f};
inline ConfigVar<float> cfg_UICountPosY{"UI.StageHUD.Counter", "CountPosY", 170.0f};
inline ConfigVar<float> cfg_UICountW{"UI.StageHUD.Counter", "CountWidth", 200.0f};
inline ConfigVar<float> cfg_UICountH{"UI.StageHUD.Counter", "CountHeight", 40.0f};
inline ConfigVar<float> cfg_UICountR{"UI.StageHUD.Counter", "CountColorR", 0.0f};
inline ConfigVar<float> cfg_UICountG{"UI.StageHUD.Counter", "CountColorG", 1.0f};
inline ConfigVar<float> cfg_UICountB{"UI.StageHUD.Counter", "CountColorB", 1.0f};

inline ConfigVar<std::string> cfg_RoomPath{"UI.StageHUD.Asset", "RoomPNGPass", "Assets/Textures/Count.png"};

// �Փːݒ�
inline ConfigVar<float> cfg_CollisionCellSize{"Stage.Collision", "CollisionCellSize", 20.0f};

// �Q�[���ݒ�
inline ConfigVar<float> cfg_LimitTime{"Stage.Rule", "LimitTime", 10.0f};

// �X�^�[�g�����ݒ�
inline ConfigVar<float> cfg_StartEmissiveR{"Stage.Start.Emissive", "StartEmissiveR", 0.0f};
inline ConfigVar<float> cfg_StartEmissiveG{"Stage.Start.Emissive", "StartEmissiveG", 0.5f};
inline ConfigVar<float> cfg_StartEmissiveB{"Stage.Start.Emissive", "StartEmissiveB", 1.0f};
inline ConfigVar<float> cfg_StartEmissiveIntensity{"Stage.Start.Emissive", "StartEmissiveIntensity", 1.5f};
inline ConfigVar<float> cfg_StartPulseMin{"Stage.Start.Emissive", "StartPulseMin", 4.0f};
inline ConfigVar<float> cfg_StartPulseMax{"Stage.Start.Emissive", "StartPulseMax", 10.0f};
inline ConfigVar<float> cfg_StartPulseSpeed{"Stage.Start.Emissive", "StartPulseSpeed", 1.0f};

// �S�[�������ݒ�
inline ConfigVar<float> cfg_GoalEmissiveR{"Stage.Goal.Emissive", "GoalEmissiveR", 1.0f};
inline ConfigVar<float> cfg_GoalEmissiveG{"Stage.Goal.Emissive", "GoalEmissiveG", 0.8f};
inline ConfigVar<float> cfg_GoalEmissiveB{"Stage.Goal.Emissive", "GoalEmissiveB", 0.0f};
inline ConfigVar<float> cfg_GoalEmissiveIntensity{"Stage.Goal.Emissive", "GoalEmissiveIntensity", 2.0f};
inline ConfigVar<float> cfg_GoalPulseMin{"Stage.Goal.Emissive", "GoalPulseMin", 4.0f};
inline ConfigVar<float> cfg_GoalPulseMax{"Stage.Goal.Emissive", "GoalPulseMax", 10.0f};
inline ConfigVar<float> cfg_GoalPulseSpeed{"Stage.Goal.Emissive", "GoalPulseSpeed", 1.0f};

// ���C�g�͈͐ݒ�
inline ConfigVar<float> cfg_StartLightRange{"Stage.Start.Light", "StartLightRange", 5.0f};
inline ConfigVar<float> cfg_GoalLightRange{"Stage.Goal.Light", "GoalLightRange", 8.0f};

