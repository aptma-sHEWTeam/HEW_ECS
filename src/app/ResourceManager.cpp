#include "app/ResourceManager.h"
#include "app/DebugLog.h"

#include <algorithm>

static std::string NormalizeAssetPath(std::string path) {
    if (path.size() >= 2 && path[0] == '.' && (path[1] == '/' || path[1] == '\\')) {
        path.erase(0, 2);
    }
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

const std::vector<ModelPrefabNode>& ResourceManager::GetModel(const std::string& filePath) {
    static const std::vector<ModelPrefabNode> kEmpty;

    const std::string normalizedPath = NormalizeAssetPath(filePath);

    auto it = modelCache_.find(normalizedPath);
    if (it != modelCache_.end()) {
        return it->second;
    }

    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Model cache miss, loading: " + normalizedPath);
    std::vector<ModelPrefabNode> loadedModel = ModelLoader::LoadModel(normalizedPath);

    if (loadedModel.empty()) {
        return kEmpty;
    }

    auto result = modelCache_.emplace(normalizedPath, std::move(loadedModel));
    return result.first->second;
}

void ResourceManager::Clear() {
    DEBUGLOG_CATEGORY(DebugLog::Category::Graphics, "Clearing all cached resources.");
    modelCache_.clear();
}

