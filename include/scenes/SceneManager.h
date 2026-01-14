/**
 * @file SceneManager.h
 * @brief Scene management utilities.
 */
#pragma once

#include "app/DebugLog.h"
#include "ecs/World.h"
#include "input/InputSystem.h"
#include "systems/SoundSystem.h"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @enum TransitionDirection
 * @brief Slide transition direction for scene changes
 */
enum class TransitionDirection {
    None, // No transition (instant)
    Left, // Slide left (going to previous)
    Right // Slide right (going to next)
};

/**
 * @enum TransitionPhase
 * @brief Current phase of the transition animation
 */
enum class TransitionPhase {
    None,     // No transition active
    SlideOut, // Current scene sliding out
    SlideIn   // New scene sliding in
};

/**
 * @class IScene
 * @brief Interface all scenes must implement.
 */
class IScene {
  public:
    virtual ~IScene() = default;

    virtual void OnEnter(World &world) = 0;
    virtual void OnUpdate(World &world, InputSystem &input, float deltaTime) = 0;
    virtual void OnRender(World &world) {} // Default empty implementation
    virtual void OnExit(World &world) = 0;

    virtual bool ShouldChangeScene() const {
        return false;
    }
    virtual const char *GetNextScene() const {
        return nullptr;
    }
};

/**
 * @class SceneManager
 * @brief Owns scene instances and orchestrates transitions.
 */
class SceneManager {
  public:
    // Transition configuration
    static constexpr float TRANSITION_DURATION = 0.35f; // Total duration for slide out/in
    static constexpr float SCREEN_WIDTH_FACTOR = 1.2f;  // How far to slide (1.0 = full screen width)
    /**
     * @brief Initialise manager and enter the first scene.
     * @param startSceneName Name of the scene to activate. May be nullptr.
     * @param world ECS world reference.
     */
    void Init(const char *startSceneName, World &world) {
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
    void RegisterScene(const char *name, std::unique_ptr<IScene> scene) {
        if (!name || !scene) {
            DEBUGLOG_WARNING("SceneManager::RegisterScene() - invalid input");
            return;
        }
        scenes_[name] = std::move(scene);
    }

    /**
     * @brief Update active scene and perform transitions when requested.
     */
    void Update(World &world, InputSystem &input, float deltaTime) {
        if (!currentScene_) {
            return;
        }

        // Handle transition animation
        if (transitionPhase_ != TransitionPhase::None) {
            UpdateTransition(world, deltaTime);
            // During transition, still update scene but input is effectively blocked
            // by the transition state check in scenes
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
    void Render(World &world) {
        if (!currentScene_) {
            return;
        }
        currentScene_->OnRender(world);
    }

    /**
     * @brief Transition to a different scene.
     */
    void ChangeScene(const char *sceneName, World &world) {
        if (!sceneName) {
            return;
        }

        IScene *nextScene = FindScene(sceneName);
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
     * @brief Transition to a different scene with slide animation.
     * @param sceneName Target scene name
     * @param world ECS world reference
     * @param direction Slide direction (Left = going to previous, Right = going to next)
     */
    void ChangeSceneWithTransition(const char *sceneName, World &world, TransitionDirection direction) {
        if (!sceneName || direction == TransitionDirection::None) {
            // Fall back to instant transition
            ChangeScene(sceneName, world);
            return;
        }

        IScene *nextScene = FindScene(sceneName);
        if (!nextScene || nextScene == currentScene_) {
            return;
        }

        // Store transition state
        pendingSceneName_ = sceneName;
        transitionDirection_ = direction;
        transitionPhase_ = TransitionPhase::SlideOut;
        transitionProgress_ = 0.0f;

        DEBUGLOG_CATEGORY(DebugLog::Category::Scene,
                          std::string("Starting slide transition to: ") + sceneName +
                              " direction: " + (direction == TransitionDirection::Right ? "Right" : "Left"));
    }

    /**
     * @brief Get the current transition offset for rendering.
     * @return Normalized offset (-1.0 to 1.0) where negative = left, positive = right
     * 
     * Scenes should multiply this by their screen width to offset all rendering.
     */
    float GetTransitionOffset() const {
        if (transitionPhase_ == TransitionPhase::None) {
            return 0.0f;
        }

        // Apply easing for smooth animation
        float easedProgress = EaseInOutCubic(transitionProgress_);

        // Direction multiplier: Right means content moves right (positive), Left means left (negative)
        float dirMult = (transitionDirection_ == TransitionDirection::Right) ? 1.0f : -1.0f;

        if (transitionPhase_ == TransitionPhase::SlideOut) {
            // Slide out: 0 -> full offset in direction
            return dirMult * easedProgress * SCREEN_WIDTH_FACTOR;
        } else {
            // Slide in: full offset from opposite direction -> 0
            return -dirMult * (1.0f - easedProgress) * SCREEN_WIDTH_FACTOR;
        }
    }

    /**
     * @brief Check if a transition is currently in progress.
     */
    bool IsTransitioning() const {
        return transitionPhase_ != TransitionPhase::None;
    }

    /**
     * @brief Get the current transition phase.
     */
    TransitionPhase GetTransitionPhase() const {
        return transitionPhase_;
    }

    /**
     * @brief Accessor for the currently active scene.
     */
    IScene *GetCurrentScene() const {
        return currentScene_;
    }

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
    void Shutdown(World &world) {
        if (isShutdown_) {
            return;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "SceneManager::Shutdown()");

        if (currentScene_) {
            currentScene_->OnExit(world);
            currentScene_ = nullptr;
        }

        SOUND_SYS.StopBGM();
        scenes_.clear();
        isShutdown_ = true;
    }

  private:
    /**
     * @brief Update transition animation state.
     */
    void UpdateTransition(World &world, float deltaTime) {
        transitionProgress_ += deltaTime / TRANSITION_DURATION;

        if (transitionProgress_ >= 1.0f) {
            transitionProgress_ = 1.0f;

            if (transitionPhase_ == TransitionPhase::SlideOut) {
                // Complete slide out, perform actual scene change
                PerformSceneSwitch(world);

                // Start slide in phase
                transitionPhase_ = TransitionPhase::SlideIn;
                transitionProgress_ = 0.0f;
            } else {
                // Slide in complete, transition finished
                transitionPhase_ = TransitionPhase::None;
                transitionDirection_ = TransitionDirection::None;
                pendingSceneName_.clear();

                DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Slide transition complete");
            }
        }
    }

    /**
     * @brief Perform the actual scene switch (called mid-transition).
     */
    void PerformSceneSwitch(World &world) {
        if (pendingSceneName_.empty()) {
            return;
        }

        IScene *nextScene = FindScene(pendingSceneName_.c_str());
        if (!nextScene) {
            transitionPhase_ = TransitionPhase::None;
            return;
        }

        if (currentScene_) {
            DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Transition: OnExit()");
            currentScene_->OnExit(world);
            world.FlushDestroyEndOfFrame();
        }

        currentScene_ = nextScene;
        DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Transition: OnEnter()");
        currentScene_->OnEnter(world);
    }

    /**
     * @brief Cubic ease-in-out function for smooth animation.
     */
    static float EaseInOutCubic(float t) {
        if (t < 0.5f) {
            return 4.0f * t * t * t;
        }
        float p = 2.0f * t - 2.0f;
        return 0.5f * p * p * p + 1.0f;
    }

    IScene *FindScene(const char *name) {
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

    IScene *currentScene_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IScene>> scenes_;
    bool isShutdown_ = false;

    // Transition state
    TransitionPhase transitionPhase_ = TransitionPhase::None;
    TransitionDirection transitionDirection_ = TransitionDirection::None;
    float transitionProgress_ = 0.0f;
    std::string pendingSceneName_;
};
