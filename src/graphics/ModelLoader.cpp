#include "graphics/ModelLoader.h"
#include "app/ServiceLocator.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXMath.h>

// 頂点構造体
struct SimpleVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 TexCoord;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT3 Bitangent;
};

namespace {
DirectX::XMFLOAT3 QuaternionToEulerDeg(const DirectX::XMVECTOR& q) {
    using namespace DirectX;
    XMFLOAT4 qf;
    XMStoreFloat4(&qf, q);

    float sinr_cosp = 2.0f * (qf.w * qf.x + qf.y * qf.z);
    float cosr_cosp = 1.0f - 2.0f * (qf.x * qf.x + qf.y * qf.y);
    float roll = atan2f(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qf.w * qf.y - qf.z * qf.x);
    float pitch = fabsf(sinp) >= 1.0f ? copysignf(XM_PIDIV2, sinp) : asinf(sinp);

    float siny_cosp = 2.0f * (qf.w * qf.z + qf.x * qf.y);
    float cosy_cosp = 1.0f - 2.0f * (qf.y * qf.y + qf.z * qf.z);
    float yaw = atan2f(siny_cosp, cosy_cosp);

    XMFLOAT3 eulerRad{ pitch, yaw, roll };
    return XMFLOAT3{
        XMConvertToDegrees(eulerRad.x),
        XMConvertToDegrees(eulerRad.y),
        XMConvertToDegrees(eulerRad.z) };
}
} // namespace

std::vector<ModelPrefabNode> ModelLoader::LoadModel(const std::string& filePath)
{
    auto& gfx = ServiceLocator::Get<GfxDevice>();
    std::vector<ModelPrefabNode> nodes;
    Assimp::Importer importer;

    // モデルをロード
    // aiProcess_Triangulate: 全てのプリミティブを三角形に変換
    // aiProcess_FlipUVs: UV座標を反転 (DirectXの慣例に合わせる)
    // aiProcess_GenNormals: 法線がなければ生成
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    // エラーチェック
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        DEBUGLOG_ERROR("Assimp Error: " + std::string(importer.GetErrorString()));
        return nodes;
    }

    // ファイルパスからディレクトリを抽出
    std::string directory = filePath.substr(0, filePath.find_last_of('/'));
    if (directory.empty()) {
        directory = filePath.substr(0, filePath.find_last_of('\\'));
    }

    // シーンのルートノードから再帰的に処理
    ProcessNode(scene->mRootNode, -1, scene, directory, nodes, gfx);

    DEBUGLOG_CATEGORY(DebugLog::Category::Render, "Model loaded: " + filePath + ", Nodes: " + std::to_string(nodes.size()));
    return nodes;
}

void ModelLoader::ProcessNode(
    const aiNode* node,
    int parentIndex,
    const aiScene* scene,
    const std::string& directory,
    std::vector<ModelPrefabNode>& outNodes,
    GfxDevice& gfx) {

    // ノードのローカルTRSを分解
    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D translation;
    node->mTransformation.Decompose(scaling, rotation, translation);

    ModelPrefabNode baseNode;
    baseNode.translation = { translation.x, translation.y, translation.z };
    DirectX::XMVECTOR rot = DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w);
    baseNode.rotationDeg = QuaternionToEulerDeg(rot);
    baseNode.scale = { scaling.x, scaling.y, scaling.z };
    baseNode.parentIndex = parentIndex;

    // このノードが保持するメッシュ（複数ある場合は1つ目をこのノードに、2つ目以降は子ノードとして複製）
    if (node->mNumMeshes > 0) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[0]];
        baseNode.component = ProcessMesh(mesh, scene, directory, gfx);
        baseNode.hasMesh = baseNode.component.indexCount > 0;
    }

    int currentIndex = static_cast<int>(outNodes.size());
    outNodes.push_back(baseNode);

    // 追加のメッシュを持つ場合は同一TRSの子ノードとして生成
    for (unsigned int i = 1; i < node->mNumMeshes; ++i) {
        ModelPrefabNode extra = baseNode;
        extra.parentIndex = currentIndex;
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        extra.component = ProcessMesh(mesh, scene, directory, gfx);
        extra.hasMesh = extra.component.indexCount > 0;
        outNodes.push_back(extra);
    }

    // 子ノードを再帰処理
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], currentIndex, scene, directory, outNodes, gfx);
    }
}

ModelComponent ModelLoader::ProcessMesh(
    aiMesh* mesh,
    const aiScene* scene,
    const std::string& directory,
    GfxDevice& gfx) {
    std::vector<SimpleVertex> vertices;
    std::vector<unsigned short> indices;

    // 頂点データを処理
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SimpleVertex vertex;
        vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        
        // テクスチャ座標が存在する場合
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        } else {
            vertex.TexCoord = { 0.0f, 0.0f };
        }

        if (mesh->mNormals) {
            vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        } else {
            vertex.Normal = { 0.0f, 1.0f, 0.0f }; // Default normal if not present
        }

        if (mesh->mTangents) {
            vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
        }

        if (mesh->mBitangents) {
            vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
        }
        vertices.push_back(vertex);
    }

    // インデックスデータを処理
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(static_cast<unsigned short>(face.mIndices[j]));
        }
    }

    // ModelComponentを作成
    ModelComponent mc{};
    mc.indexCount = static_cast<UINT>(indices.size());

    // 頂点バッファの作成
    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(SimpleVertex));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA vinit{ vertices.data(), 0, 0 };
    if (FAILED(gfx.Dev()->CreateBuffer(&vbd, &vinit, mc.vertexBuffer.GetAddressOf()))) {
        DEBUGLOG_ERROR("Failed to create vertex buffer for model.");
        mc.indexCount = 0;
        return mc;
    }

    // インデックスバッファの作成
    D3D11_BUFFER_DESC ibd{};
    ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(unsigned short));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA iinit{ indices.data(), 0, 0 };
    if (FAILED(gfx.Dev()->CreateBuffer(&ibd, &iinit, mc.indexBuffer.GetAddressOf()))) {
        DEBUGLOG_ERROR("Failed to create index buffer for model.");
        mc.indexCount = 0;
        return mc;
    }

    // マテリアルを処理
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // 現時点ではDiffuseテクスチャのみをロード
        mc.texture = LoadMaterialTextures(material, aiTextureType_DIFFUSE, directory);
        mc.normalTexture = LoadMaterialTextures(material, aiTextureType_NORMALS, directory);

        // マテリアルから色情報を取得 (Ambient/Diffuse/Specularなど、ここではDiffuseを代表として使用)
        aiColor3D color (0.f,0.f,0.f);
        if(AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            mc.color = {color.r, color.g, color.b};
        }
    }

    return mc;
}

TextureManager::TextureHandle ModelLoader::LoadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& directory
) {
    auto& texMgr = ServiceLocator::Get<TextureManager>();
    TextureManager::TextureHandle textureHandle = TextureManager::INVALID_TEXTURE;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string filename = str.C_Str();
        
        // テクスチャパスを構築 (モデルファイルと同じディレクトリを基準)
        std::string fullPath = directory + "/" + filename;
        
        // テクスチャマネージャーでロード
        textureHandle = texMgr.LoadFromFile(fullPath.c_str());
        if (textureHandle != TextureManager::INVALID_TEXTURE) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Render, "Loaded texture: " + fullPath);
            break; // 最初のテクスチャのみを使用
        } else {
            DEBUGLOG_WARNING("Failed to load texture: " + fullPath);
        }
    }
    return textureHandle;
}
