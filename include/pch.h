/**
 * @file pch.h
 * @brief プリコンパイル済みヘッダー
 * @author 山内陽
 * @date 2025
 * @version 1.0
 *
 * @details
 * よく使うヘッダーをまとめたプリコンパイル済みヘッダーです。
 * このファイルをインクルードするだけで、基本的なコンポーネントや
 * システムが使えるようになります。
 */
#pragma once

// ========================================================
// 標準ライブラリ
// ========================================================
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>
#include <cstdint>

// ========================================================
// DirectX
// ========================================================
#include <DirectXMath.h>

// ========================================================
// ECS コア
// ========================================================
#include "ecs/Entity.h"
#include "ecs/World.h"

// ========================================================
// 基本コンポーネント
// ========================================================
#include "components/Component.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/Model.h"
#include "scenes/Tags.h"
#include "components/Collision.h"

// ========================================================
// システム
// ========================================================
#include "input/InputSystem.h"
#include "graphics/Camera.h"
#include "graphics/GfxDevice.h"
#include "graphics/TextureManager.h"
#include "graphics/RenderSystem.h"

// ========================================================
// シーン管理
// ========================================================
#include "scenes/SceneManager.h"

// ========================================================
// アニメーション（オプション）
// ========================================================
#include "animation/Animation.h"
