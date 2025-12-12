#include "app/ResourceManager.h"
#include "app/DebugLog.h"

const std::vector<ModelPrefabNode>& ResourceManager::GetModel(const std::string& filePath) {
    static const std::vector<ModelPrefabNode> kEmpty;

    auto it = modelCache_.find(filePath);
    if (it != modelCache_.end()) {
        return it->second;
    }

    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Model cache miss, loading: " + filePath);
    std::vector<ModelPrefabNode> loadedModel = ModelLoader::LoadModel(filePath);

    if (loadedModel.empty()) {
        return kEmpty;
    }

    auto result = modelCache_.emplace(filePath, std::move(loadedModel));
    return result.first->second;
}

void ResourceManager::Clear() {
    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Clearing all cached resources.");
    modelCache_.clear();
}

