#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "components/ModelComponent.h"
#include "graphics/ModelLoader.h"

/**
 * @file ResourceManager.h
 * @brief 3Dモデルなどのリソースを管理（キャッシュ）するクラス
 * @author 山内陽
 * @date 2025
 * @version 6.0
 */

class ResourceManager {
public:
    // モデルをファイルパスで取得（キャッシュ対応）
    std::vector<ModelComponent> GetModel(const std::string& filePath);

    // キャッシュをクリア
    void Clear();

private:
    // モデルキャッシュ
    std::unordered_map<std::string, std::vector<ModelComponent>> modelCache_;
};
