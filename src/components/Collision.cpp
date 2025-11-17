/**
 * @file Collision.cpp
 * @brief 衝突判定システムの実装
 * @author 立山悠朔・上手凉太郎・山内陽
 * @date 2025
 * @version 2.1
 */

#include "pch.h"
#include "components/Collision.h"
#include "app/BuildConfig.h"
#include "scenes/Game.h"
#include <unordered_set>
#include <typeindex>

#if ENABLE_DEBUG_VISUALS
#include "graphics/DebugDraw.h"
#include "app/ServiceLocator.h"
#endif

// ========================================================
// CollisionHandlerRegistry 実装
// ========================================================
namespace {
    using TryFunc = CollisionHandlerRegistry::TryFunc;

    std::vector<TryFunc>& HandlerFuncs() {
        static std::vector<TryFunc> v;
        return v;
    }
    std::unordered_set<size_t>& RegisteredTypes() {
        static std::unordered_set<size_t> s;
        return s;
    }
}

void CollisionHandlerRegistry::RegisterType(std::type_index type, TryFunc func) {
    size_t key = type.hash_code();
    auto& types = RegisteredTypes();
    if (types.insert(key).second) {
        HandlerFuncs().push_back(func);
    }
}

void CollisionHandlerRegistry::ForEach(World& w, Entity e, const std::function<void(ICollisionHandler*)>& func) {
    for (auto f : HandlerFuncs()) { f(w, e, func); }
}

// ========================================================
// CollisionDetectionSystem のイベントハンドラー実装
// ========================================================

void CollisionDetectionSystem::ForEachHandler(World& w, Entity e, const std::function<void(ICollisionHandler*)>& func) {
    CollisionHandlerRegistry::ForEach(w, e, func);
}

void CollisionDetectionSystem::TriggerCollisionEnter(World& w, Entity a, Entity b, const CollisionInfo& info) {
    DEBUGLOG("🔥 OnCollisionEnter: Entity " + std::to_string(a.id) + " <-> Entity " + std::to_string(b.id));

    // エンティティAのハンドラーを呼び出す
    ForEachHandler(w, a, [&](ICollisionHandler* handler) {
        DEBUGLOG("  ✅ Entity " + std::to_string(a.id) + " has handler, calling OnCollisionEnter");
        handler->OnCollisionEnter(w, a, b, info);
    });

    // エンティティBのハンドラーを呼び出す(法線を反転)
    ForEachHandler(w, b, [&](ICollisionHandler* handler) {
        DEBUGLOG("  ✅ Entity " + std::to_string(b.id) + " has handler, calling OnCollisionEnter");
        CollisionInfo reversedInfo = info;
  std::swap(reversedInfo.entityA, reversedInfo.entityB);
      reversedInfo.normal.x = -reversedInfo.normal.x;
        reversedInfo.normal.y = -reversedInfo.normal.y;
        reversedInfo.normal.z = -reversedInfo.normal.z;
        handler->OnCollisionEnter(w, b, a, reversedInfo);
    });
}

void CollisionDetectionSystem::TriggerCollisionStay(World& w, Entity a, Entity b, const CollisionInfo& info) {
    ForEachHandler(w, a, [&](ICollisionHandler* handler) {
        handler->OnCollisionStay(w, a, b, info);
    });

    ForEachHandler(w, b, [&](ICollisionHandler* handler) {
        CollisionInfo reversedInfo = info;
        std::swap(reversedInfo.entityA, reversedInfo.entityB);
      reversedInfo.normal.x = -reversedInfo.normal.x;
    reversedInfo.normal.y = -reversedInfo.normal.y;
        reversedInfo.normal.z = -reversedInfo.normal.z;
      handler->OnCollisionStay(w, b, a, reversedInfo);
    });
}

void CollisionDetectionSystem::TriggerCollisionExit(World& w, Entity a, Entity b) {
  if (!w.IsAlive(a) || !w.IsAlive(b)) return;

    DEBUGLOG("🔚 OnCollisionExit: Entity " + std::to_string(a.id) + " <-> Entity " + std::to_string(b.id));

    ForEachHandler(w, a, [&](ICollisionHandler* handler) {
        handler->OnCollisionExit(w, a, b);
    });

    ForEachHandler(w, b, [&](ICollisionHandler* handler) {
        handler->OnCollisionExit(w, b, a);
    });
}
