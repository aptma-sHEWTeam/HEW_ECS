/**
 * @file StageFactory.h
 * @brief ステージオブジェクト用のConfigVar定義
 * @author �R���z
 * @date 2025
 * @version 1.0
 */
#pragma once

#include <string>
#include <xaudio2.h>
#include "config/ConfigVar.h"

// =========================================
// ステージ関連 ConfigVars
// =========================================

// プレイヤー見た目設定
inline ConfigVar<float> cfg_PlayerScale{"Stage.Player.Appearance", "PlayerScale", 0.8f, "プレイヤーモデルのスケール"};
inline ConfigVar<float> cfg_PlayerR{"Stage.Player.Appearance", "PlayerColorR", 0.0f, "プレイヤー色 R"};
inline ConfigVar<float> cfg_PlayerG{"Stage.Player.Appearance", "PlayerColorG", 0.0f, "プレイヤー色 G"};
inline ConfigVar<float> cfg_PlayerB{"Stage.Player.Appearance", "PlayerColorB", 1.0f, "プレイヤー色 B"};
inline ConfigVar<float> cfg_PlayerStartY{"Stage.Player.Appearance", "PlayerStartY", 8.0f, "プレイヤーの初期Y位置"};
inline ConfigVar<float> cfg_PlayerHeight{"Stage.Player.Appearance", "PlayerHeight", 2.0f, "プレイヤーのカプセル高さ"};

// 床設定
inline ConfigVar<float> cfg_FloorR{"Stage.Floor.Visual", "FloorColorR", 0.5f, "床の色 R"};
inline ConfigVar<float> cfg_FloorG{"Stage.Floor.Visual", "FloorColorG", 0.5f, "床の色 G"};
inline ConfigVar<float> cfg_FloorB{"Stage.Floor.Visual", "FloorColorB", 0.5f, "床の色 B"};
inline ConfigVar<float> cfg_FloorYOffset{"Stage.Floor.Geometry", "FloorYOffset", -2.5f, "床メッシュのYオフセット"};
inline ConfigVar<float> cfg_FloorThickness{"Stage.Floor.Geometry", "FloorThickness", 0.2f, "床の厚み"};

// スタートマーカー見た目
inline ConfigVar<float> cfg_StartR{"Stage.Start.Visual", "StartColorR", 0.0f, "スタートマーカーの色 R"};
inline ConfigVar<float> cfg_StartG{"Stage.Start.Visual", "StartColorG", 0.0f, "スタートマーカーの色 G"};
inline ConfigVar<float> cfg_StartB{"Stage.Start.Visual", "StartColorB", 1.0f, "スタートマーカーの色 B"};

// ゴールマーカー見た目
inline ConfigVar<float> cfg_GoalR{"Stage.Goal.Visual", "GoalColorR", 1.000000f, "ゴールマーカーの色 R"};
inline ConfigVar<float> cfg_GoalG{"Stage.Goal.Visual", "GoalColorG", 0.000000f, "ゴールマーカーの色 G"};
inline ConfigVar<float> cfg_GoalB{"Stage.Goal.Visual", "GoalColorB", 0.000000f, "ゴールマーカーの色 B"};

// 壁見た目
inline ConfigVar<float> cfg_WallR{"Stage.Wall.Visual", "WallColorR", 1.0f, "壁の色 R"};
inline ConfigVar<float> cfg_WallG{"Stage.Wall.Visual", "WallColorG", 1.0f, "壁の色 G"};
inline ConfigVar<float> cfg_WallB{"Stage.Wall.Visual", "WallColorB", 1.0f, "壁の色 B"};

// 環境光・平行光・ポイントライトデフォルト
inline ConfigVar<float> cfg_AmbientR{"Lighting.Ambient", "ColorR", 0.0f, "環境光の色 R"};
inline ConfigVar<float> cfg_AmbientG{"Lighting.Ambient", "ColorG", 0.0f, "環境光の色 G"};
inline ConfigVar<float> cfg_AmbientB{"Lighting.Ambient", "ColorB", 0.0f, "環境光の色 B"};
inline ConfigVar<float> cfg_AmbientIntensity{"Lighting.Ambient", "Intensity", 0.0, "環境光の強さ"};
inline ConfigVar<bool>  cfg_DirLightEnabled{"Lighting.Directional", "Enabled", false, "平行光の有効フラグ"};
inline ConfigVar<float> cfg_DirLightX{"Lighting.Directional", "DirX", 0.577f, "平行光の向き X"};
inline ConfigVar<float> cfg_DirLightY{"Lighting.Directional", "DirY", -0.577f, "平行光の向き Y"};
inline ConfigVar<float> cfg_DirLightZ{"Lighting.Directional", "DirZ", 0.577f, "平行光の向き Z"};
inline ConfigVar<float> cfg_DirLightR{"Lighting.Directional", "ColorR", 1.0f, "平行光の色 R"};
inline ConfigVar<float> cfg_DirLightG{"Lighting.Directional", "ColorG", 1.0f, "平行光の色 G"};
inline ConfigVar<float> cfg_DirLightB{"Lighting.Directional", "ColorB", 1.0f, "平行光の色 B"};
inline ConfigVar<float> cfg_DirLightIntensity{"Lighting.Directional", "Intensity", 1.0f, "平行光の強さ"};
inline ConfigVar<float> cfg_PointLightConst{"Lighting.Point.Default", "Constant", 0.5f, "ポイントライト減衰定数項"};
inline ConfigVar<float> cfg_PointLightLinear{"Lighting.Point.Default", "Linear", 0.04f, "ポイントライト減衰一次項"};
inline ConfigVar<float> cfg_PointLightQuadratic{"Lighting.Point.Default", "Quadratic", 0.007f, "ポイントライト減衰二次項"};
inline ConfigVar<float> cfg_PointLightRange{"Lighting.Point.Default", "Range", 10.0f, "ポイントライトの有効距離"};
inline ConfigVar<float> cfg_PointLightYOffset{"Lighting.Point.Default", "YOffset", 0.5f, "ポイントライトのYオフセット"};
inline ConfigVar<float> cfg_BakeLightSpacing{"Lighting.Bake", "Spacing", 6.0f, "ライトベイク時の配置間隔"};
inline ConfigVar<float> cfg_BakeLightHeight{"Lighting.Bake", "Height", 2.5f, "ライトベイク時の高さ"};
inline ConfigVar<float> cfg_BakeLightIntensity{"Lighting.Bake", "Intensity", 0.7f, "ライトベイクの強さ"};
inline ConfigVar<float> cfg_BakeLightR{"Lighting.Bake", "ColorR", 0.9f, "ライトベイク用ライト色 R"};
inline ConfigVar<float> cfg_BakeLightG{"Lighting.Bake", "ColorG", 0.9f, "ライトベイク用ライト色 G"};
inline ConfigVar<float> cfg_BakeLightB{"Lighting.Bake", "ColorB", 0.9f, "ライトベイク用ライト色 B"};

inline ConfigVar<float> cfg_FloorWallR{"Stage.Wall.Visual", "FloorWallColorR", 0.5f, "床境界壁の色 R"};
inline ConfigVar<float> cfg_FloorWallG{"Stage.Wall.Visual", "FloorWallColorG", 0.5f, "床境界壁の色 G"};
inline ConfigVar<float> cfg_FloorWallB{"Stage.Wall.Visual", "FloorWallColorB", 0.5f, "床境界壁の色 B"};
inline ConfigVar<float> cfg_WallSize{"Stage.Wall.Geometry", "WallSize", 1.0f, "壁ブロックのサイズ"};


// FBXパス設定
inline ConfigVar<std::string> cfg_PlayerFBXPass{"Player.Asset", "PlayerFBXPass", "Assets/Models/Player/obj_player.fbx", "プレイヤーモデル FBX パス"};
inline ConfigVar<std::string> cfg_FloorFBXPass{"Stage.Floor.Asset", "FloorFBXPass", "Assets/Models/StageObj/Ground/floor_cube.fbx", "床モデル FBX パス"};
inline ConfigVar<std::string> cfg_WallFBXPass{"Stage.Wall.Asset", "WallFBXPass", "Assets/Models/StageObj/Wall/obj_wall.fbx", "壁モデル FBX パス"};
inline ConfigVar<std::string> cfg_HalfWallFBXPass{"Stage.Wall.Asset", "HalfWallFBXPass", "Assets/Models/StageObj/Wall/obj_wall_half.fbx", "半分の高さの壁 FBX パス"};
inline ConfigVar<std::string> cfg_StartFBXPass{"Stage.Start.Asset", "StartFBXPass", "Assets/Models/StageObj/Start/obj_start.fbx", "スタートマーカー FBX パス"};
inline ConfigVar<std::string> cfg_GoalFBXPass{"Stage.Goal.Asset", "GoalFBXPass", "Assets/Models/StageObj/StageGoal/obj_stagegoal.fbx", "ゴールマーカー FBX パス"};
inline ConfigVar<std::string> cfg_MovingObstacleFBXPass{"Stage.MovingObstacle.Asset", "MovingObstacleFBXPass", "Assets/Models/StageObj/Move/vehicle.fbx", "動く障害物 FBX パス"};
inline ConfigVar<std::string> cfg_DashBoardFBXPass{"Stage.DashBoard.Asset", "DashBoardFBXPass", "Assets/Models/StageObj/SpeedUp/obj_speedup2.fbx", "加速板 FBX パス"};
inline ConfigVar<std::string> cfg_AObstacleFBXPass{"Stage.PC.Asset","AObstacle","Assets/Models/StageObj/PC/PC.fbx", "PC 障害物 FBX パス"};
inline ConfigVar<std::string> cfg_BObstacleFBXPass{"Stage.GasPipe.Asset", "BObstacleFBXPass", "Assets/Models/StageObj/GasPipe/obj_obstacleB.fbx", "ガスパイプ障害物 FBX パス"};
inline ConfigVar<std::string> cfg_CObstacleFBXPass{"Stage.Cube.Asset", "CObstacleFBXPass", "Assets/Models/StageObj/Cub/Cube.fbx", "キューブ障害物 FBX パス"};
inline ConfigVar<std::string> cfg_WallLightFBXPass{"Stage.Light.Asset", "WallLightFBXPass", "Assets/Models/StageObj/Light/obj_light.fbx", "蛍光灯 FBX パス"};

inline ConfigVar<std::string> cfg_WindowFBXPass{"Title.Window.Asset", "WindowsPass", "Assets/Models/StageObj/Window/window.fbx", "窓付きの壁FBX パス"};


// UI設定（ゲーム中 HUD）
inline ConfigVar<float> cfg_UICountPosX{"UI.StageHUD.Counter", "CountPosX", 20.0f, "ステージHUDカウンタのX座標"};
inline ConfigVar<float> cfg_UICountPosY{"UI.StageHUD.Counter", "CountPosY", 170.0f, "ステージHUDカウンタのY座標"};
inline ConfigVar<float> cfg_UICountW{"UI.StageHUD.Counter", "CountWidth", 200.0f, "ステージHUDカウンタの幅"};
inline ConfigVar<float> cfg_UICountH{"UI.StageHUD.Counter", "CountHeight", 40.0f, "ステージHUDカウンタの高さ"};
inline ConfigVar<float> cfg_UICountR{"UI.StageHUD.Counter", "CountColorR", 0.0f, "ステージHUDカウンタの色 R"};
inline ConfigVar<float> cfg_UICountG{"UI.StageHUD.Counter", "CountColorG", 1.0f, "ステージHUDカウンタの色 G"};
inline ConfigVar<float> cfg_UICountB{"UI.StageHUD.Counter", "CountColorB", 1.0f, "ステージHUDカウンタの色 B"};

inline ConfigVar<std::string> cfg_RoomPath{"UI.StageHUD.Asset", "RoomPNGPass", "Assets/Textures/Count.png", "部屋番号表示用テクスチャパス"};

// 衝突設定
inline ConfigVar<float> cfg_CollisionCellSize{"Stage.Collision", "CollisionCellSize", 20.0f, "ステージ衝突グリッドのセルサイズ"};

// ゲームルール設定
inline ConfigVar<float> cfg_LimitTime{"Stage.Rule", "LimitTime", 10.0f, "1ルームあたりの制限時間（秒）"};

// スタート演出用エミッシブ
inline ConfigVar<float> cfg_StartEmissiveR{"Stage.Start.Emissive", "StartEmissiveR", 0.0f, "スタートパネルのエミッシブ色 R"};
inline ConfigVar<float> cfg_StartEmissiveG{"Stage.Start.Emissive", "StartEmissiveG", 0.5f, "スタートパネルのエミッシブ色 G"};
inline ConfigVar<float> cfg_StartEmissiveB{"Stage.Start.Emissive", "StartEmissiveB", 1.0f, "スタートパネルのエミッシブ色 B"};
inline ConfigVar<float> cfg_StartEmissiveIntensity{"Stage.Start.Emissive", "StartEmissiveIntensity", 1.5f, "スタートパネルのエミッシブ強さ"};
inline ConfigVar<float> cfg_StartPulseMin{"Stage.Start.Emissive", "StartPulseMin", 4.0f, "スタートパネル発光の最小周期"};
inline ConfigVar<float> cfg_StartPulseMax{"Stage.Start.Emissive", "StartPulseMax", 10.0f, "スタートパネル発光の最大周期"};
inline ConfigVar<float> cfg_StartPulseSpeed{"Stage.Start.Emissive", "StartPulseSpeed", 1.0f, "スタートパネル発光の速さ"};

// ゴール演出用エミッシブ
inline ConfigVar<float> cfg_GoalEmissiveR{"Stage.Goal.Emissive", "GoalEmissiveR", 1.000000f, "ゴールオブジェクトのエミッシブ色 R"};
inline ConfigVar<float> cfg_GoalEmissiveG{"Stage.Goal.Emissive", "GoalEmissiveG", 0.000000f, "ゴールオブジェクトのエミッシブ色 G"};
inline ConfigVar<float> cfg_GoalEmissiveB{"Stage.Goal.Emissive", "GoalEmissiveB", 0.000000f, "ゴールオブジェクトのエミッシブ色 B"};
inline ConfigVar<float> cfg_GoalEmissiveIntensity{"Stage.Goal.Emissive", "GoalEmissiveIntensity", 1.000000f, "ゴールオブジェクトのエミッシブ強さ"};
inline ConfigVar<float> cfg_GoalPulseMin{"Stage.Goal.Emissive", "GoalPulseMin", 4.0f, "ゴール発光の最小周期"};
inline ConfigVar<float> cfg_GoalPulseMax{"Stage.Goal.Emissive", "GoalPulseMax", 10.0f, "ゴール発光の最大周期"};
inline ConfigVar<float> cfg_GoalPulseSpeed{"Stage.Goal.Emissive", "GoalPulseSpeed", 1.0f, "ゴール発光の速さ"};

// ライト距離設定
inline ConfigVar<float> cfg_StartLightRange{"Stage.Start.Light", "StartLightRange", 5.0f, "スタート地点のライト照射距離"};
inline ConfigVar<float> cfg_GoalLightRange{"Stage.Goal.Light", "GoalLightRange", 8.0f, "ゴール地点のライト照射距離"};

//サウンド設定
inline ConfigVar<std::string> cfg_ColdMP3Pass{"Stage.Cold.SE", "ColdMP3Pass","Assets/Sound/DemoSE/cold.mp3", "はじき時の風音"};
inline ConfigVar<std::string> cfg_CollideMP3Pass{"Stage.Collide.SE", "CollideMP3Pass","Assets/Sound/DemoSE/collide.mp3", "衝突時の音"};
inline ConfigVar<std::string> cfg_DeathMP3Pass{"Stage.Death.SE", "DeathMP3Pass","Assets/Sound/DemoSE/death.mp3", "死亡時の音"};
inline ConfigVar<std::string> cfg_DriftMP3Pass{"Stage.Drift.SE", "DriftMP3Pass","Assets/Sound/DemoSE/drift_7.mp3", "ドリフト音"};
inline ConfigVar<std::string> cfg_Fire1MP3Pass{"Stage.Fire1.SE", "Fire1MP3Pass","Assets/Sound/DemoSE/fire1.mp3", "ジェットパック段階音１"};
inline ConfigVar<std::string> cfg_Fire2MP3Pass{"Stage.Fire2.SE", "Fire2MP3Pass","Assets/Sound/DemoSE/fire2.mp3", "ジェットパック段階音２"};
inline ConfigVar<std::string> cfg_Fire3MP3Pass{"Stage.Fire3.SE", "Fire3MP3Pass","Assets/Sound/DemoSE/fire3.mp3", "ジェットパック段階音３"};
inline ConfigVar<std::string> cfg_SirenMP3Pass{"Stage.Siren.SE", "SirenMP3Pass","Assets/Sound/DemoSE/siren.mp3", "警報音"};
inline ConfigVar<std::string> cfg_StartMP3Pass{"Stage.Start.SE", "StartMP3Pass","Assets/Sound/DemoSE/start.mp3", "スタート時の音"};
inline ConfigVar<std::string> cfg_SpeedUpMP3Pass{"Stage.SpeedUp.SE", "SpeedUpMP3Pass","Assets/Sound/DemoSE/speedup.mp3", "加速板の音"};
inline ConfigVar<std::string> cfg_WarpDownMP3Pass{"Stage.WarpDown.SE", "WarpDownMP3Pass","Assets/Sound/DemoSE/warpdown.mp3", "ワープから出る音"};
inline ConfigVar<std::string> cfg_WarpUpMP3Pass{"Stage.WarpUp.SE", "WarpUpMP3Pass","Assets/Sound/DemoSE/warpup.mp3", "ワープに入ったときの音"};
inline ConfigVar<std::string> cfg_EnterMP3Pass{"Stage.Enter.SE", "EnterMP3Pass","Assets/Sound/DemoSE/enter.mp3", "決定ボタンを押した音"};
inline ConfigVar<std::string> cfg_SelectMP3Pass{"Stage.Select.SE", "SelectMP3Pass","Assets/Sound/DemoSE/select.mp3", "選択を移動させた音"};
inline ConfigVar<std::string> cfg_KeyMP3Pass{"Stage.Key.SE", "KeyMP3Pass","Assets/Sound/DemoSE/key.mp3", "鍵が開いた音"};
inline ConfigVar<std::string> cfg_ClearMP3Pass{"Stage.Clear.BGM", "ClearMP3Pass","Assets/Sound/DemoBGM/clearBGM20260119.mp3", "クリア時のBGM"};
inline ConfigVar<std::string> cfg_TitleMP3Pass{"Stage.Title.BGM", "TitleMP3Pass","Assets/Sound/DemoBGM/titleBGM.mp3", "タイトル画面のBGM"};
inline ConfigVar<std::string> cfg_GameMP3Pass{"Stage.Game.BGM", "GameMP3Pass","Assets/Sound/DemoBGM/escape20260119.mp3", "ゲーム画面のBGM"};
inline ConfigVar<float> cfg_MasterVolume{"Stage.Master.Volume", "MasterVolume", 0.3f, "マスタの大きさ"};
inline ConfigVar<float> cfg_BGMVolume{"Stage.BGM.Volume", "BGMVolume", 1.0f, "BGMの大きさ"};
inline ConfigVar<float> cfg_SEVolume{"Stage.SE.Volume", "SEVolume", 0.6f, "SEの大きさ"};
// プレイヤー移動設定
inline static ConfigVar<float> cfg_AccelerateAccfication{"Player.Movement", "AccelerateAccfication", 1.5f, "加速版接触時の加速倍率"};
