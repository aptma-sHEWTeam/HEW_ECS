#pragma once
#include <DirectXMath.h>
#include "components/ModelComponent.h"

/**
 * @brief FBX/Assimp読み込み時のノード情報（メッシュとローカルTRS、親インデックス）
 */
struct ModelPrefabNode {
    ModelComponent component;                    ///< メッシュデータ（hasMesh == true のとき有効）
    DirectX::XMFLOAT3 translation{0, 0, 0};      ///< ローカル平行移動
    DirectX::XMFLOAT3 rotationDeg{0, 0, 0};      ///< ローカル回転（度数法）
    DirectX::XMFLOAT3 scale{1, 1, 1};            ///< ローカルスケール
    int parentIndex = -1;                        ///< 親ノードのインデックス（-1 ならルート）
    bool hasMesh = false;                        ///< メッシュを保持するノードか
};
