#pragma once

#include "components/Component.h"

/**
 * @brief タグコンポーネント
 * @details エンティティの種類を識別するためのマーカーです。
 * データは持たず、エンティティが特定の種類であることを示すために使用します。
 */
struct PlayerTag : IComponent {};  ///< プレイヤータグ
struct EnemyTag : IComponent {};   ///< 敵タグ
struct BulletTag : IComponent {};  ///< 弾丸タグ
