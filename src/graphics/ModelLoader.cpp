#include "graphics/ModelLoader.h"
#include "app/DebugLog.h"
#include "app/ServiceLocator.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXMath.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <map>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <sstream>

// 頂点構造体
struct SimpleVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 TexCoord;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT3 Bitangent;
    uint32_t BoneIndices[4] = {0, 0, 0, 0};
    float BoneWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
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

// Assimp のテクスチャパスを正規化して実際のファイルパスに解決する
static std::string ResolveTexturePath(const std::string& rawPath, const std::string& directory)
{
    if (rawPath.empty()) return {};

    std::string path = rawPath;

    // バックスラッシュをスラッシュに統一
    std::replace(path.begin(), path.end(), '\\', '/');

    // パス前後の空白を削る
    while (!path.empty() && std::isspace(static_cast<unsigned char>(path.front()))) path.erase(path.begin());
    while (!path.empty() && std::isspace(static_cast<unsigned char>(path.back()))) path.pop_back();

    // 絶対パス or ドライブレター付きならそのまま
    if (path.size() > 1 && path[1] == ':') {
        return path;
    }
    if (!path.empty() && (path[0] == '/' || path[0] == '\\')) {
        return path;
    }

    // カレントとモデルディレクトリの両方を試す
    std::filesystem::path rel = path;
    if (std::filesystem::exists(rel)) {
        return std::filesystem::canonical(rel).string();
    }

    std::filesystem::path dir = directory;
    std::filesystem::path underModel = dir / rel;
    if (std::filesystem::exists(underModel)) {
        return std::filesystem::canonical(underModel).string();
    }

    // モデルディレクトリ直下の Texture, Textures, textures などもよくある
    static const char* kTexDirs[] = { "", "Texture", "Textures", "texture", "textures", "Tex", "tex" };
    for (const char* sub : kTexDirs) {
        std::filesystem::path candidate = dir;
        if (sub && *sub) candidate /= sub;
        candidate /= rel.filename();
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::canonical(candidate).string();
        }
    }

    return {};
}

// FBXインポートで付与される "$AssimpFbx$_Translation" などのノード名を元のボーン名に戻す
static std::string SanitizeFbxChannelName(const std::string& rawName) {
    std::string name = rawName;
    const std::string marker = "_$AssimpFbx$_";
    size_t pos = name.find(marker);
    if (pos != std::string::npos) {
        name.erase(pos); // 以降を丸ごと削る（PreRotation, PostRotation, Translation等すべて切り捨て）
    }
    return name;
}

static ModelComponent::Keyframe& FindOrCreateKeyframe(std::vector<ModelComponent::Keyframe>& frames, float time) {
    constexpr float EPS = 1e-4f;
    for (auto& f : frames) {
        if (std::fabs(f.time - time) < EPS) {
            return f;
        }
    }

    ModelComponent::Keyframe kf{};
    kf.time = time;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    kf.position = {nan, nan, nan};
    kf.rotation = {nan, nan, nan, nan};
    kf.scale = {nan, nan, nan};
    frames.push_back(kf);
    return frames.back();
}

static std::string ToString(const aiMatrix4x4& m) {
    std::ostringstream oss;
    oss << "[" << m.a1 << "," << m.a2 << "," << m.a3 << "," << m.a4
        << " | " << m.b1 << "," << m.b2 << "," << m.b3 << "," << m.b4
        << " | " << m.c1 << "," << m.c2 << "," << m.c3 << "," << m.c4
        << " | " << m.d1 << "," << m.d2 << "," << m.d3 << "," << m.d4 << "]";
    return oss.str();
}

static std::string ToString(const DirectX::XMFLOAT4X4& m) {
    std::ostringstream oss;
    oss << "[" << m._11 << "," << m._12 << "," << m._13 << "," << m._14
        << " | " << m._21 << "," << m._22 << "," << m._23 << "," << m._24
        << " | " << m._31 << "," << m._32 << "," << m._33 << "," << m._34
        << " | " << m._41 << "," << m._42 << "," << m._43 << "," << m._44 << "]";
    return oss.str();
}
} // namespace

// ノードツリーから「ボーン名 -> 親ボーン名」を収集（サフィックス除去後）
static void CollectBoneParents(const aiNode* node,
                               const std::unordered_set<std::string>& boneNames,
                               std::unordered_map<std::string, std::string>& outParent) {
    if (!node) return;
    std::string name = SanitizeFbxChannelName(node->mName.C_Str());
    std::string parentName;
    if (node->mParent) {
        parentName = SanitizeFbxChannelName(node->mParent->mName.C_Str());
    }

    if (boneNames.count(name)) {
        if (!parentName.empty() && parentName != name) {
            outParent[name] = parentName;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        CollectBoneParents(node->mChildren[i], boneNames, outParent);
    }
}

// ノード名を収集してインデックス化（メッシュが無いFBXでも名前解決するため）
static void CollectNodeNames(const aiNode* node, std::map<std::string, int>& outIndexByName) {
    if (!node) return;
    std::string name = SanitizeFbxChannelName(node->mName.C_Str());
    if (!name.empty() && outIndexByName.find(name) == outIndexByName.end()) {
        outIndexByName[name] = static_cast<int>(outIndexByName.size());
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        CollectNodeNames(node->mChildren[i], outIndexByName);
    }
}

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
        std::string err = "Assimp Error: " + std::string(importer.GetErrorString()) + " | file=" + filePath;
        DEBUGLOG_ERROR(err);
#ifdef _DEBUG
        MessageBoxA(nullptr, err.c_str(), "Model Load Error", MB_OK | MB_ICONERROR);
#endif
        return nodes;
    }

    // ファイルパスからディレクトリを抽出
    std::string directory = filePath.substr(0, filePath.find_last_of('/'));
    if (directory.empty()) {
        directory = filePath.substr(0, filePath.find_last_of('\\'));
    }

    // シーンのルートノードから再帰的に処理
    ProcessNode(scene->mRootNode, -1, scene, directory, filePath, nodes, gfx);

    if (nodes.empty()) {
        std::string msg = "Model contains no renderable nodes: " + filePath;
        DEBUGLOG_WARNING(msg);
#ifdef _DEBUG
        MessageBoxA(nullptr, msg.c_str(), "Model Load Warning", MB_OK | MB_ICONWARNING);
#endif
    }

    DEBUGLOG_CATEGORY(DebugLog::Category::Render, "Model loaded: " + filePath + ", Nodes: " + std::to_string(nodes.size()));
    return nodes;
}

void ModelLoader::ProcessNode(
    const aiNode* node,
    int parentIndex,
    const aiScene* scene,
    const std::string& directory,
    const std::string& modelFilePath,
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
        baseNode.component = ProcessMesh(mesh, scene, directory, modelFilePath, gfx);
        baseNode.hasMesh = baseNode.component.indexCount > 0;
    }

    int currentIndex = static_cast<int>(outNodes.size());
    outNodes.push_back(baseNode);

    // 追加のメッシュを持つ場合は同一TRSの子ノードとして生成
    for (unsigned int i = 1; i < node->mNumMeshes; ++i) {
        ModelPrefabNode extra = baseNode;
        extra.parentIndex = currentIndex;
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        extra.component = ProcessMesh(mesh, scene, directory, modelFilePath, gfx);
        extra.hasMesh = extra.component.indexCount > 0;
        outNodes.push_back(extra);
    }

    // 子ノードを再帰処理
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], currentIndex, scene, directory, modelFilePath, outNodes, gfx);
    }
}

ModelComponent ModelLoader::ProcessMesh(
    aiMesh* mesh,
    const aiScene* scene,
    const std::string& directory,
    const std::string& modelFilePath,
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

    ModelComponent mc{};
    mc.indexCount = static_cast<UINT>(indices.size());

    // シーンルートのグローバル行列の逆行列を保存しておく（スキニング用）
    if (scene && scene->mRootNode) {
        aiMatrix4x4 rootGlobalRaw = scene->mRootNode->mTransformation;
        aiMatrix4x4 rootGlobal = rootGlobalRaw;
        rootGlobal.Transpose(); // Assimp列優先→DirectX行優先
        aiMatrix4x4 inv = rootGlobal;
        inv.Inverse();
        memcpy(&mc.globalInverse, &inv, sizeof(DirectX::XMFLOAT4X4));
#ifdef _DEBUG
        DEBUGLOG("ModelLoader: root transform (Assimp col-major) = " + ToString(rootGlobalRaw));
        DEBUGLOG("ModelLoader: root transform (DX row-major)    = " + ToString(rootGlobal));
        DEBUGLOG("ModelLoader: globalInverse (DX row-major)     = " + ToString(mc.globalInverse));
#endif
    }

    // マテリアルを処理
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // 現時点ではDiffuseテクスチャのみをロード
        mc.texture = LoadMaterialTextures(material, aiTextureType_DIFFUSE, directory, modelFilePath);
        mc.normalTexture = LoadMaterialTextures(material, aiTextureType_NORMALS, directory, modelFilePath);

        // マテリアルから色情報を取得 (Ambient/Diffuse/Specularなど、ここではDiffuseを代表として使用)
        aiColor3D color(0.f, 0.f, 0.f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            mc.color = {color.r, color.g, color.b};
        }
        mc.color.x = std::clamp(mc.color.x, 0.0f, 1.0f);
        mc.color.y = std::clamp(mc.color.y, 0.0f, 1.0f);
        mc.color.z = std::clamp(mc.color.z, 0.0f, 1.0f);

        // スペキュラー/反射系パラメータを可能な限り読み取る
        aiColor3D specColor(0.f, 0.f, 0.f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specColor)) {
            mc.specularColor = { specColor.r, specColor.g, specColor.b };
        }
        mc.specularColor.x = std::clamp(mc.specularColor.x, 0.0f, 1.0f);
        mc.specularColor.y = std::clamp(mc.specularColor.y, 0.0f, 1.0f);
        mc.specularColor.z = std::clamp(mc.specularColor.z, 0.0f, 1.0f);

        float reflectance = 0.0f;
        material->Get(AI_MATKEY_REFLECTIVITY, reflectance); // 0〜1 を想定
        mc.reflectance = reflectance;

        aiColor3D reflColor(1.f, 1.f, 1.f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_REFLECTIVE, reflColor)) {
            mc.reflectionColor = { reflColor.r, reflColor.g, reflColor.b };
        }
        mc.reflectionColor.x = std::clamp(mc.reflectionColor.x, 0.0f, 1.0f);
        mc.reflectionColor.y = std::clamp(mc.reflectionColor.y, 0.0f, 1.0f);
        mc.reflectionColor.z = std::clamp(mc.reflectionColor.z, 0.0f, 1.0f);

        // shininess があればスペキュラーを有効化する目安とする
        float shininess = 0.0f;
        material->Get(AI_MATKEY_SHININESS, shininess);

        // reflectance が0なら、specularColorから簡易F0を推定
        if (mc.reflectance <= 0.0f) {
            float avgSpec = (mc.specularColor.x + mc.specularColor.y + mc.specularColor.z) / 3.0f;
            mc.reflectance = std::min(1.0f, std::max(0.0f, avgSpec));
        }

        bool hasSpec = (mc.specularColor.x > 0.0f || mc.specularColor.y > 0.0f || mc.specularColor.z > 0.0f);
        // 減衰は shininess を0-1に粗く正規化（32を標準とみなす）。最低でも reflectance や specColor があればオン。
        float attenuationFromShininess = std::min(1.0f, shininess / 32.0f);
        if (hasSpec || mc.reflectance > 0.0f || shininess > 0.0f) {
            mc.specularAttenuation = std::max(attenuationFromShininess, 0.2f); // 最低限効かせる
        } else {
            mc.specularAttenuation = 0.0f;
        }

        // 反射・スペキュラーが強すぎて白飛びしないように上限を設ける
        mc.reflectance = std::clamp(mc.reflectance, 0.0f, 0.25f);                // 金属感も0.25程度まで
        mc.specularAttenuation = std::clamp(mc.specularAttenuation, 0.0f, 0.6f); // ハイライト総量の抑制

        // 以前は「player」含みのモデルをアンリット強制していたが、
        // シェーディングが当たらず真っ白になるため解除する。
    }


    // ボーンの処理
    if (mesh->HasBones()) {
        mc.isSkinned = true;
        mc.skeleton.bones.resize(mesh->mNumBones);
        mc.skeleton.boneTransforms.resize(mesh->mNumBones);

        // 事前に全ボーン名セットを構築
        std::unordered_set<std::string> allBoneNames;
        allBoneNames.reserve(mesh->mNumBones);
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            allBoneNames.insert(SanitizeFbxChannelName(mesh->mBones[i]->mName.C_Str()));
        }

        // ノードツリーから親関係を収集
        std::unordered_map<std::string, std::string> parentByName;
        CollectBoneParents(scene->mRootNode, allBoneNames, parentByName);

        // ボーン名からインデックスへのマップ (親検索用) ※AssimpFbxサフィックスを除去して統一
        std::unordered_map<std::string, int> boneNameToIndex;
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            std::string sanitized = SanitizeFbxChannelName(mesh->mBones[i]->mName.C_Str());
            boneNameToIndex[sanitized] = i;
        }

        // 親に存在するがボーン配列に無いノード（例: RootNode）をスケルトンに追加して親子関係を失わないようにする
        for (const auto& kv : parentByName) {
            const std::string& parentName = kv.second;
            if (boneNameToIndex.find(parentName) == boneNameToIndex.end()) {
                int newIndex = static_cast<int>(mc.skeleton.bones.size());
                ModelComponent::Bone pseudo{};
                pseudo.name = parentName;
                pseudo.parentIndex = -1;
                DirectX::XMStoreFloat4x4(&pseudo.offsetMatrix, DirectX::XMMatrixIdentity());
                mc.skeleton.bones.push_back(pseudo);
                mc.skeleton.boneTransforms.push_back(DirectX::XMFLOAT4X4{
                    1,0,0,0,
                    0,1,0,0,
                    0,0,1,0,
                    0,0,0,1
                });
                boneNameToIndex[parentName] = newIndex;
#ifdef _DEBUG
                DEBUGLOG_WARNING("ModelLoader: added pseudo parent bone '" + parentName + "' idx=" + std::to_string(newIndex));
#endif
            }
        }

        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];
            std::string boneName = SanitizeFbxChannelName(bone->mName.C_Str());
            
            mc.skeleton.bones[i].name = boneName;
            
            // オフセット行列はメッシュ空間->ボーン空間(バインド)の変換。シェーダー側で行ベクトル mul(pos, M) を使う前提のため転置して保存。
            aiMatrix4x4 offset = bone->mOffsetMatrix;
            offset.Transpose();
            memcpy(&mc.skeleton.bones[i].offsetMatrix, &offset, sizeof(DirectX::XMFLOAT4X4));
#ifdef _DEBUG
            if (boneName == "Hips") {
                DEBUGLOG("ModelLoader: Hips offset (DX row-major, transposed) = " + ToString(mc.skeleton.bones[i].offsetMatrix));
            }
#endif
            
            // ウェイトの登録
            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                const aiVertexWeight& weight = bone->mWeights[j];
                unsigned int vertexId = weight.mVertexId;
                float w = weight.mWeight;

                if (vertexId >= vertices.size()) continue;

                // 空いているスロットを探す
                for (int k = 0; k < 4; k++) {
                    if (vertices[vertexId].BoneWeights[k] == 0.0f) {
                        vertices[vertexId].BoneWeights[k] = w;
                        vertices[vertexId].BoneIndices[k] = i;
                        break;
                    }
                }
            }

            // 親ボーンの検索
            // 親ボーン探索: 非ボーンノード(Armature等)を経由する場合は最近傍のボーン祖先まで辿る
            auto resolveParentIndex = [&](const std::string& startName) -> int {
                std::string cur = startName;
                std::unordered_set<std::string> visited;
                while (!cur.empty() && !visited.count(cur)) {
                    visited.insert(cur);
                    auto pit = boneNameToIndex.find(cur);
                    if (pit != boneNameToIndex.end()) return pit->second;
                    auto up = parentByName.find(cur);
                    if (up == parentByName.end()) break;
                    cur = up->second;
                }
                return -1;
            };

            int parentIdx = -1;
            auto itParentName = parentByName.find(boneName);
            if (itParentName != parentByName.end()) {
                parentIdx = resolveParentIndex(itParentName->second);
            }
            mc.skeleton.bones[i].parentIndex = parentIdx;
#ifdef _DEBUG
            if (parentIdx < 0) {
                DEBUGLOG_WARNING("ModelLoader: parent not resolved for '" + boneName + "', originalParent='" +
                                  (itParentName != parentByName.end() ? itParentName->second : std::string("(none)")) + "'");
            }
#endif
        }
        
        // ウェイトの正規化 (合計が1になるように)
        size_t zeroWeightVerts = 0;
        for (auto& v : vertices) {
            float sum = v.BoneWeights[0] + v.BoneWeights[1] + v.BoneWeights[2] + v.BoneWeights[3];
            if (sum > 0.0f) {
                for (int k = 0; k < 4; k++) v.BoneWeights[k] /= sum;
            } else {
                // ウェイトが付与されなかった頂点はルートボーンに1.0を割り当てて伸縮を防ぐ
                v.BoneWeights[0] = 1.0f;
                v.BoneIndices[0] = 0;
                zeroWeightVerts++;
            }
        }

#ifdef _DEBUG
        DEBUGLOG("Skeleton for mesh (bones=" + std::to_string(mesh->mNumBones) +
                 ", zeroWeightVerts=" + std::to_string(zeroWeightVerts) +
                 ", totalVerts=" + std::to_string(vertices.size()) + "):");
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            const auto& b = mc.skeleton.bones[i];
            DEBUGLOG("  Bone[" + std::to_string(i) + "]: name=" + b.name +
                     ", parentIndex=" + std::to_string(b.parentIndex));
        }
        // サンプル頂点のボーンインデックス/ウェイトをいくつか出力
        const size_t sampleCount = std::min<size_t>(5, vertices.size());
        for (size_t vi = 0; vi < sampleCount; ++vi) {
            const auto& v = vertices[vi];
            float sumW = v.BoneWeights[0] + v.BoneWeights[1] + v.BoneWeights[2] + v.BoneWeights[3];
            DEBUGLOG("  Vert[" + std::to_string(vi) + "]: idx={" +
                     std::to_string(v.BoneIndices[0]) + "," +
                     std::to_string(v.BoneIndices[1]) + "," +
                     std::to_string(v.BoneIndices[2]) + "," +
                     std::to_string(v.BoneIndices[3]) + "} w={" +
                     std::to_string(v.BoneWeights[0]) + "," +
                     std::to_string(v.BoneWeights[1]) + "," +
                     std::to_string(v.BoneWeights[2]) + "," +
                     std::to_string(v.BoneWeights[3]) + "} sum=" +
                     std::to_string(sumW));
        }
#endif
    }

    // 頂点バッファの作成（ボーン／ウェイト処理の後で作ること）
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

    return mc;
}

// アニメーションクリップ構築（ノードベースの名前解決対応）
static std::vector<ModelComponent::AnimationClip> BuildClipsFromAssimp(const aiScene* scene) {
    std::vector<ModelComponent::AnimationClip> clips;
    if (!scene || !scene->mAnimations || scene->mNumAnimations == 0) return clips;

    // ノードツリーから名前→インデックスを収集（スキニング配列とは異なるため使用しない）
    std::map<std::string, int> nameIndex;
    if (scene->mRootNode) {
        CollectNodeNames(scene->mRootNode, nameIndex);
    }

#ifdef _DEBUG
    DEBUGLOG("BuildClipsFromAssimp: node-tree names collected (for reference only):");
    for (const auto& kv : nameIndex) {
        DEBUGLOG("  name=" + kv.first + ", idx=" + std::to_string(kv.second));
    }
#endif

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        const aiAnimation* a = scene->mAnimations[i];
        if (!a) continue;

        ModelComponent::AnimationClip clip;
        clip.name = (a->mName.length > 0) ? a->mName.C_Str() : ("Anim_" + std::to_string(i));
        clip.duration = static_cast<float>(a->mDuration);
        clip.ticksPerSecond = (a->mTicksPerSecond != 0.0) ? static_cast<float>(a->mTicksPerSecond) : 30.0f;
        const float origDuration = clip.duration;

        std::unordered_map<std::string, ModelComponent::BoneAnimation> boneAnimMap;
        float maxTime = -std::numeric_limits<float>::infinity();
        for (unsigned int c = 0; c < a->mNumChannels; ++c) {
            const aiNodeAnim* ch = a->mChannels[c];
            if (!ch) continue;

            std::string rawName = ch->mNodeName.C_Str();
            std::string sanitizedName = SanitizeFbxChannelName(rawName);

            auto& boneAnim = boneAnimMap[sanitizedName];
            if (boneAnim.boneName.empty()) {
                boneAnim.boneName = sanitizedName;
                // 重要: スキニング側（meshの mBones[] 配列）と一致するインデックスは、後段の AnimationSystem で
                // 名前マッピングにより決定されるため、ここでは -1 を設定しておく。
                boneAnim.boneIndex = -1;
#ifdef _DEBUG
                DEBUGLOG("  Channel->BoneAnimation: raw=" + rawName +
                         ", name=" + sanitizedName +
                         ", mappedIndex(mesh)=-1 (deferred by AnimationSystem)");
#endif
            }

            // 位置キー
            for (unsigned int k = 0; k < ch->mNumPositionKeys; ++k) {
                const aiVectorKey& vk = ch->mPositionKeys[k];
                auto& kf = FindOrCreateKeyframe(boneAnim.keyframes, static_cast<float>(vk.mTime));
                kf.position = { vk.mValue.x, vk.mValue.y, vk.mValue.z };
                boneAnim.hasPositionKeys = true;
            }
            // 回転キー
            for (unsigned int k = 0; k < ch->mNumRotationKeys; ++k) {
                const aiQuatKey& rk = ch->mRotationKeys[k];
                auto& kf = FindOrCreateKeyframe(boneAnim.keyframes, static_cast<float>(rk.mTime));
                kf.rotation = { rk.mValue.x, rk.mValue.y, rk.mValue.z, rk.mValue.w };
                boneAnim.hasRotationKeys = true;
            }
            // スケールキー
            for (unsigned int k = 0; k < ch->mNumScalingKeys; ++k) {
                const aiVectorKey& sk = ch->mScalingKeys[k];
                auto& kf = FindOrCreateKeyframe(boneAnim.keyframes, static_cast<float>(sk.mTime));
                kf.scale = { sk.mValue.x, sk.mValue.y, sk.mValue.z };
                boneAnim.hasScaleKeys = true;
            }
        }

        // キーフレーム整形と追加
        for (auto& pair : boneAnimMap) {
            auto& boneAnim = pair.second;
            if (boneAnim.keyframes.empty()) continue;
            std::sort(boneAnim.keyframes.begin(), boneAnim.keyframes.end(),
                      [](const ModelComponent::Keyframe& a, const ModelComponent::Keyframe& b) { return a.time < b.time; });
            maxTime = std::max(maxTime, boneAnim.keyframes.back().time);

            DirectX::XMFLOAT3 lastPos{0.0f, 0.0f, 0.0f};
            DirectX::XMFLOAT4 lastRot{0.0f, 0.0f, 0.0f, 1.0f};
            DirectX::XMFLOAT3 lastScale{1.0f, 1.0f, 1.0f};
            bool hasPos = false, hasRot = false, hasScale = false;

            for (auto& kf : boneAnim.keyframes) {
                if (std::isnan(kf.position.x) || std::isnan(kf.position.y) || std::isnan(kf.position.z)) {
                    kf.position = hasPos ? lastPos : DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f};
                } else { lastPos = kf.position; hasPos = true; }
                if (std::isnan(kf.rotation.w)) {
                    kf.rotation = hasRot ? lastRot : DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f};
                } else { lastRot = kf.rotation; hasRot = true; }
                if (std::isnan(kf.scale.x) || std::isnan(kf.scale.y) || std::isnan(kf.scale.z)) {
                    kf.scale = hasScale ? lastScale : DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f};
                } else { lastScale = kf.scale; hasScale = true; }
            }

            clip.boneAnimations.push_back(std::move(boneAnim));
        }

        // 最小時刻を0に正規化
        float minTime = std::numeric_limits<float>::infinity();
        for (const auto& ba : clip.boneAnimations) {
            if (!ba.keyframes.empty()) {
                minTime = std::min(minTime, ba.keyframes.front().time);
            }
        }
        if (std::isfinite(minTime) && minTime != 0.0f) {
            for (auto& ba : clip.boneAnimations) {
                for (auto& kf : ba.keyframes) {
                    kf.time -= minTime;
                }
            }
            const float durationFromKeys = std::isfinite(maxTime) ? (maxTime - minTime) : 0.0f;
            const float durationFromAssimp = origDuration - minTime;
            clip.duration = std::max(0.0f, std::max(durationFromKeys, durationFromAssimp));
        } else {
            const float durationFromKeys = std::isfinite(maxTime) ? maxTime : 0.0f;
            clip.duration = std::max(0.0f, std::max(durationFromKeys, origDuration));
        }

        if (!clip.boneAnimations.empty() && clip.duration > 0.0f) {
            clips.push_back(std::move(clip));
        }
    }

    return clips;
}

std::vector<ModelComponent::AnimationClip> ModelLoader::LoadAnimation(const std::string& path)
{
    DEBUGLOG("ModelLoader::LoadAnimation - begin: " + path);

    std::vector<ModelComponent::AnimationClip> clips;

    Assimp::Importer importer;

    const unsigned int primaryFlags =
        aiProcess_Triangulate |
        aiProcess_LimitBoneWeights |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType;

    DEBUGLOG("Assimp (primary) flags: Triangulate | LimitBoneWeights | JoinIdenticalVertices | SortByPType");

    const aiScene* scene = importer.ReadFile(path, primaryFlags);
    if (!scene) {
        DEBUGLOG_WARNING(std::string("Assimp Error (anim,primary): ") + importer.GetErrorString() + " | file=" + path);
    } else {
        DEBUGLOG("Assimp primary load OK: " + path +
                 ", animations=" + std::to_string(scene->mNumAnimations) +
                 ", meshes=" + std::to_string(scene->mNumMeshes));

        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            const aiAnimation* a = scene->mAnimations[i];
            std::string name = (a && a->mName.length > 0) ? a->mName.C_Str() : ("Anim_" + std::to_string(i));
            DEBUGLOG("  aiAnimation[" + std::to_string(i) + "]: name=" + name +
                     ", durationTicks=" + std::to_string(a ? a->mDuration : 0.0) +
                     ", ticksPerSec=" + std::to_string(a ? a->mTicksPerSecond : 0.0) +
                     ", channels=" + std::to_string(a ? a->mNumChannels : 0u));
        }
    }

    Assimp::Importer fallbackImporter;
    const aiScene* fallbackScene = nullptr;
    if (!scene || scene->mNumAnimations == 0) {
        const unsigned int fallbackFlags = aiProcess_Triangulate;
        DEBUGLOG("Assimp (fallback-minimal) flags: Triangulate");

        fallbackScene = fallbackImporter.ReadFile(path, fallbackFlags);
        if (!fallbackScene) {
            DEBUGLOG_WARNING(std::string("Assimp Error (anim,fallback-minimal): ") +
                             fallbackImporter.GetErrorString() + " | file=" + path);
        } else {
            DEBUGLOG("Assimp fallback-minimal load OK: " + path +
                     ", animations=" + std::to_string(fallbackScene->mNumAnimations) +
                     ", meshes=" + std::to_string(fallbackScene->mNumMeshes));

            for (unsigned int i = 0; i < fallbackScene->mNumAnimations; ++i) {
                const aiAnimation* a = fallbackScene->mAnimations[i];
                std::string name = (a && a->mName.length > 0) ? a->mName.C_Str() : ("Anim_" + std::to_string(i));
                DEBUGLOG("  [FB] aiAnimation[" + std::to_string(i) + "]: name=" + name +
                         ", durationTicks=" + std::to_string(a ? a->mDuration : 0.0) +
                         ", ticksPerSec=" + std::to_string(a ? a->mTicksPerSecond : 0.0) +
                         ", channels=" + std::to_string(a ? a->mNumChannels : 0u));
            }
        }

        if (fallbackScene) {
            scene = fallbackScene;
        }
    }

    if (!scene || scene->mNumAnimations == 0) {
        DEBUGLOG_WARNING("Failed to load any animations from: " + path);
        DEBUGLOG("ModelLoader::LoadAnimation - end (no clips): " + path);
        return clips;
    }

    clips = BuildClipsFromAssimp(scene);

    DEBUGLOG("ModelLoader::LoadAnimation - end: " + path +
             ", clips=" + std::to_string(clips.size()));
    return clips;
}

TextureManager::TextureHandle ModelLoader::LoadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& directory,
    const std::string& modelFilePath)
{
    TextureManager::TextureHandle handle = TextureManager::INVALID_TEXTURE;

    if (!mat) {
        return handle;
    }

    TextureManager& texMgr = ServiceLocator::Get<TextureManager>();

    const unsigned int texCount = mat->GetTextureCount(type);
    if (texCount == 0) {
        return handle;
    }

    aiString pathStr;
    if (mat->GetTexture(type, 0, &pathStr) != AI_SUCCESS) {
        return handle;
    }

    std::string rawPath = pathStr.C_Str();
    std::string resolved = ResolveTexturePath(rawPath, directory);

    if (resolved.empty()) {
        DEBUGLOG_WARNING("ModelLoader: texture path could not be resolved: '" + rawPath +
                         "' (model: " + modelFilePath + ")");
        return handle;
    }

    // TextureManager は LoadFromFile(const char*) を提供
    handle = texMgr.LoadFromFile(resolved.c_str());
    if (handle == TextureManager::INVALID_TEXTURE) {
        DEBUGLOG_WARNING("ModelLoader: failed to load texture: " + resolved +
                         " (model: " + modelFilePath + ")");
    } else {
        DEBUGLOG_CATEGORY(DebugLog::Category::Render,
            "ModelLoader: loaded texture '" + resolved + "' for model '" + modelFilePath + "'");
    }

    return handle;
}
