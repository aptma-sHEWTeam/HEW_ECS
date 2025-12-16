#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "components/ModelPrefab.h"
#include "graphics/GfxDevice.h"
#include "graphics/TextureManager.h"
#include "app/DebugLog.h"

class ModelLoader {
public:
    // FBX/OBJなどをロードし、ノード階層を保持したままメッシュとローカルTRSを返す
    static std::vector<ModelPrefabNode> LoadModel(const std::string& filePath);

    // アニメーションのみをロードする
    static std::vector<ModelComponent::AnimationClip> LoadAnimation(const std::string& filePath);

private:
    static void ProcessNode(
        const aiNode* node,
        const DirectX::XMMATRIX& parentTransform,
        int parentIndex,
        const aiScene* scene,
        const std::string& directory,
        const std::string& modelFilePath,
        std::vector<ModelPrefabNode>& outNodes,
        GfxDevice& gfx);

    static ModelComponent ProcessMesh(
        aiMesh* mesh,
        const DirectX::XMMATRIX& nodeGlobalTransform,
        const aiScene* scene,
        const std::string& directory,
        const std::string& modelFilePath,
        GfxDevice& gfx);

    static TextureManager::TextureHandle LoadMaterialTextures(
        aiMaterial* mat,
        aiTextureType type,
        const std::string& directory,
        const std::string& modelFilePath
    );
};
