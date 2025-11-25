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

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include <DirectXMath.h>

#include "ecs/Entity.h"
#include "ecs/World.h"
#include "components/Component.h"
#include "components/Transform.h"
#include "components/MeshRenderer.h"
#include "components/Collision.h"
#include "scenes/Tags.h"
#include "input/InputSystem.h"
#include "graphics/Camera.h"
#include "graphics/GfxDevice.h"
#include "graphics/RenderSystem.h"
#include "scenes/SceneManager.h"
