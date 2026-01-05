#pragma once

#include "components/Component.h"

/**
 * @brief タグコンポーネント
 * @details エンティティの種類を識別するためのマーカーです。
 * データは持たず、エンティティが特定の種類であることを示すために使用します。
 */
struct PlayerTag : IComponent {};		///< プレイヤータグ
struct EnemyTag : IComponent {};		///< 敵タグ
struct BulletTag : IComponent {};		///< 弾丸タグ
struct WallTag : IComponent {};			///< 壁タグ
struct GimmickTag : IComponent {};		///< ギミックタグ
struct FloorWallTag : IComponent {};	///< ステージ共通の壁タグ
struct SwitchTag : IComponent {};		///< スイッチタグ
