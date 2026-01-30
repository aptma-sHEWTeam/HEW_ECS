/**
 * @file InitScene.h
 * @brief 初期化シーン（アセットロードと進捗表示）
 * @author AI Assistant
 * @date 2026/01/30
 */
#pragma once
#include "scenes/SceneManager.h"
#include "app/ResourceManager.h"
#include "app/ServiceLocator.h"
#include "graphics/TextureManager.h"
#include "graphics/Effect.h"
#include "systems/SoundSystem.h"
#include "components/UIComponents.h"
#include "components/UIImageComponents.h"
#include "systems/UISystem.h"
#include "scenes/StageConfig.h"
#include "animation/AnimationConfig.h"
#include "animation/AnimationTools.h"
#include <vector>
#include <functional>
#include <string>

class InitScene : public IScene {
public:
    void OnEnter(World& world) override {
        DEBUGLOG("InitScene::OnEnter()");

        // システム初期化
        if (!InitSystems()) return;

        // UIシステムのセットアップ
        SetupUI(world);

        // ロードタスクの登録
        RegisterTasks();

        totalTasks_ = static_cast<int>(loadingTasks_.size());
        completedTasks_ = 0;
        
        // 最初のフレームは描画のみ行うためにタスク実行しない
        isFirstFrame_ = true;
    }

    void OnUpdate(World& world, InputSystem& input, float deltaTime) override {
        // UIシステムの更新（描画のため）
        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem& sys) {
            if (!sys.input_) sys.input_ = &input;
        });

        if (isFading_) {
            world.Tick(deltaTime);
            bool fadeFinished = false;
            if (auto* anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_)) {
                if (anim->isFinished && !anim->isPlaying) {
                    fadeFinished = true;
                }
            } else {
                fadeFinished = true;
            }
            if (fadeFinished) {
                if (auto* manager = ServiceLocator::TryGet<SceneManager>()) {
                    manager->ChangeScene("Title", world);
                }
            }
            return;
        }

        // 最初のフレームはスキップ（"Loading..." を描画させるため）
        if (isFirstFrame_) {
            isFirstFrame_ = false;
            return;
        }

        // 10%ごとの待機処理
        if (isInStepWait_) {
            stepWaitTimer_ += deltaTime;
            
            // 待機中もアニメーションは更新
            animationTimer_ += deltaTime;
            UpdateProgressUI(world);

            if (stepWaitTimer_ < 0.2f) {
                return;
            }
            // 待機終了
            isInStepWait_ = false;
            stepWaitTimer_ = 0.0f;
        }

        // 通常時アニメーション更新
        animationTimer_ += deltaTime;

        // タスク実行（1フレームに1つ）
        // ※必要に応じて複数実行してロード速度を調整可
        if (!loadingTasks_.empty()) {
            auto& task = loadingTasks_.front();
            task();
            loadingTasks_.erase(loadingTasks_.begin());
            completedTasks_++;

            // 進捗率チェック (10%刻みでウェイトを入れる)
            if (totalTasks_ > 0) {
                int currentDecile = (completedTasks_ * 10) / totalTasks_;
                if (currentDecile > lastDecile_) {
                    isInStepWait_ = true;
                    lastDecile_ = currentDecile;
                }
            }
        }

        // 進捗率更新
        UpdateProgressUI(world);

        // 全タスク完了したら遷移
        if (loadingTasks_.empty()) {
            // 少し待機してから遷移（100%を見せるため）
            waitTimer_ += deltaTime;
            if (waitTimer_ > 0.5f) {
                if (!isFading_) {
                    StartSpriteFade(world, fadeEntity_, 1, false);
                    isFading_ = true;
                }
            }
        }
    }

    void OnRender(World& world) override {
        // UI描画
        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            // MeshRenderer renderer; // InitSceneでは3D描画しないなら不要おｋも？ UIRenderSystemの実装次第。
            // Title.hではローカル変数のMeshRendererを作っているが、UIRenderSystem::Render内で使われるかは不明。
            // しかしTitle.hを真似るのが安全。
            sys.Render(world);
        });
    }

    void OnExit(World& world) override {
        DEBUGLOG("InitScene::OnExit()");
        
        // UIエンティティのクリーンアップ
        for (auto e : uiEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntity(e);
            }
        }
        uiEntities_.clear();

        textSystem_.Shutdown();
        imageSystem_.Shutdown();
    }

private:
    std::vector<std::function<void()>> loadingTasks_;
    int totalTasks_ = 0;
    int completedTasks_ = 0;
    bool isFirstFrame_ = true;

    float waitTimer_ = 0.0f;
    
    // 演出用ウェイト変数
    float stepWaitTimer_ = 0.0f;
    bool isInStepWait_ = false;
    int lastDecile_ = 0;
    
    // アニメーション用
    float animationTimer_ = 0.0f;
    TextureManager::TextureHandle hNormal_ = TextureManager::INVALID_TEXTURE;
    TextureManager::TextureHandle hBoost_ = TextureManager::INVALID_TEXTURE;

    std::vector<Entity> uiEntities_;
    Entity progressTextEntity_;
    Entity astronautEntity_;
    Entity uiSystemEntity_;
    Entity fadeEntity_;

    TextSystem textSystem_{};
    ImageSystem imageSystem_{};
    bool isFading_ = false;

    bool InitSystems() {
        auto* gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) return false;

        if (!textSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("InitScene: TextSystem init failed");
            return false;
        }
        if (!imageSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("InitScene: ImageSystem init failed");
            return false;
        }
        return true;
    }


    // 宇宙飛行士UI設定
    // 宇宙飛行士UI設定
    // cfg_AstronautTexturePath is deprecated
    inline static ConfigVar<float> cfg_AstronautRadius{"Init.Astronaut", "Radius", 300.0f, "ロード画面: 宇宙飛行士 回転半径"};
    inline static ConfigVar<float> cfg_AstronautScale{"Init.Astronaut", "Scale", 1.7f, "ロード画面: 宇宙飛行士 スケール"};

    void SetupUI(World& world) {
        auto* gfx = ServiceLocator::TryGet<GfxDevice>();
        float width = gfx ? static_cast<float>(gfx->Width()) : 1280.0f;

        float height = gfx ? static_cast<float>(gfx->Height()) : 720.0f;

        // テキストフォーマット作成
        TextSystem::TextFormat fmt;
        fmt.fontSize = 40.0f;
        fmt.fontFamily = L"Meiryo";
        fmt.alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
        textSystem_.CreateTextFormat("loading", fmt);

        // UIRenderSystem エンティティ作成
        uiSystemEntity_ = world.Create().With<UIRenderSystem>().Build();
        if (auto* sys = world.TryGet<UIRenderSystem>(uiSystemEntity_)) {
            sys->SetTextSystem(&textSystem_);
            sys->SetImageSystem(&imageSystem_);
            sys->SetScreenSize(width, height);
        }
        uiEntities_.push_back(uiSystemEntity_);

        // Canvas作成
        Entity canvas = world.Create().With<UICanvas>().Build();
        uiEntities_.push_back(canvas);
        if (auto* sys = world.TryGet<UIRenderSystem>(uiSystemEntity_)) {
            sys->SetTextSystem(&textSystem_);
            sys->SetImageSystem(&imageSystem_);
            sys->SetScreenSize(width, height);
        }
        uiEntities_.push_back(uiSystemEntity_);

        // 背景（黒）
        {
            UITransform tf;
            tf.position = {0.0f, 0.0f};
            tf.size = {width, height};
            tf.anchor = {0.0f, 0.0f};
            tf.pivot = {0.0f, 0.0f};
            UIPanel panel;
            panel.color = {0.0f, 0.0f, 0.0f, 1.0f};
            panel.drawBeforeImages = true;

            Entity e = world.Create().With<UITransform>(tf).With<UIPanel>(panel).Build();
            uiEntities_.push_back(e);
        }

        // 宇宙飛行士画像プリロード
        TextureManager& tm = ServiceLocator::Get<TextureManager>();
        hNormal_ = tm.LoadFromFile("Assets/Textures/Title/Loading_PlayerNormal.png");
        hBoost_ = tm.LoadFromFile("Assets/Textures/Title/Loading_PlayerBoost.png");

        // 宇宙飛行士作成
        CreateAstronaut(world, width, height);

        // "NOW LOADING" テキスト
        {
            UITransform tf;
            tf.position = {width - 50.0f, height - 50.0f};
            tf.anchor = {0.0f, 0.0f};
            tf.pivot = {1.0f, 1.0f}; // 右下基準
            tf.size = {600.0f, 60.0f};

            UIText text;
            text.text = L"NOW LOADING... 0%";
            text.fontSize = 40.0f;
            text.color = {1.0f, 1.0f, 1.0f, 1.0f};
            text.formatId = "loading";

            Entity e = world.Create().With<UITransform>(tf).With<UIText>(text).Build();
            progressTextEntity_ = e;
            uiEntities_.push_back(e);
        }

        UITransform fadeTransform;
        fadeTransform.position = {0.0f, 0.0f};
        fadeTransform.size = {width, height};
        fadeTransform.anchor = {0.0f, 0.0f};
        fadeTransform.pivot = {0.0f, 0.0f};

        UIImage fade{L"./Assets/Textures/Fade/tex_fade.png"};
        fade.opacity = 1.0f;
        fade.keepAspect = false;
        fade.overlay = true;

        SpriteSheetDesc fadeDesc = SpriteSheetDesc::Grid(
            AnimationConfig::UI::FadeFrames,
            AnimationConfig::UI::FadeCols,
            AnimationConfig::UI::FadeFrameTime,
            /*loop*/ false);
        fadeDesc.playOnStart = false;

        Entity fadeEntity = world.Create()
                             .With<UITransform>(fadeTransform)
                             .With<UIImage>(fade)
                             .Build();
        AnimationTools::AddSpriteSheet(world, fadeEntity, fadeDesc);
        fadeEntity_ = fadeEntity;
        uiEntities_.push_back(fadeEntity_);
    }

    void CreateAstronaut(World& world, float screenW, float screenH) {


        UITransform tf;
        tf.anchor = {0.0f, 0.0f};
        tf.pivot = {0.5f, 0.5f};
        
        // 初期位置をアニメーション開始位置と合わせる (画面右外)
        tf.position = {screenW + 300.0f, screenH * 0.7f}; 

        float baseSize = 256.0f;
        tf.size = {baseSize * cfg_AstronautScale.Get(), baseSize * cfg_AstronautScale.Get()};

        // std::wstring path = Utf8toWide(cfg_AstronautTexturePath.Get());
        
        // ダミーパスまたは空文字で初期化し、Handleで描画
        UIImage img{L"Assets/Textures/Title/Loading_PlayerNormal.png"};
        img.textureHandle = hNormal_; // ハンドル優先
        img.keepAspect = true;

        astronautEntity_ = world.Create()
            .With<UITransform>(tf)
            .With<UIImage>(img)
            .Build();
        uiEntities_.push_back(astronautEntity_);
    }

    void UpdateAstronaut(World& world, float progress) {
        if (!world.IsAlive(astronautEntity_)) return;

        auto* tf = world.TryGet<UITransform>(astronautEntity_);
        if (tf) {
            auto* gfx = ServiceLocator::TryGet<GfxDevice>();
            float screenW = gfx ? static_cast<float>(gfx->Width()) : 1280.0f;
            float screenH = gfx ? static_cast<float>(gfx->Height()) : 720.0f;

            // 左から右へ (0% -> 100%)
            // 右から左へ (0% -> 100%)
            float startX = screenW + 300.0f;
            float endX = -300.0f;
            float currentX = startX + (endX - startX) * progress;
            
            // Y座標 (基本位置 + ふわふわ)
            float baseY = screenH * 0.7f;
            float yOffset = std::sin(animationTimer_ * 5.0f) * 20.0f;
            float currentY = baseY + yOffset;

            tf->position = {currentX, currentY};
            
            tf->rotation = 0.0f;

            bool isRising = std::cos(animationTimer_ * 5.0f) > 0.0f;
            
            auto* img = world.TryGet<UIImage>(astronautEntity_);
            if (img) {
                img->textureHandle = isRising ? hNormal_ : hBoost_;
            }

            // スケール更新
            float startScale = 0.7f;
            float endScale = 0.7f;
            float currentScale = startScale + (endScale - startScale) * progress;

            float baseSize = 256.0f; 
            tf->size = {baseSize * currentScale, baseSize * currentScale};
        }
    }

    void UpdateProgressUI(World& world) {
        float progress = 0.0f;
        if (totalTasks_ > 0) {
            progress = static_cast<float>(completedTasks_) / static_cast<float>(totalTasks_);
        }
        if (loadingTasks_.empty()) progress = 1.0f;

        // テキスト更新
        if (world.IsAlive(progressTextEntity_)) {
            auto* uiText = world.TryGet<UIText>(progressTextEntity_);
            if (uiText) {
                int percent = static_cast<int>(progress * 100);
                uiText->text = L"NOW LOADING... " + std::to_wstring(percent) + L"%";
            }
        }

        // 宇宙飛行士更新
        UpdateAstronaut(world, progress);
    }


    void RegisterTasks() {
        // 1. テクスチャロード
        // Skybox
        AddTask([this]() {
            auto& tm = ServiceLocator::Get<TextureManager>();
            tm.LoadFromFile("Assets/Textures/Skybox/Sky_Box.png");
        });
        // Fade
        AddTask([this]() {
            auto& tm = ServiceLocator::Get<TextureManager>();
            tm.LoadFromFile("./Assets/Textures/Fade/tex_fade.png");
        });

        // 2. モデルロード
        // Player
        AddTask([this]() {
            auto& rm = ServiceLocator::Get<ResourceManager>();
            rm.GetModel(AnimationConfig::Paths::PlayerModel);
        });
        // Player Animations
        AddTask([this]() {
            auto& rm = ServiceLocator::Get<ResourceManager>(); // ModelLoader内部でキャッシュされないならResourceManager経由であるべきだが、AnimationはResourceManagerにない？
            // AnimationLoaderはModelLoader::LoadAnimationを使う。これのキャッシュ機構は現状ないかもしれないが、
            // ファイルIOのコストをここで払う。
            ModelLoader::LoadAnimation(AnimationConfig::Paths::PlayerAnimTitle);
        });
        
        // Stage Objects
        AddTask([this]() {
            auto& rm = ServiceLocator::Get<ResourceManager>();
            rm.GetModel("Assets/Models/StageObj/Window/window.fbx");
        });
        AddTask([this]() {
            auto& rm = ServiceLocator::Get<ResourceManager>();
            rm.GetModel("Assets/Models/StageObj/Wall/obj_wall.fbx");
        });
        AddTask([this]() {
            auto& rm = ServiceLocator::Get<ResourceManager>();
            rm.GetModel("Assets/Textures/Skybox/skybox.fbx");
        });

        // 3. エフェクトロード
        // 定義済みエフェクトを一括取得して個別タスク化
        auto& em = EffekseerManager::GetInstance();
        const auto& effects = em.GetEffectDefs();
        
        struct EffectDef { std::string name; std::string path; };
        std::vector<EffectDef> effectList = {
            {"Goal",           "Assets/Effect/Goal/efe_goal.efkefc"},
            {"WarpIn",         "Assets/Effect/Warp/warpin_effect.efkefc"},
            {"WarpOut",        "Assets/Effect/Warp/warpout_effect.efkefc"},
            {"DashBoard",      "Assets/Effect/SpeedUp/efe_SpeedUp2.efkefc"},
            {"SpeedUp",        "Assets/Effect/SpeedUp/efe_SpeedUp.efkefc"},
            {"FireFirst",      "Assets/Effect/Fire/fire1.efkefc"},
            {"FireSecond",     "Assets/Effect/Fire/fire_2.efkefc"},
            {"FireThird",      "Assets/Effect/Fire/fire_3.efkefc"},
            {"StarSmall",      "Assets/Effect/Star/Star_Effects_Small.efkefc"},
            {"StarMedium",     "Assets/Effect/Star/Star_Effects_Medium.efkefc"},
            {"StarBig",        "Assets/Effect/Star/Star_Effects_Big.efkefc"},
        };

        for (const auto& def : effectList) {
            AddTask([def]() {
                EffekseerManager::GetInstance().LoadEffect(def.name, def.path);
            });
        }

        // 4. サウンドロード (SoundSystemの実装依存だが、PlaySEでロードされるならここでの事前ロードは難しいかも)
        // SoundSystem::LoadSound は private ではないが、ハッシュマップm_soundMapに格納される。
        // しかしパス指定で再生するときはLoadSoundを呼ぶか？
        // SoundSystem::PlaySE(path) -> m_soundMap確認 -> なければLoad -> 再生
        // なので、ここで LoadSound を呼んでおけばキャッシュされるはず。
        // ※Title.h からパスを取得したいが、private/inline static 変数。
        // ここではハードコードするか、Title.h の変数を public にする。
        // 一旦代表的なものをロード。
        AddTask([this]() {
            SOUND_SYS.LoadSound("Assets/Sounds/SE/System/se_select.mp3");
        });
        AddTask([this]() {
            SOUND_SYS.LoadSound("Assets/Sounds/SE/System/se_enter.mp3");
        });
    }

    void AddTask(std::function<void()> task) {
        loadingTasks_.push_back(task);
    }

    void StartSpriteFade(World &world, Entity target, int direction, bool forceOpaque) {
        if (!world.IsAlive(target))
            return;
        AnimationTools::PlaySpriteSheet(world, target, direction, /*loop*/ false, /*reset*/ true);
        if (auto *img = world.TryGet<UIImage>(target)) {
            img->opacity = 1.0f;
        }
        if (auto *anim = world.TryGet<SpriteSheetAnimation>(target)) {
            anim->isFinished = false;
        }
    }
};
