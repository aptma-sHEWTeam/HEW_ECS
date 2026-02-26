/**
 * @file CameraReaction.h
 * @brief カメラリアクション（シェイク、インパルス、ズーム等）の定義
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "config/ConfigVar.h"
#include <string>

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
/** @brief 壁衝突時色収差強度 */
inline ConfigVar<float> cfg_WallHitChromaticIntensity{"Camera.Reaction.WallHit", "WallHitChromaticIntensity", 0.130f, "壁衝突時色収差強度"};
/** @brief 壁衝突時色収差継続時間 */
inline ConfigVar<float> cfg_WallHitChromaticDuration{"Camera.Reaction.WallHit", "WallHitChromaticDuration", 0.120f, "壁衝突時色収差継続時間"};
/** @brief 壁衝突時色収差UVオフセット量 */
inline ConfigVar<float> cfg_WallHitChromaticSampleOffset{"Camera.Reaction.WallHit", "WallHitChromaticSampleOffset", 0.0052f, "壁衝突時色収差UVオフセット量"};
/** @brief 壁衝突時色収差の半径倍率 */
inline ConfigVar<float> cfg_WallHitChromaticRadialScale{"Camera.Reaction.WallHit", "WallHitChromaticRadialScale", 1.25f, "壁衝突時色収差の半径倍率"};
/** @brief 壁衝突時インパルス強度 */
inline ConfigVar<float> cfg_WallHitImpulseIntensity{"Camera.Reaction.WallHit", "WallHitImpulseIntensity", 0.260f, "壁衝突時インパルス強度"};
/** @brief 壁衝突時インパルス継続時間 */
inline ConfigVar<float> cfg_WallHitImpulseDuration{"Camera.Reaction.WallHit", "WallHitImpulseDuration", 0.070f, "壁衝突時インパルス継続時間"};
/** @brief 壁衝突時振動強度 */
inline ConfigVar<float> cfg_WallHitRumbleStrength{"Camera.Reaction.WallHit", "WallHitRumbleStrength", 0.850f, "壁衝突時振動強度"};
/** @brief 壁衝突時振動継続時間 */
inline ConfigVar<float> cfg_WallHitRumbleDuration{"Camera.Reaction.WallHit", "WallHitRumbleDuration", 0.280f, "壁衝突時振動継続時間"};

/** @brief タイムアップ時シェイク強度 */
inline ConfigVar<float> cfg_TimeUpShakeIntensity{"Camera.Reaction.TimeUp", "ShakeIntensity", 0.420f, "タイムアップ時シェイク強度"};
/** @brief タイムアップ時シェイク継続時間 */
inline ConfigVar<float> cfg_TimeUpShakeDuration{"Camera.Reaction.TimeUp", "ShakeDuration", 0.320f, "タイムアップ時シェイク継続時間"};
/** @brief タイムアップ時色収差強度 */
inline ConfigVar<float> cfg_TimeUpChromaticIntensity{"Camera.Reaction.TimeUp", "ChromaticIntensity", 0.180f, "タイムアップ時色収差強度"};
/** @brief タイムアップ時色収差継続時間 */
inline ConfigVar<float> cfg_TimeUpChromaticDuration{"Camera.Reaction.TimeUp", "ChromaticDuration", 0.180f, "タイムアップ時色収差継続時間"};
/** @brief タイムアップ時色収差UVオフセット量 */
inline ConfigVar<float> cfg_TimeUpChromaticSampleOffset{"Camera.Reaction.TimeUp", "ChromaticSampleOffset", 0.0060f, "タイムアップ時色収差UVオフセット量"};
/** @brief タイムアップ時色収差の半径倍率 */
inline ConfigVar<float> cfg_TimeUpChromaticRadialScale{"Camera.Reaction.TimeUp", "ChromaticRadialScale", 1.35f, "タイムアップ時色収差の半径倍率"};
/** @brief タイムアップ時インパルス強度 */
inline ConfigVar<float> cfg_TimeUpImpulseIntensity{"Camera.Reaction.TimeUp", "ImpulseIntensity", 0.180f, "タイムアップ時インパルス強度"};
/** @brief タイムアップ時インパルス継続時間 */
inline ConfigVar<float> cfg_TimeUpImpulseDuration{"Camera.Reaction.TimeUp", "ImpulseDuration", 0.100f, "タイムアップ時インパルス継続時間"};
/** @brief タイムアップ時振動強度 */
inline ConfigVar<float> cfg_TimeUpRumbleStrength{"Camera.Reaction.TimeUp", "RumbleStrength", 0.700f, "タイムアップ時振動強度"};
/** @brief タイムアップ時振動継続時間 */
inline ConfigVar<float> cfg_TimeUpRumbleDuration{"Camera.Reaction.TimeUp", "RumbleDuration", 0.320f, "タイムアップ時振動継続時間"};

/** @brief チャージ解放時シェイクの基本強度 */
inline ConfigVar<float> cfg_ChargeReleaseShakeBaseIntensity{"Camera.Reaction.ChargeRelease", "ShakeBaseIntensity", 0.015f, "チャージ解放時シェイクの基本強度"};
/** @brief チャージ量に応じて加算するシェイク強度 */
inline ConfigVar<float> cfg_ChargeReleaseShakeChargeScale{"Camera.Reaction.ChargeRelease", "ShakeChargeScale", 0.060f, "チャージ量に応じて加算するシェイク強度"};
/** @brief チャージ解放時シェイクの継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseShakeDuration{"Camera.Reaction.ChargeRelease", "ShakeDuration", 0.25f, "チャージ解放時シェイクの継続時間"};
/** @brief チャージ解放時ズームの基本量 */
inline ConfigVar<float> cfg_ChargeReleaseZoomBaseAmount{"Camera.Reaction.ChargeRelease", "ZoomBaseAmount", 0.020f, "チャージ解放時ズームの基本量"};
/** @brief チャージ量に応じて加算するズーム量 */
inline ConfigVar<float> cfg_ChargeReleaseZoomChargeScale{"Camera.Reaction.ChargeRelease", "ZoomChargeScale", 0.080f, "チャージ量に応じて加算するズーム量"};
/** @brief チャージ解放時ズーム量の上限 */
inline ConfigVar<float> cfg_ChargeReleaseZoomMaxAmount{"Camera.Reaction.ChargeRelease", "ZoomMaxAmount", 0.120f, "チャージ解放時ズーム量の上限"};
/** @brief チャージ解放時ズーム継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseZoomDuration{"Camera.Reaction.ChargeRelease", "ZoomDuration", 0.140f, "チャージ解放時ズーム継続時間"};
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
/** @brief チャージ中の持続色収差を有効化するフラグ */
inline ConfigVar<bool> cfg_ChargeHoldChromaticEnabled{"Camera.Reaction.ChargeHold", "ChromaticEnabled", true, "チャージ中の持続色収差を有効化する"};
/** @brief チャージ中の持続色収差強度 */
inline ConfigVar<float> cfg_ChargeHoldChromaticIntensity{"Camera.Reaction.ChargeHold", "ChromaticIntensity", 0.06f, "チャージ中の持続色収差強度"};
/** @brief チャージ中の持続色収差UVオフセット量 */
inline ConfigVar<float> cfg_ChargeHoldChromaticSampleOffset{"Camera.Reaction.ChargeHold", "ChromaticSampleOffset", 0.004f, "チャージ中の持続色収差UVオフセット量"};
/** @brief チャージ中の持続色収差の半径倍率 */
inline ConfigVar<float> cfg_ChargeHoldChromaticRadialScale{"Camera.Reaction.ChargeHold", "ChromaticRadialScale", 1.2f, "チャージ中の持続色収差の半径倍率"};
/** @brief チャージ中の持続色収差を維持する更新間隔(秒) */
inline ConfigVar<float> cfg_ChargeHoldChromaticRefreshDuration{"Camera.Reaction.ChargeHold", "ChromaticRefreshDuration", 0.08f, "チャージ中の持続色収差を維持する更新間隔(秒)"};
/** @brief チャージ解放時インパルスの基本強度 */
inline ConfigVar<float> cfg_ChargeReleaseImpulseBaseIntensity{"Camera.Reaction.ChargeRelease", "ImpulseBaseIntensity", 0.060f, "チャージ解放時インパルスの基本強度"};
/** @brief チャージ量に応じて加算するインパルス強度 */
inline ConfigVar<float> cfg_ChargeReleaseImpulseChargeScale{"Camera.Reaction.ChargeRelease", "ImpulseChargeScale", 0.140f, "チャージ量に応じて加算するインパルス強度"};
/** @brief チャージ解放時インパルス継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseImpulseDuration{"Camera.Reaction.ChargeRelease", "ImpulseDuration", 0.090f, "チャージ解放時インパルス継続時間"};
/** @brief チャージ解放時の振動最小強度 */
inline ConfigVar<float> cfg_ChargeReleaseRumbleMin{"Camera.Reaction.ChargeRelease", "RumbleMin", 0.250f, "チャージ解放時の振動最小強度"};
/** @brief チャージ解放時の振動最大強度 */
inline ConfigVar<float> cfg_ChargeReleaseRumbleMax{"Camera.Reaction.ChargeRelease", "RumbleMax", 0.950f, "チャージ解放時の振動最大強度"};
/** @brief チャージ解放時の振動継続時間 */
inline ConfigVar<float> cfg_ChargeReleaseRumbleDuration{"Camera.Reaction.ChargeRelease", "RumbleDuration", 0.220f, "チャージ解放時の振動継続時間"};

/** @brief ブースト開始時演出の有効フラグ */
inline ConfigVar<bool> cfg_BoostBurstEnabled{"Camera.Reaction.BoostBurst", "Enabled", true, "ブースト開始時の演出を有効化する"};
/** @brief ブースト開始時シェイク強度 */
inline ConfigVar<float> cfg_BoostBurstShakeIntensity{"Camera.Reaction.BoostBurst", "ShakeIntensity", 0.010f, "ブースト開始時シェイク強度"};
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

/** @brief スタートイントロ演出の有効フラグ */
inline ConfigVar<bool> cfg_StartIntroEnabled{"Camera.Reaction.StartIntro", "Enabled", true, "スタートイントロ演出を有効化する"};
/** @brief スタートイントロ演出の継続時間 */
inline ConfigVar<float> cfg_StartIntroDuration{"Camera.Reaction.StartIntro", "Duration", 1.2f, "スタートイントロ演出の継続時間"};
/** @brief スタートイントロ時のカメラXオフセット */
inline ConfigVar<float> cfg_StartIntroOffsetX{"Camera.Reaction.StartIntro", "OffsetX", 0.0f, "スタートイントロ時のカメラXオフセット"};
/** @brief スタートイントロ時のカメラYオフセット */
inline ConfigVar<float> cfg_StartIntroOffsetY{"Camera.Reaction.StartIntro", "OffsetY", 6.0f, "スタートイントロ時のカメラYオフセット"};
/** @brief スタートイントロ時のカメラZオフセット */
inline ConfigVar<float> cfg_StartIntroOffsetZ{"Camera.Reaction.StartIntro", "OffsetZ", -2.2f, "スタートイントロ時のカメラZオフセット"};
/** @brief スタートイントロ時の注視点Y補正 */
inline ConfigVar<float> cfg_StartIntroLookAtYOffset{"Camera.Reaction.StartIntro", "LookAtYOffset", 1.0f, "スタートイントロ時の注視点Y補正"};
/** @brief スタートイントロ開始時のFOV(度) */
inline ConfigVar<float> cfg_StartIntroFovDegrees{"Camera.Reaction.StartIntro", "IntroFovDegrees", 38.0f, "スタートイントロ開始時のFOV(度)"};
/** @brief リスポーン時にスタートイントロを再生するか */
inline ConfigVar<bool> cfg_StartIntroPlayOnRespawn{"Camera.Reaction.StartIntro", "PlayOnRespawn", false, "リスポーン時にスタートイントロを再生する"};

/** @brief 衝突ズーム演出の有効フラグ */
inline ConfigVar<bool> cfg_CollisionZoomEnabled{"Camera.Reaction.CollisionZoom", "Enabled", true, "衝突ズーム演出を有効化する"};
/** @brief 壁衝突時ズーム量 */
inline ConfigVar<float> cfg_WallHitZoomAmount{"Camera.Reaction.CollisionZoom", "WallHitZoomAmount", 0.10f, "壁衝突時ズーム量"};
/** @brief 壁衝突時ズーム継続時間 */
inline ConfigVar<float> cfg_WallHitZoomDuration{"Camera.Reaction.CollisionZoom", "WallHitZoomDuration", 0.16f, "壁衝突時ズーム継続時間"};
/** @brief タイムアップ時ズーム量 */
inline ConfigVar<float> cfg_TimeUpZoomAmount{"Camera.Reaction.CollisionZoom", "TimeUpZoomAmount", 0.14f, "タイムアップ時ズーム量"};
/** @brief タイムアップ時ズーム継続時間 */
inline ConfigVar<float> cfg_TimeUpZoomDuration{"Camera.Reaction.CollisionZoom", "TimeUpZoomDuration", 0.20f, "タイムアップ時ズーム継続時間"};
/** @brief 同時ズーム時の最大ズーム量 */
inline ConfigVar<float> cfg_CollisionMaxConcurrentZoom{"Camera.Reaction.CollisionZoom", "MaxConcurrentZoom", 0.22f, "同時ズーム時の最大ズーム量"};
/** @brief 衝突ズーム中のプレイヤー中心フレーミング有効フラグ */
inline ConfigVar<bool> cfg_CollisionFocusEnabled{"Camera.Reaction.CollisionZoom", "FocusEnabled", true, "衝突ズーム中のプレイヤー中心フレーミングを有効化する"};
/** @brief 衝突ズーム中のフレーミング補間速度 */
inline ConfigVar<float> cfg_CollisionFocusBlendSpeed{"Camera.Reaction.CollisionZoom", "FocusBlendSpeed", 7.0f, "衝突ズーム中のフレーミング補間速度"};
/** @brief 衝突ズーム中のプレイヤー注視Yオフセット */
inline ConfigVar<float> cfg_CollisionFocusLookAtYOffset{"Camera.Reaction.CollisionZoom", "FocusLookAtYOffset", 1.0f, "衝突ズーム中のプレイヤー注視Yオフセット"};
/** @brief 衝突ズーム中のカメラ距離縮小率 */
inline ConfigVar<float> cfg_CollisionFocusDistanceScale{"Camera.Reaction.CollisionZoom", "FocusDistanceScale", 0.80f, "衝突ズーム中のカメラ距離縮小率"};
/** @brief 衝突ズーム中の最大フレーミング寄り率 */
inline ConfigVar<float> cfg_CollisionFocusMaxBlend{"Camera.Reaction.CollisionZoom", "FocusMaxBlend", 0.95f, "衝突ズーム中の最大フレーミング寄り率"};

/** @brief ゴール吸引開始時演出の有効フラグ */
inline ConfigVar<bool> cfg_GoalInFxEnabled{"Camera.Reaction.GoalIn", "Enabled", true, "ゴール吸引開始時演出を有効化する"};
/** @brief ゴール吸引開始時ズーム量 */
inline ConfigVar<float> cfg_GoalInZoomAmount{"Camera.Reaction.GoalIn", "ZoomAmount", 0.06f, "ゴール吸引開始時ズーム量"};
/** @brief ゴール吸引開始時ズーム継続時間 */
inline ConfigVar<float> cfg_GoalInZoomDuration{"Camera.Reaction.GoalIn", "ZoomDuration", 0.32f, "ゴール吸引開始時ズーム継続時間"};
/** @brief ゴール吸引開始時色収差強度 */
inline ConfigVar<float> cfg_GoalInChromaticIntensity{"Camera.Reaction.GoalIn", "ChromaticIntensity", 0.09f, "ゴール吸引開始時色収差強度"};
/** @brief ゴール吸引開始時色収差継続時間 */
inline ConfigVar<float> cfg_GoalInChromaticDuration{"Camera.Reaction.GoalIn", "ChromaticDuration", 0.16f, "ゴール吸引開始時色収差継続時間"};
/** @brief ゴール吸引開始時色収差UVオフセット量 */
inline ConfigVar<float> cfg_GoalInChromaticSampleOffset{"Camera.Reaction.GoalIn", "ChromaticSampleOffset", 0.0048f, "ゴール吸引開始時色収差UVオフセット量"};
/** @brief ゴール吸引開始時色収差の半径倍率 */
inline ConfigVar<float> cfg_GoalInChromaticRadialScale{"Camera.Reaction.GoalIn", "ChromaticRadialScale", 1.25f, "ゴール吸引開始時色収差の半径倍率"};
/** @brief ゴール吸引開始時振動強度 */
inline ConfigVar<float> cfg_GoalInRumbleStrength{"Camera.Reaction.GoalIn", "RumbleStrength", 0.350f, "ゴール吸引開始時振動強度"};
/** @brief ゴール吸引開始時振動継続時間 */
inline ConfigVar<float> cfg_GoalInRumbleDuration{"Camera.Reaction.GoalIn", "RumbleDuration", 0.240f, "ゴール吸引開始時振動継続時間"};
/** @brief ゴール吸引中にプレイヤーへ寄るカメラ演出を有効化する */
inline ConfigVar<bool> cfg_GoalInPlayerCameraEnabled{"Camera.Reaction.GoalIn", "PlayerCameraEnabled", true, "ゴール吸引中にプレイヤーへ寄るカメラ演出を有効化する"};
/** @brief ゴール吸引中カメラのXオフセット */
inline ConfigVar<float> cfg_GoalInPlayerCameraOffsetX{"Camera.Reaction.GoalIn", "PlayerCameraOffsetX", 0.0f, "ゴール吸引中カメラのXオフセット"};
/** @brief ゴール吸引中カメラのYオフセット */
inline ConfigVar<float> cfg_GoalInPlayerCameraOffsetY{"Camera.Reaction.GoalIn", "PlayerCameraOffsetY", 4.8f, "ゴール吸引中カメラのYオフセット"};
/** @brief ゴール吸引中カメラのZオフセット */
inline ConfigVar<float> cfg_GoalInPlayerCameraOffsetZ{"Camera.Reaction.GoalIn", "PlayerCameraOffsetZ", -1.8f, "ゴール吸引中カメラのZオフセット"};
/** @brief ゴール吸引中カメラの注視点Y補正 */
inline ConfigVar<float> cfg_GoalInPlayerCameraLookAtYOffset{"Camera.Reaction.GoalIn", "PlayerCameraLookAtYOffset", 1.0f, "ゴール吸引中カメラの注視点Y補正"};
/** @brief ゴール吸引中カメラFOV(度) */
inline ConfigVar<float> cfg_GoalInPlayerCameraFovDegrees{"Camera.Reaction.GoalIn", "PlayerCameraFovDegrees", 20.0f, "ゴール吸引中カメラFOV(度)"};
/** @brief ゴール吸引中カメラの補間速度 */
inline ConfigVar<float> cfg_GoalInPlayerCameraBlendSpeed{"Camera.Reaction.GoalIn", "PlayerCameraBlendSpeed", 4.5f, "ゴール吸引中カメラの補間速度"};

/** @brief スクリーンFXディレクター有効フラグ */
inline ConfigVar<bool> cfg_ScreenFxDirectorEnabled{"Camera.Reaction.ScreenFXDirector", "Enabled", true, "スクリーンFXディレクターを有効化する"};
/** @brief スクリーンFXプリセット名 */
inline ConfigVar<std::string> cfg_ScreenFxPreset{"Camera.Reaction.ScreenFXDirector", "Preset", "Default", "スクリーンFXプリセット(Competitive/Stream/Default/Cinematic)"};
/** @brief 低FPS判定開始値 */
inline ConfigVar<float> cfg_ScreenFxLowFpsStart{"Camera.Reaction.ScreenFXDirector", "LowFpsStart", 50.0f, "低FPS判定開始値"};
/** @brief 低FPS時の最低演出係数 */
inline ConfigVar<float> cfg_ScreenFxLowFpsMinScale{"Camera.Reaction.ScreenFXDirector", "LowFpsMinScale", 0.45f, "低FPS時の最低演出係数"};
/** @brief UI/フェード中の演出係数 */
inline ConfigVar<float> cfg_ScreenFxUiScale{"Camera.Reaction.ScreenFXDirector", "UiScale", 0.70f, "UI/フェード中の演出係数"};
/** @brief 演出ごとの最大シェイク量 */
inline ConfigVar<float> cfg_ScreenFxMaxShake{"Camera.Reaction.ScreenFXDirector", "MaxShake", 0.08f, "演出ごとの最大シェイク量"};
/** @brief 演出ごとの最大ズーム量 */
inline ConfigVar<float> cfg_ScreenFxMaxZoom{"Camera.Reaction.ScreenFXDirector", "MaxZoom", 0.22f, "演出ごとの最大ズーム量"};
/** @brief 演出ごとの最大色収差量 */
inline ConfigVar<float> cfg_ScreenFxMaxChromatic{"Camera.Reaction.ScreenFXDirector", "MaxChromatic", 0.35f, "演出ごとの最大色収差量"};
/** @brief ブーストバーストのクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownBoostBurst{"Camera.Reaction.ScreenFXDirector", "CooldownBoostBurst", 0.08f, "ブーストバーストのクールダウン"};
/** @brief チャージ解放のクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownChargeRelease{"Camera.Reaction.ScreenFXDirector", "CooldownChargeRelease", 0.06f, "チャージ解放のクールダウン"};
/** @brief 切迫パルスのクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownUrgency{"Camera.Reaction.ScreenFXDirector", "CooldownUrgency", 0.10f, "切迫パルスのクールダウン"};
/** @brief 壁衝突のクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownWallHit{"Camera.Reaction.ScreenFXDirector", "CooldownWallHit", 0.08f, "壁衝突のクールダウン"};
/** @brief タイムアップのクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownTimeUp{"Camera.Reaction.ScreenFXDirector", "CooldownTimeUp", 0.10f, "タイムアップのクールダウン"};
/** @brief ゴール吸引開始のクールダウン */
inline ConfigVar<float> cfg_ScreenFxCooldownGoalIn{"Camera.Reaction.ScreenFXDirector", "CooldownGoalIn", 0.10f, "ゴール吸引開始のクールダウン"};

/** @brief 競技/配信用プリセットの演出係数 */
inline ConfigVar<float> cfg_ScenicPresetCompetitiveScale{"Camera.Reaction.ScenicPreset", "CompetitiveScale", 0.55f, "Competitiveプリセット時の演出係数"};
/** @brief 配信用プリセットの演出係数 */
inline ConfigVar<float> cfg_ScenicPresetStreamScale{"Camera.Reaction.ScenicPreset", "StreamScale", 0.65f, "Streamプリセット時の演出係数"};
/** @brief シネマティックプリセットの演出係数 */
inline ConfigVar<float> cfg_ScenicPresetCinematicScale{"Camera.Reaction.ScenicPreset", "CinematicScale", 1.10f, "Cinematicプリセット時の演出係数"};

/** @brief 映像強化レイヤー全体の有効フラグ */
inline ConfigVar<bool> cfg_ScenicLayerEnabled{"Camera.Reaction.Scenic", "Enabled", true, "常時/状態/瞬間レイヤー演出を有効化する"};
/** @brief 画面中心保護半径（0.2前後で中央40%を保護） */
inline ConfigVar<float> cfg_ScenicCenterSafeRadius{"Camera.Reaction.Scenic", "CenterSafeRadius", 0.20f, "中心視野を保護するための半径"};
/** @brief 入力中は外周ノイズ等を抑制する */
inline ConfigVar<bool> cfg_ScenicSuppressOnInput{"Camera.Reaction.Scenic", "SuppressOnInput", true, "入力中の外周演出を抑制してUI干渉を避ける"};
/** @brief 入力アクティブ判定しきい値 */
inline ConfigVar<float> cfg_ScenicInputActiveThreshold{"Camera.Reaction.Scenic", "InputActiveThreshold", 0.15f, "入力アクティブ判定に使うスティックしきい値"};

/** @brief 常時レイヤー有効フラグ */
inline ConfigVar<bool> cfg_ScenicAmbientEnabled{"Camera.Reaction.ScenicAmbient", "Enabled", true, "常時レイヤー演出を有効化する"};
/** @brief 遠景パーティクル有効フラグ */
inline ConfigVar<bool> cfg_ScenicFarParticlesEnabled{"Camera.Reaction.ScenicAmbient", "FarParticlesEnabled", true, "遠景パーティクルを有効化する"};
/** @brief 遠景パーティクルのスケール */
inline ConfigVar<float> cfg_ScenicFarParticlesScale{"Camera.Reaction.ScenicAmbient", "FarParticlesScale", 5.0f, "遠景パーティクルのスケール"};
/** @brief 遠景パーティクルの高さオフセット */
inline ConfigVar<float> cfg_ScenicFarParticlesHeight{"Camera.Reaction.ScenicAmbient", "FarParticlesHeight", 8.0f, "遠景パーティクルの高さオフセット"};
/** @brief 壁ライト脈動の有効フラグ */
inline ConfigVar<bool> cfg_ScenicWallPulseEnabled{"Camera.Reaction.ScenicAmbient", "WallPulseEnabled", true, "壁/床演出として弱い発光脈動を有効化する"};
/** @brief 壁ライト脈動の最小強度 */
inline ConfigVar<float> cfg_ScenicWallPulseMin{"Camera.Reaction.ScenicAmbient", "WallPulseMin", 0.88f, "壁ライト脈動の最小強度"};
/** @brief 壁ライト脈動の最大強度 */
inline ConfigVar<float> cfg_ScenicWallPulseMax{"Camera.Reaction.ScenicAmbient", "WallPulseMax", 1.05f, "壁ライト脈動の最大強度"};
/** @brief 壁ライト脈動の速度 */
inline ConfigVar<float> cfg_ScenicWallPulseSpeed{"Camera.Reaction.ScenicAmbient", "WallPulseSpeed", 0.9f, "壁ライト脈動の周期速度"};
/** @brief 常時フォグ強度 */
inline ConfigVar<float> cfg_ScenicAmbientFogIntensity{"Camera.Reaction.ScenicAmbient", "FogIntensity", 0.08f, "空気感フォグの基礎強度"};
/** @brief 外周ノイズ強度 */
inline ConfigVar<float> cfg_ScenicAmbientNoiseIntensity{"Camera.Reaction.ScenicAmbient", "NoiseIntensity", 0.08f, "UI外周ノイズの強度"};

/** @brief 状態レイヤー有効フラグ */
inline ConfigVar<bool> cfg_ScenicStateEnabled{"Camera.Reaction.ScenicState", "Enabled", true, "残り時間/ブースト/ゴール連動演出を有効化する"};
/** @brief 外周ビネットの基礎強度 */
inline ConfigVar<float> cfg_ScenicStateVignetteBase{"Camera.Reaction.ScenicState", "VignetteBase", 0.06f, "外周ビネットの基礎強度"};
/** @brief 切迫時に追加する外周ビネット強度 */
inline ConfigVar<float> cfg_ScenicStateVignetteUrgencyScale{"Camera.Reaction.ScenicState", "VignetteUrgencyScale", 0.16f, "残り時間切迫時に追加するビネット強度"};
/** @brief 切迫時の暖色寄り量 */
inline ConfigVar<float> cfg_ScenicStateWarmByUrgency{"Camera.Reaction.ScenicState", "WarmByUrgency", 0.45f, "時間切迫時の暖色寄り係数"};
/** @brief ブースト時の寒色寄り量 */
inline ConfigVar<float> cfg_ScenicStateCoolByBoost{"Camera.Reaction.ScenicState", "CoolByBoost", 0.32f, "ブースト時の寒色寄り係数"};
/** @brief ゴール吸引時の寒色寄り量 */
inline ConfigVar<float> cfg_ScenicStateCoolByGoal{"Camera.Reaction.ScenicState", "CoolByGoal", 0.22f, "ゴール吸引時の寒色寄り係数"};
/** @brief 速度線の最大強度 */
inline ConfigVar<float> cfg_ScenicStateSpeedLineMax{"Camera.Reaction.ScenicState", "SpeedLineMax", 0.40f, "状態レイヤー速度線の最大強度"};
/** @brief 速度線のブースト寄与率 */
inline ConfigVar<float> cfg_ScenicStateSpeedLineBoostScale{"Camera.Reaction.ScenicState", "SpeedLineBoostScale", 0.70f, "速度線へのブースト寄与率"};
/** @brief 速度線の切迫寄与率 */
inline ConfigVar<float> cfg_ScenicStateSpeedLineUrgencyScale{"Camera.Reaction.ScenicState", "SpeedLineUrgencyScale", 0.30f, "速度線への切迫寄与率"};
/** @brief 床面ライン流速の最小スケール */
inline ConfigVar<float> cfg_ScenicFloorFlowMinScale{"Camera.Reaction.ScenicState", "FloorFlowMinScale", 0.90f, "床面ライン流速の最小スケール"};
/** @brief 床面ライン流速の最大スケール */
inline ConfigVar<float> cfg_ScenicFloorFlowMaxScale{"Camera.Reaction.ScenicState", "FloorFlowMaxScale", 1.45f, "床面ライン流速の最大スケール"};
/** @brief 状態レイヤーの色収差有効フラグ */
inline ConfigVar<bool> cfg_ScenicStateChromaticEnabled{"Camera.Reaction.ScenicState", "ChromaticEnabled", true, "状態レイヤーの短時間色収差を有効化する"};
/** @brief 状態レイヤー色収差の基礎強度 */
inline ConfigVar<float> cfg_ScenicStateChromaticIntensity{"Camera.Reaction.ScenicState", "ChromaticIntensity", 0.05f, "状態レイヤー色収差の基礎強度"};
/** @brief 状態レイヤー色収差の継続時間 */
inline ConfigVar<float> cfg_ScenicStateChromaticDuration{"Camera.Reaction.ScenicState", "ChromaticDuration", 0.09f, "状態レイヤー色収差の継続時間"};
/** @brief 状態レイヤー色収差UVオフセット量 */
inline ConfigVar<float> cfg_ScenicStateChromaticSampleOffset{"Camera.Reaction.ScenicState", "ChromaticSampleOffset", 0.0035f, "状態レイヤー色収差のUVオフセット量"};
/** @brief 状態レイヤー色収差半径倍率 */
inline ConfigVar<float> cfg_ScenicStateChromaticRadialScale{"Camera.Reaction.ScenicState", "ChromaticRadialScale", 1.15f, "状態レイヤー色収差の半径倍率"};
/** @brief 状態レイヤー色収差の更新間隔 */
inline ConfigVar<float> cfg_ScenicStateChromaticInterval{"Camera.Reaction.ScenicState", "ChromaticInterval", 0.18f, "状態レイヤー色収差の更新間隔"};

/** @brief 瞬間レイヤー有効フラグ */
inline ConfigVar<bool> cfg_ScenicInstantEnabled{"Camera.Reaction.ScenicInstant", "Enabled", true, "瞬間レイヤー演出を有効化する"};
/** @brief マイクロフラッシュ有効フラグ */
inline ConfigVar<bool> cfg_ScenicMicroFlashEnabled{"Camera.Reaction.ScenicInstant", "MicroFlashEnabled", true, "マイクロフラッシュを有効化する"};
/** @brief ブースト時マイクロフラッシュ強度 */
inline ConfigVar<float> cfg_ScenicMicroFlashBoostIntensity{"Camera.Reaction.ScenicInstant", "MicroFlashBoostIntensity", 0.10f, "加速時マイクロフラッシュ強度"};
/** @brief 着地時マイクロフラッシュ強度 */
inline ConfigVar<float> cfg_ScenicMicroFlashLandingIntensity{"Camera.Reaction.ScenicInstant", "MicroFlashLandingIntensity", 0.07f, "着地時マイクロフラッシュ強度"};
/** @brief 衝突時マイクロフラッシュ強度 */
inline ConfigVar<float> cfg_ScenicMicroFlashCollisionIntensity{"Camera.Reaction.ScenicInstant", "MicroFlashCollisionIntensity", 0.13f, "衝突時マイクロフラッシュ強度"};
/** @brief ブースト時マイクロフラッシュ継続 */
inline ConfigVar<float> cfg_ScenicMicroFlashBoostDuration{"Camera.Reaction.ScenicInstant", "MicroFlashBoostDuration", 0.10f, "加速時マイクロフラッシュ継続時間"};
/** @brief 着地時マイクロフラッシュ継続 */
inline ConfigVar<float> cfg_ScenicMicroFlashLandingDuration{"Camera.Reaction.ScenicInstant", "MicroFlashLandingDuration", 0.10f, "着地時マイクロフラッシュ継続時間"};
/** @brief 衝突時マイクロフラッシュ継続 */
inline ConfigVar<float> cfg_ScenicMicroFlashCollisionDuration{"Camera.Reaction.ScenicInstant", "MicroFlashCollisionDuration", 0.16f, "衝突時マイクロフラッシュ継続時間"};
/** @brief マイクロフラッシュ最短間隔 */
inline ConfigVar<float> cfg_ScenicMicroFlashMinInterval{"Camera.Reaction.ScenicInstant", "MicroFlashMinInterval", 0.16f, "明滅を抑えるための最短間隔"};
/** @brief 速度線バースト有効フラグ */
inline ConfigVar<bool> cfg_ScenicSpeedLineBurstEnabled{"Camera.Reaction.ScenicInstant", "SpeedLineBurstEnabled", true, "進行方向速度線バーストを有効化する"};
/** @brief 速度線バースト強度 */
inline ConfigVar<float> cfg_ScenicSpeedLineBurstIntensity{"Camera.Reaction.ScenicInstant", "SpeedLineBurstIntensity", 0.28f, "進行方向速度線バースト強度"};
/** @brief 速度線バースト継続 */
inline ConfigVar<float> cfg_ScenicSpeedLineBurstDuration{"Camera.Reaction.ScenicInstant", "SpeedLineBurstDuration", 0.14f, "進行方向速度線バースト継続時間"};
/** @brief SE同期リップル有効フラグ */
inline ConfigVar<bool> cfg_ScenicSERippleEnabled{"Camera.Reaction.ScenicInstant", "SERippleEnabled", true, "SE同期の軽い発光リップルを有効化する"};
/** @brief SE同期リップル強度 */
inline ConfigVar<float> cfg_ScenicSERippleIntensity{"Camera.Reaction.ScenicInstant", "SERippleIntensity", 0.18f, "SE同期発光リップル強度"};
/** @brief SE同期リップル継続 */
inline ConfigVar<float> cfg_ScenicSERippleDuration{"Camera.Reaction.ScenicInstant", "SERippleDuration", 0.16f, "SE同期発光リップル継続時間"};

/** @brief 色収差の最大継続時間（短時間制限） */
inline ConfigVar<float> cfg_ScenicChromaticMaxDuration{"Camera.Reaction.ScenicGuard", "ChromaticMaxDuration", 0.20f, "色収差継続時間の上限"};
/** @brief シェイク上限（小振幅固定） */
inline ConfigVar<float> cfg_ScenicGuardMaxShake{"Camera.Reaction.ScenicGuard", "MaxShake", 0.05f, "シェイクを小振幅に制限する上限値"};
