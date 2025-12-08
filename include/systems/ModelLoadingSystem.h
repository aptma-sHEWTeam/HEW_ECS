/**
 * @file ModelLoadingSystem.h
 * @brief 3Dモデルのロードシステム
 * @author 山内陽
 */
#pragma once

#include "ecs/World.h"
#include "components/Model.h"
#include "components/Component.h"
#include "components/ModelComponent.h"
#include "components/Transform.h"
#include "components/TransformHierarchy.h"
#include "app/ServiceLocator.h"
#include "app/ResourceManager.h"
#include "components/ModelPrefab.h"

struct ModelLoadingSystem : public Behaviour {
    void OnUpdate(World &world, Entity self, float dt) override {
        auto &resMgr = ServiceLocator::Get<ResourceManager>();

        world.ForEach<Model>([&](Entity entity, Model &model) {
            if (world.Has<ModelComponent>(entity)) {
                return;
            }

            const auto &nodes = resMgr.GetModel(model.filePath);
            if (nodes.empty()) {
                // Fallback: if model cannot be loaded, attach a simple placeholder mesh
                MeshRenderer placeholder{};
                placeholder.meshType = MeshType::Cube;
                placeholder.color = DirectX::XMFLOAT3{ 0.8f, 0.2f, 0.2f };
                if (!world.Has<MeshRenderer>(entity)) {
                    world.Add<MeshRenderer>(entity, placeholder);
                }
                // Remove Model to avoid retry each frame
                world.Remove<Model>(entity);
                return;
            }

            // 親になるエンティティに階層コンポーネントを付与
            if (!world.Has<TransformHierarchy>(entity)) {
                world.Add<TransformHierarchy>(entity);
            }

            std::vector<Entity> created;
            created.reserve(nodes.size());

            // 1st pass: エンティティ生成（Transform/Hierarchy/ModelComponent）
            for (const auto &node : nodes) {
                Transform t{ node.translation, node.rotationDeg, node.scale };
                auto builder = world.Create()
                                   .With<Transform>(t)
                                   .With<TransformHierarchy>();
                if (node.hasMesh && node.component.indexCount > 0) {
                    builder.With<ModelComponent>(node.component);
                }
                Entity createdEntity = builder.Build();
                created.push_back(createdEntity);
            }

            // 2nd pass: 親子リンク設定（親が存在しない場合は元エンティティを親にする）
            for (size_t i = 0; i < nodes.size(); ++i) {
                const auto &node = nodes[i];
                Entity child = created[i];

                Entity parent = entity;
                if (node.parentIndex >= 0 && static_cast<size_t>(node.parentIndex) < created.size()) {
                    parent = created[node.parentIndex];
                }

                auto *childHierarchy = world.TryGet<TransformHierarchy>(child);
                auto *parentHierarchy = world.TryGet<TransformHierarchy>(parent);
                if (childHierarchy && parentHierarchy) {
                    childHierarchy->SetParent(parent);
                    parentHierarchy->AddChild(child);
                }
            }

            // Modelコンポーネントは1回処理したら除去して再生成を防ぐ
            world.Remove<Model>(entity);
        });
    }
};
