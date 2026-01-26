/**
 * @file SceneManager.h
 * @brief シーン管理ユーティリティ。
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
 * @brief シーン変更時のスライド遷移方向
 */
enum class TransitionDirection {
    None,   // 遷移なし（即時）
    Left,   // 左へスライド（前のシーンへ）
    Right,  // 右へスライド（次のシーンへ）
    Forward // 前進 / ズームイン（より深いシーンへ）
};

/**
 * @enum TransitionPhase
 * @brief 遷移アニメーションの現在のフェーズ
 */
enum class TransitionPhase {
    None,     // 遷移なし
    SlideOut, // 現在のシーンがスライドアウト
    SlideIn   // 新しいシーンがスライドイン
};

/**
 * @class IScene
 * @brief すべてのシーンが実装する必要のあるインターフェース。
 */
class IScene {
  public:
    virtual ~IScene() = default;

    virtual void OnEnter(World &world) = 0;
    virtual void OnUpdate(World &world, InputSystem &input, float deltaTime) = 0;
    virtual void OnRender(World &world) {} // デフォルト空実装
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
 * @brief シーンインスタンスを所有し、遷移をオーケストレート。
 */
class SceneManager {
  public:
    // 遷移設定
    static constexpr float TRANSITION_DURATION = 0.8f; // スライドアウト/インの合計時間（演出確認用に長め）
    static constexpr float SCREEN_WIDTH_FACTOR = 1.2f; // スライドする距離（1.0 = 画面幅）
    /**
     * @brief マネージャを初期化し、最初のシーンをアクティブ化。
     * @param startSceneName アクティブ化するシーンの名前。nullptrの場合あり。
     * @param world ECSワールド参照。
     */
    void Init(const char *startSceneName, World &world) {
        isShutdown_ = false;
        currentScene_ = FindScene(startSceneName);
        if (!currentScene_) {
            DEBUGLOG_WARNING("SceneManager::Init() - start scene not found");
            return;
        }
        SOUND_SYS.StopBGM();
        currentScene_->OnEnter(world);
    }

    /**
     * @brief シーンインスタンスを登録。
     * @param name シーン識別子。
     * @param scene シーンインスタンス（所有権が移転）。
     */
    void RegisterScene(const char *name, std::unique_ptr<IScene> scene) {
        if (!name || !scene) {
            DEBUGLOG_WARNING("SceneManager::RegisterScene() - invalid input");
            return;
        }
        scenes_[name] = std::move(scene);
    }

    /**
     * @brief アクティブシーンを更新し、リクエストされた場合に遷移を実行。
     */
    void Update(World &world, InputSystem &input, float deltaTime) {
        if (!currentScene_) {
            return;
        }

        // 遷移アニメーションを処理
        if (transitionPhase_ != TransitionPhase::None) {
            UpdateTransition(world, deltaTime);
            // 遷移中はシーンを更新するが、遷移状態チェックにより入力は効果的にブロックされる
            // シーン内のシーン
            return;
        }

        currentScene_->OnUpdate(world, input, deltaTime);

        if (currentScene_->ShouldChangeScene()) {
            ChangeScene(currentScene_->GetNextScene(), world);
        }
    }

    /**
     * @brief アクティブシーンをレンダリング。
     */
    void Render(World &world) {
        if (!currentScene_) {
            return;
        }
        
        currentScene_->OnRender(world);
    }

    /**
     * @brief 異なるシーンへ遷移。
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
     * @brief スライドアニメーションで異なるシーンへ遷移。
     * @param sceneName ターゲットシーン名
     * @param world ECSワールド参照
     * @param direction スライド方向（Left = 前のシーンへ、Right = 次のシーンへ）
     */
    void ChangeSceneWithTransition(const char *sceneName, World &world, TransitionDirection direction) {
        if (!sceneName || direction == TransitionDirection::None) {
            // 即時遷移にフォールバック
            ChangeScene(sceneName, world);
            return;
        }

        IScene *nextScene = FindScene(sceneName);
        if (!nextScene || nextScene == currentScene_) {
            return;
        }

        // 遷移状態を保存
        pendingSceneName_ = sceneName;
        transitionDirection_ = direction;
        transitionPhase_ = TransitionPhase::SlideOut;
        transitionProgress_ = 0.0f;

        DEBUGLOG_CATEGORY(DebugLog::Category::Scene,
                          std::string("Starting slide transition to: ") + sceneName +
                              " direction: " + (direction == TransitionDirection::Right ? "Right" : "Left"));
    }

    /**
     * @brief レンダリング用の現在の遷移オフセットを取得。
     * @return 正規化オフセット（-1.0 to 1.0）負 = 左、正 = 右
     * 
     * シーンはこれを画面幅で乗算してすべてのレンダリングをオフセットすべき。
     */
    float GetTransitionOffset() const {
        if (transitionPhase_ == TransitionPhase::None || transitionDirection_ == TransitionDirection::Forward) {
            return 0.0f;
        }

        // スムーズなアニメーションのためにイージングを適用
        float easedProgress = EaseInOutCubic(transitionProgress_);

        // 方向乗数: Right はコンテンツが右へ移動（正）、Left は左へ（負）
        float dirMult = (transitionDirection_ == TransitionDirection::Right) ? 1.0f : -1.0f;

        if (transitionPhase_ == TransitionPhase::SlideOut) {
            // スライドアウト: 0 -> フルオフセット方向へ
            return dirMult * easedProgress * SCREEN_WIDTH_FACTOR;
        } else {
            // スライドイン: 反対方向からのフルオフセット -> 0
            return -dirMult * (1.0f - easedProgress) * SCREEN_WIDTH_FACTOR;
        }
    }

    /**
     * @brief 遷移が現在進行中かどうかをチェック。
     */
    bool IsTransitioning() const {
        return transitionPhase_ != TransitionPhase::None;
    }

    /**
     * @brief 現在の遷移フェーズを取得。
     */
    TransitionPhase GetTransitionPhase() const {
        return transitionPhase_;
    }

    /**
     * @brief 現在の遷移方向を取得。
     */
    TransitionDirection GetTransitionDirection() const {
        return transitionDirection_;
    }

    /**
     * @brief 現在の遷移進捗（0.0 to 1.0）を取得。
     */
    float GetTransitionProgress() const {
        return transitionProgress_;
    }

    /**
     * @brief 現在アクティブなシーンへのアクセサ。
     */
    IScene *GetCurrentScene() const {
        return currentScene_;
    }

    /**
     * @brief デストラクタはサニティログを実行。
     */
    ~SceneManager() {
        DEBUGLOG("SceneManager::~SceneManager()");
        if (!isShutdown_) {
            DEBUGLOG_WARNING("SceneManager was destroyed without Shutdown().");
        }
    }

    /**
     * @brief 明示的シャットダウン。複数回呼び出し可能。
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
     * @brief 遷移アニメーション状態を更新。
     */
    void UpdateTransition(World &world, float deltaTime) {
        // シーン切り替え直後はロード時間による長いdeltaTimeが発生する可能性があるため、
        // 1フレームだけアニメーション進行をスキップする
        if (justSwitchedScene_) {
            justSwitchedScene_ = false;
            return;
        }

        transitionProgress_ += deltaTime / TRANSITION_DURATION;

        if (transitionProgress_ >= 1.0f) {
            transitionProgress_ = 1.0f;

            if (transitionPhase_ == TransitionPhase::SlideOut) {
                // スライドアウト完了、実際のシーン変更を実行
                PerformSceneSwitch(world);

                // スライドインフェーズを開始
                transitionPhase_ = TransitionPhase::SlideIn;
                transitionProgress_ = 0.0f;

                // 次のフレームのdeltaTime（ロード時間含む）を無視するためにフラグを立てる
                justSwitchedScene_ = true;
            } else {
                // スライドイン完了、遷移終了
                transitionPhase_ = TransitionPhase::None;
                transitionDirection_ = TransitionDirection::None;
                pendingSceneName_.clear();

                DEBUGLOG_CATEGORY(DebugLog::Category::Scene, "Slide transition complete");
            }
        }
    }

    /**
     * @brief 遷移中に行われる実際のシーン切り替え。
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
     * @brief アニメーションのためのキュービックイーズインアウト関数。
     */
    static float EaseInOutCubic(float t) {
        if (t < 0.5f) {
            return 4.0f * t * t * t;
        }
        float p = 2.0f * t - 2.0f;
        return 0.5 * p * p * p + 1.0f;
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

    // 遷移状態
    TransitionPhase transitionPhase_ = TransitionPhase::None;
    TransitionDirection transitionDirection_ = TransitionDirection::None;
    float transitionProgress_ = 0.0f;
    std::string pendingSceneName_;
    bool justSwitchedScene_ = false;
};
