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

/** @brief チャージ解放時シェイクの基本強度 */
inline ConfigVar<float> cfg_ChargeReleaseShakeBaseIntensity{"Camera.Reaction.ChargeRelease", "ShakeBaseIntensity", 0.03f, "チャージ解放時シェイクの基本強度"};
/** @brief チャージ量に応じて加算するシェイク強度 */
inline ConfigVar<float> cfg_ChargeReleaseShakeChargeScale{"Camera.Reaction.ChargeRelease", "ShakeChargeScale", 0.12f, "チャージ量に応じて加算するシェイク強度"};
/** @brief チャージ解放時シェイクの継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseShakeDuration{"Camera.Reaction.ChargeRelease", "ShakeDuration", 0.25f, "チャージ解放時シェイクの継続時間"};
/** @brief チャージ解放時に色収差を有効化するフラグ */
inline ConfigVar<bool> cfg_ChargeReleaseChromaticEnabled{"Camera.Reaction.ChargeRelease", "ChromaticEnabled", true, "チャージ解放時に色収差を有効化する"};
/** @brief チャージ解放時色収差の基本強度 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticBaseIntensity{"Camera.Reaction.ChargeRelease", "ChromaticBaseIntensity", 0.08f, "チャージ解放時色収差の基本強度"};
/** @brief チャージ量に応じて加算する色収差強度 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticChargeScale{"Camera.Reaction.ChargeRelease", "ChromaticChargeScale", 0.20f, "チャージ量に応じて加算する色収差強度"};
/** @brief 色収差強度の上限 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticMaxIntensity{"Camera.Reaction.ChargeRelease", "ChromaticMaxIntensity", 0.30f, "色収差強度の上限"};
/** @brief 色収差の継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticDuration{"Camera.Reaction.ChargeRelease", "ChromaticDuration", 0.16f, "色収差の継続時間"};
/** @brief 色収差のUVオフセット量 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticSampleOffset{"Camera.Reaction.ChargeRelease", "ChromaticSampleOffset", 0.006f, "色収差のUVオフセット量"};
/** @brief 画面中心から離れるほど強める倍率 */
inline ConfigVar<float> cfg_ChargeReleaseChromaticRadialScale{"Camera.Reaction.ChargeRelease", "ChromaticRadialScale", 1.5f, "画面中心から離れるほど強める倍率"};

/** @brief ブースト開始時演出の有効フラグ */
inline ConfigVar<bool> cfg_BoostBurstEnabled{"Camera.Reaction.BoostBurst", "Enabled", true, "ブースト開始時の演出を有効化する"};
/** @brief ブースト開始時シェイク強度 */
inline ConfigVar<float> cfg_BoostBurstShakeIntensity{"Camera.Reaction.BoostBurst", "ShakeIntensity", 0.018f, "ブースト開始時シェイク強度"};
/** @brief ブースト開始時シェイク継続時間 */
inline ConfigVar<float> cfg_BoostBurstShakeDuration{"Camera.Reaction.BoostBurst", "ShakeDuration", 0.12f, "ブースト開始時シェイク継続時間"};
/** @brief ブースト開始時色収差強度 */
inline ConfigVar<float> cfg_BoostBurstChromaticIntensity{"Camera.Reaction.BoostBurst", "ChromaticIntensity", 0.10f, "ブースト開始時色収差強度"};
/** @brief ブースト開始時色収差継続時間 */
inline ConfigVar<float> cfg_BoostBurstChromaticDuration{"Camera.Reaction.BoostBurst", "ChromaticDuration", 0.12f, "ブースト開始時色収差継続時間"};
/** @brief ブースト開始時色収差UVオフセット量 */
inline ConfigVar<float> cfg_BoostBurstChromaticSampleOffset{"Camera.Reaction.BoostBurst", "ChromaticSampleOffset", 0.0055f, "ブースト開始時色収差UVオフセット量"};
/** @brief ブースト開始時色収差の半径倍率 */
inline ConfigVar<float> cfg_BoostBurstChromaticRadialScale{"Camera.Reaction.BoostBurst", "ChromaticRadialScale", 1.2f, "ブースト開始時色収差の半径倍率"};
/** @brief ブースト速度に応じた演出強度の増幅率 */
inline ConfigVar<float> cfg_BoostBurstSpeedInfluence{"Camera.Reaction.BoostBurst", "SpeedInfluence", 0.5f, "ブースト速度に応じた演出強度の増幅率"};

/** @brief 残り時間切迫時演出の有効フラグ */
inline ConfigVar<bool> cfg_UrgencyFxEnabled{"Camera.Reaction.Urgency", "Enabled", true, "残り時間切迫時の演出を有効化する"};
/** @brief 切迫演出を開始する残り時間比率 */
inline ConfigVar<float> cfg_UrgencyFxThreshold{"Camera.Reaction.Urgency", "Threshold", 0.35f, "切迫演出を開始する残り時間比率"};
/** @brief 切迫演出の最短パルス間隔 */
inline ConfigVar<float> cfg_UrgencyFxPulseMinInterval{"Camera.Reaction.Urgency", "PulseMinInterval", 0.22f, "切迫演出の最短パルス間隔"};
/** @brief 切迫演出の最長パルス間隔 */
inline ConfigVar<float> cfg_UrgencyFxPulseMaxInterval{"Camera.Reaction.Urgency", "PulseMaxInterval", 0.75f, "切迫演出の最長パルス間隔"};
/** @brief 切迫演出シェイク最小強度 */
inline ConfigVar<float> cfg_UrgencyFxShakeMinIntensity{"Camera.Reaction.Urgency", "ShakeMinIntensity", 0.010f, "切迫演出シェイク最小強度"};
/** @brief 切迫演出シェイク最大強度 */
inline ConfigVar<float> cfg_UrgencyFxShakeMaxIntensity{"Camera.Reaction.Urgency", "ShakeMaxIntensity", 0.030f, "切迫演出シェイク最大強度"};
/** @brief 切迫演出シェイク継続時間 */
inline ConfigVar<float> cfg_UrgencyFxShakeDuration{"Camera.Reaction.Urgency", "ShakeDuration", 0.10f, "切迫演出シェイク継続時間"};
/** @brief 切迫演出色収差最小強度 */
inline ConfigVar<float> cfg_UrgencyFxChromaticMinIntensity{"Camera.Reaction.Urgency", "ChromaticMinIntensity", 0.030f, "切迫演出色収差最小強度"};
/** @brief 切迫演出色収差最大強度 */
inline ConfigVar<float> cfg_UrgencyFxChromaticMaxIntensity{"Camera.Reaction.Urgency", "ChromaticMaxIntensity", 0.140f, "切迫演出色収差最大強度"};
/** @brief 切迫演出色収差継続時間 */
inline ConfigVar<float> cfg_UrgencyFxChromaticDuration{"Camera.Reaction.Urgency", "ChromaticDuration", 0.10f, "切迫演出色収差継続時間"};
/** @brief 切迫演出色収差UVオフセット量 */
inline ConfigVar<float> cfg_UrgencyFxChromaticSampleOffset{"Camera.Reaction.Urgency", "ChromaticSampleOffset", 0.004f, "切迫演出色収差UVオフセット量"};
/** @brief 切迫演出色収差の半径倍率 */
inline ConfigVar<float> cfg_UrgencyFxChromaticRadialScale{"Camera.Reaction.Urgency", "ChromaticRadialScale", 1.3f, "切迫演出色収差の半径倍率"};
