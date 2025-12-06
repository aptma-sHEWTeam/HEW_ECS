/**
 * @file CameraReaction.h
 * @brief カメラリアクション（シェイク、インパルス、ズーム等）の定義
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "config/ConfigVar.h"

/**
 * @brief カメラリアクションタイプ
 */
enum class CameraReactionType {
    None,       ///< リアクションなし
    Shake,      ///< 揺れ（ランダム）
    Impulse,    ///< 衝撃（一方向へ跳ねる）
    Zoom,       ///< ズームイン/アウト
    Tilt        ///< 傾き
};

// =========================================
// カメラリアクション用ConfigVars
// =========================================
/** @brief カメラシェイク時のX軸方向の揺れの周波数 */
inline ConfigVar<float> cfg_CameraShakeFreqX{"Camera", "ShakeFreqX", 35.0f};
/** @brief カメラシェイク時のY軸方向の揺れの周波数 */
inline ConfigVar<float> cfg_CameraShakeFreqY{"Camera", "ShakeFreqY", 28.0f};
/** @brief カメラシェイク時のZ軸方向の揺れの周波数 */
inline ConfigVar<float> cfg_CameraShakeFreqZ{"Camera", "ShakeFreqZ", 41.0f};
/** @brief カメラシェイク効果の減衰率。大きいほど早く収まる */
inline ConfigVar<float> cfg_CameraShakeDecay{"Camera", "ShakeDecay", 3.0f};
/** @brief カメラシェイクのランダム性の強さ。0で完全なsin波、1で完全なランダム */
inline ConfigVar<float> cfg_CameraShakeRandomness{"Camera", "ShakeRandomness", 0.3f};
/** @brief カメラシェイク時のY軸方向の揺れのスケール（他軸との相対） */
inline ConfigVar<float> cfg_CameraShakeYScale{"Camera", "ShakeYScale", 0.5f};
/** @brief カメラの衝撃（インパルス）効果の減衰率。大きいほど早く収まる */
inline ConfigVar<float> cfg_CameraImpulseDecay{"Camera", "ImpulseDecay", 5.0f};
/** @brief カメラのズームイン/アウト アニメーションの速度 */
inline ConfigVar<float> cfg_CameraZoomSpeed{"Camera", "ZoomSpeed", 2.0f};
/** @brief カメラがプレイヤーを追従する際の滑らかさ。大きいほど素早く追従する */
inline ConfigVar<float> cfg_CameraFollowSmooth{"Camera", "FollowSmooth", 5.0f};

// =========================================
// 壁衝突リスポーン用ConfigVars
// =========================================
/** @brief 壁衝突時のカメラシェイクの強さ */
inline ConfigVar<float> cfg_WallHitShakeIntensity{"Game", "WallHitShakeIntensity", 0.5f};
/** @brief 壁衝突時のカメラシェイクの持続時間 */
inline ConfigVar<float> cfg_WallHitShakeDuration{"Game", "WallHitShakeDuration", 0.3f};
/** @brief 壁衝突後、プレイヤーがリスポーンするまでの遅延時間 */
inline ConfigVar<float> cfg_WallHitRespawnDelay{"Game", "WallHitRespawnDelay", 0.4f};
