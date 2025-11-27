#pragma once

/**
 * @file BuildConfig.h
 * @brief ビルド時に有効/無効を切り替える調整用マクロ群
 *
 * DebugFast 構成などで高コストなデバッグレイヤーを簡単に外せるよう、
 * デフォルト値をここで一元管理する。
 */

#ifndef ENABLE_GFX_DEBUG_LAYER
#if defined(_DEBUG)
#define ENABLE_GFX_DEBUG_LAYER 1
#else
#define ENABLE_GFX_DEBUG_LAYER 0
#endif
#endif

#ifndef ENABLE_SHADER_DEBUG
#if defined(_DEBUG)
#define ENABLE_SHADER_DEBUG 1
#else
#define ENABLE_SHADER_DEBUG 0
#endif
#endif

#ifndef ENABLE_VERBOSE_INPUT_LOG
#define ENABLE_VERBOSE_INPUT_LOG 0
#endif

#ifndef ENABLE_ECS_TRACE_LOG
#define ENABLE_ECS_TRACE_LOG 0
#endif

#ifndef ENABLE_DEBUG_VISUALS
#define ENABLE_DEBUG_VISUALS 0
#endif

#ifndef ENABLE_METRICS_LOG
#if defined(_DEBUG)
#define ENABLE_METRICS_LOG 1
#else
#define ENABLE_METRICS_LOG 0
#endif
#endif

#ifndef ENABLE_FRAME_PACING
#define ENABLE_FRAME_PACING 1
#endif

#ifndef FRAME_PACING_TARGET_FPS
#define FRAME_PACING_TARGET_FPS 60.0f
#endif

#ifndef FRAME_PACING_SPIN_THRESHOLD_MS
#define FRAME_PACING_SPIN_THRESHOLD_MS 1.0f
#endif

#ifndef METRICS_LOG_INTERVAL_FRAMES
#define METRICS_LOG_INTERVAL_FRAMES 60
#endif

#ifndef DEBUGLOG_AUTO_FLUSH_THRESHOLD
#define DEBUGLOG_AUTO_FLUSH_THRESHOLD 512
#endif // DEBUGLOG_AUTO_FLUSH_THRESHOLD
