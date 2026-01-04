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
inline ConfigVar<float> cfg_CameraShakeFreqX{"Camera.Reaction.Shake", "ShakeFreqX", 35.0f, "カメラシェイクのX軸方向の周波数"};
/** @brief カメラシェイク時のY軸方向の揺れの周波数 */
inline ConfigVar<float> cfg_CameraShakeFreqY{"Camera.Reaction.Shake", "ShakeFreqY", 28.0f, "カメラシェイクのY軸方向の周波数"};
/** @brief カメラシェイク時のZ軸方向の揺れの周波数 */
inline ConfigVar<float> cfg_CameraShakeFreqZ{"Camera.Reaction.Shake", "ShakeFreqZ", 41.0f, "カメラシェイクのZ軸方向の周波数"};
/** @brief カメラシェイク効果の減衰率。大きいほど早く収まる */
inline ConfigVar<float> cfg_CameraShakeDecay{"Camera.Reaction.Shake", "ShakeDecay", 3.0f, "カメラシェイクの減衰速度"};
/** @brief カメラシェイクのランダム性の強さ。0で完全なsin波、1で完全なランダム */
inline ConfigVar<float> cfg_CameraShakeRandomness{"Camera.Reaction.Shake", "ShakeRandomness", 0.3f, "カメラシェイクのランダム成分の強さ"};
/** @brief カメラシェイク時のY軸方向の揺れのスケール（他軸との相対） */
inline ConfigVar<float> cfg_CameraShakeYScale{"Camera.Reaction.Shake", "ShakeYScale", 0.5f, "カメラシェイクのY軸方向の揺れ量係数"};
/** @brief カメラの衝撃（インパルス）効果の減衰率。大きいほど早く収まる */
inline ConfigVar<float> cfg_CameraImpulseDecay{"Camera.Reaction.Impulse", "ImpulseDecay", 5.0f, "カメラインパルスの減衰速度"};
/** @brief カメラのズームイン/アウト アニメーションの速度 */
inline ConfigVar<float> cfg_CameraZoomSpeed{"Camera.Reaction.Zoom", "ZoomSpeed", 2.0f, "カメラズームの補間速度"};
/** @brief カメラがプレイヤーを追従する際の滑らかさ。大きいほど素早く追従する */
inline ConfigVar<float> cfg_CameraFollowSmooth{"Camera.Reaction.Follow", "FollowSmooth", 5.0f, "カメラの追従スムーズさ"};

// =========================================
// 壁衝突リスポーン用ConfigVars
// =========================================
/** @brief 壁衝突時のカメラシェイクの強さ */
inline ConfigVar<float> cfg_WallHitShakeIntensity{"Camera.Reaction.WallHit", "WallHitShakeIntensity", 0.5f, "壁に衝突したときのカメラシェイク強度"};
/** @brief 壁衝突時のカメラシェイクの持続時間 */
inline ConfigVar<float> cfg_WallHitShakeDuration{"Camera.Reaction.WallHit", "WallHitShakeDuration", 0.3f, "壁に衝突したときのカメラシェイク継続時間"};
/** @brief 壁衝突後、プレイヤーがリスポーンするまでの遅延時間 */
inline ConfigVar<float> cfg_WallHitRespawnDelay{"Camera.Reaction.WallHit", "WallHitRespawnDelay", 1.0f, "壁衝突後にプレイヤーをリスポーンさせるまでの遅延時間"};
