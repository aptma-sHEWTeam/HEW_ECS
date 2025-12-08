/**
 * @file SceneManager.h
 * @brief Scene management utilities.
 */
#pragma once

#include "app/DebugLog.h"
#include "ecs/World.h"
#include "input/InputSystem.h"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @class IScene
 * @brief Interface all scenes must implement.
 */
class IScene {
public:
    virtual ~IScene() = default;

    virtual void OnEnter(World& world) = 0;
    virtual void OnUpdate(World& world, InputSystem& input, float deltaTime) = 0;
    virtual void OnRender(World& world) {} // Default empty implementation
    virtual void OnExit(World& world) = 0;

    virtual bool ShouldChangeScene() const { return false; }
    virtual const char* GetNextScene() const { return nullptr; }
};

/**
 * @class SceneManager
 * @brief Owns scene instances and orchestrates transitions.
 */
class SceneManager {
public:
    /**
     * @brief Initialise manager and enter the first scene.
     * @param startSceneName Name of the scene to activate. May be nullptr.
     * @param world ECS world reference.
     */
    void Init(const char* startSceneName, World& world) {
        isShutdown_ = false;
        currentScene_ = FindScene(startSceneName);
        if (!currentScene_) {
            DEBUGLOG_WARNING("SceneManager::Init() - start scene not found");
            return;
        }
        currentScene_->OnEnter(world);
    }

    /**
     * @brief Register a scene instance.
     * @param name Scene identifier.
     * @param scene Scene instance (ownership is transferred).
     */
    void RegisterScene(const char* name, std::unique_ptr<IScene> scene) {
        if (!name || !scene) {
            DEBUGLOG_WARNING("SceneManager::RegisterScene() - invalid input");
            return;
        }
        scenes_[name] = std::move(scene);
    }

    /**
     * @brief Update active scene and perform transitions when requested.
     */
    void Update(World& world, InputSystem& input, float deltaTime) {
        if (!currentScene_) {
            return;
        }

        currentScene_->OnUpdate(world, input, deltaTime);

        if (currentScene_->ShouldChangeScene()) {
            ChangeScene(currentScene_->GetNextScene(), world);
        }
    }

    /**
     * @brief Render active scene.
     */
    void Render(World& world) {
        if (!currentScene_) {
            return;
        }
        currentScene_->OnRender(world);
    }

    /**
     * @brief Transition to a different scene.
     */
    void ChangeScene(const char* sceneName, World& world) {
        if (!sceneName) {
            return;
        }


        IScene* nextScene = FindScene(sceneName);
        if (!nextScene || nextScene == currentScene_) {
            return;
        }

        if (currentScene_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Scene change: OnExit()");
            currentScene_->OnExit(world);
            world.FlushDestroyEndOfFrame();
        }

        currentScene_ = nextScene;
        DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Scene change: OnEnter()");
        currentScene_->OnEnter(world);
    }

    /**
     * @brief Accessor for the currently active scene.
     */
    IScene* GetCurrentScene() const { return currentScene_; }

    /**
     * @brief Destructor performs sanity logging.
     */
    ~SceneManager() {
        DEBUGLOG("SceneManager::~SceneManager()");
        if (!isShutdown_) {
            DEBUGLOG_WARNING("SceneManager was destroyed without Shutdown().");
        }
    }

    /**
     * @brief Explicit shutdown. Safe to call multiple times.
     */
    void Shutdown(World& world) {
        if (isShutdown_) {
            return;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "SceneManager::Shutdown()");

        if (currentScene_) {
            currentScene_->OnExit(world);
            currentScene_ = nullptr;
        }

        scenes_.clear();
        isShutdown_ = true;
    }

private:
    IScene* FindScene(const char* name) {
        if (!name) {
            return nullptr;
        }
        auto it = scenes_.find(name);
        if (it == scenes_.end()) {
            DEBUGLOG_WARNING(std::string("Scene not found: ") + name);
            return nullptr;
        }
        return it->second.get();
    }

    IScene* currentScene_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IScene>> scenes_;
    bool isShutdown_ = false;
};


