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

struct ModelLoadingSystem : public Behaviour {
    void OnUpdate(World &world, Entity self, float dt) override {
        auto &resMgr = ServiceLocator::Get<ResourceManager>();

        world.ForEach<Model>([&](Entity entity, Model &model) {
            if (world.Has<ModelComponent>(entity)) {
                return;
            }

            const auto &components = resMgr.GetModel(model.filePath);
            if (components.empty()) {
                world.Remove<Model>(entity);
                return;
            }

            world.Add<ModelComponent>(entity, components[0]);

            if (!world.Has<TransformHierarchy>(entity)) {
                world.Add<TransformHierarchy>(entity);
            }
            auto *parentHierarchy = world.TryGet<TransformHierarchy>(entity);

            for (size_t i = 1; i < components.size(); ++i) {
                Entity child = world.Create()
                                   .With<Transform>()
                                   .With<ModelComponent>(components[i])
                                   .With<TransformHierarchy>()
                                   .Build();

                auto *childHierarchy = world.TryGet<TransformHierarchy>(child);
                if (childHierarchy && parentHierarchy) {
                    childHierarchy->SetParent(entity);
                    parentHierarchy->AddChild(child);
                }
            }
        });
    }
};
