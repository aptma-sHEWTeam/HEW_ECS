/**
 * @file VideoScene.h
 * @brief 動画再生シーン
 * @author 山内陽
 * @date 2025
 * @version 1.0
 */
#pragma once

#include "pch.h"
#include "scenes/SceneManager.h"
#include "graphics/VideoPlayer.h"
#include "graphics/GfxDevice.h"
#include "app/ServiceLocator.h"
#include "app/DebugLog.h"
#include "input/GamepadSystem.h"
#include "config/ConfigVar.h"
#include "components/UIComponents.h"
#include "graphics/TextSystem.h"
#include "graphics/ImageSystem.h"
#include "systems/UISystem.h"
#include <algorithm>
#include "components/StageComponents.h"
#include "graphics/StageSave.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

inline static ConfigVar<std::string> cfg_VideoPath{"Video.Clear", "FilePath", "Assets/Textures/Still/gameclear.mov", "ゲームクリア時のパス"};
inline static ConfigVar<std::string> cfg_VideoLoopPath{"Video.Clear", "LoopFilePath", "Assets/Textures/Still/gameclear_loop.mov", "ゲームクリア後ループ再生のパス"};
inline static ConfigVar<std::string> cfg_VideoNextScene{"Video.Clear", "NextScene", "World1_StageSelect", "動画再生後の遷移先シーン"};
inline static ConfigVar<bool> cfg_VideoSkipEnabled{"Video.Clear", "SkipEnabled", true, "動画スキップを許可するか"};

/**
 * @class VideoScene
 * @brief 汎用動画再生シーン
 */
class VideoScene : public IScene {
  public:
    VideoScene() = default;
    ~VideoScene() override = default;

    void SetVideoPath(const std::string &path) {
        videoPath_ = path;
    }
    void SetNextScene(const std::string &sceneName) {
        nextSceneName_ = sceneName;
    }
    void SetSkipEnabled(bool enabled) {
        skipEnabled_ = enabled;
    }
    void SetLoopVideoPath(const std::string &path) {
        loopVideoPath_ = path;
        loopVideoPathSet_ = true;
    }

    void SetBgmPath(const std::string &path) {
        bgmPath_ = path;
        bgmPathSet_ = true;
    }

    void OnEnter(World &world) override {
        DEBUGLOG("VideoScene::OnEnter() start");

        SOUND_SYS.StopBGM();

        auto *gfx = ServiceLocator::TryGet<GfxDevice>();
        if (!gfx) {
            DEBUGLOG_ERROR("VideoScene: GfxDevice not found");
            shouldExit_ = true;
            return;
        }

        gfx_ = gfx;
        screenWidth_ = static_cast<float>(gfx->Width());
        screenHeight_ = static_cast<float>(gfx->Height());

        if (videoPath_.empty()) {
            videoPath_ = cfg_VideoPath.Get();
        }
        if (nextSceneName_.empty()) {
            nextSceneName_ = cfg_VideoNextScene.Get();
        }

        if (!player_.Init()) {
            DEBUGLOG_ERROR("VideoScene: VideoPlayer init failed");
            shouldExit_ = true;
            return;
        }

        if (!player_.Open(*gfx, videoPath_.c_str())) {
            DEBUGLOG_ERROR(std::string("VideoScene: Failed to open video: ") + videoPath_);
            shouldExit_ = true;
            return;
        }

        if (!InitializeRendering()) {
            DEBUGLOG_ERROR("VideoScene: Failed to initialize rendering resources");
            shouldExit_ = true;
            return;
        }

        if (!loopVideoPathSet_ && loopVideoPath_.empty()) {
            loopVideoPath_ = cfg_VideoLoopPath.Get();
        }

        if (!textSystem_.Init(*gfx) || !imageSystem_.Init(*gfx)) {
            DEBUGLOG_ERROR("VideoScene: Text/ImageSystem init failed");
        } else {
            CreateUI(world, screenWidth_, screenHeight_);
        }

        UITransform FadeAnimation;
        FadeAnimation.position = {0.0f, 0.0f};
        FadeAnimation.size = {1280.0f, 720.0f};
        FadeAnimation.anchor = {0.0f, 0.0f};
        FadeAnimation.pivot = {0.0f, 0.0f};

        UIImage fade{L"./Assets/Textures/Fade/tex_fade.png"};
        fade.opacity = 1.0f;
        fade.keepAspect = false;
        fade.overlay = true;

        SpriteSheetDesc fadeDesc = SpriteSheetDesc::Grid(
            AnimationConfig::UI::FadeFrames,
            AnimationConfig::UI::FadeCols,
            0.1f,
            /*loop*/ false);
        fadeDesc.playOnStart = false;

        Entity fadeOutAnimation = world.Create()
                                      .With<UITransform>(FadeAnimation)
                                      .With<UIImage>(fade)
                                      .Build();
        AnimationTools::AddSpriteSheet(world, fadeOutAnimation, fadeDesc);

        uiOwnedEntities_.push_back(fadeOutAnimation);
        fadeEntity_ = fadeOutAnimation;

        player_.SetLoop(false);
        player_.Play();
        isPlaying_ = true;
        loopPlaying_ = false;
        isFading = false;

        DEBUGLOG(std::string("VideoScene: Video playback started - ") + videoPath_);

        if (bgmPathSet_) {
            if (!bgmPath_.empty()) {
                SOUND_SYS.PlayBGM(bgmPath_);
            }
        } else {
            SOUND_SYS.PlayBGM(cfg_ClearMP3Pass.Get());
        }
    }

    void OnUpdate(World &world, InputSystem &input, float deltaTime) override {
        if (shouldExit_) {
            TransitionToNextScene(world);
            return;
        }

        /* if () {
        
        }*/

        world.ForEach<UIInteractionSystem>([&](Entity, UIInteractionSystem &sys) {
            if (!sys.input_)
                sys.input_ = &input;
        });

        if (isFading) {
            world.Tick(deltaTime);
            if (auto *anim = world.TryGet<SpriteSheetAnimation>(fadeEntity_)) {
                if (anim->isFinished) {
                    TransitionToNextScene(world);
                }
            }
            return;
        }

        if (skipEnabled_ && CheckExitInput(input)) {
            StartFadeInNormal(world);
            isFading = true;
            return;
        }

        if (isPlaying_) {
            if (!player_.Update(deltaTime)) {
                isPlaying_ = false;
                DEBUGLOG("VideoScene: Video playback finished");
            }
        }

        if (!isPlaying_) {
            if (!loopPlaying_ && !loopVideoPath_.empty()) {
                StartLoopVideo();
                return;
            }
            if (!loopPlaying_) {
                //TransitionToNextScene(world);
                StartFadeInNormal(world);
                isFading = true;
                return;
            }
        }
        // ランク/デスUIフェードイン処理
        UpdateRankFadeIn(world, deltaTime);

        world.Tick(deltaTime);
    }

    void OnRender(World &world) override {
        if (gfx_ && (isPlaying_ || loopPlaying_)) {
            ID3D11ShaderResourceView *srv = player_.GetSRV();
            if (srv) {
                RenderVideoFrame(srv);
            }
        }



        world.ForEach<UIRenderSystem>([&](Entity, UIRenderSystem &sys) {
            sys.Render(world);
        });
    }

    void OnExit(World &world) override {
        DEBUGLOG("VideoScene::OnExit()");

        player_.Stop();
        ShutdownRendering();
        ShutdownUI(world);
        textSystem_.Shutdown();
        imageSystem_.Shutdown();

        SOUND_SYS.StopBGM();
        // Do NOT clear videoPath_ or nextSceneName_ here.
        // They are part of the scene configuration (set in App.h) and should persist.
        
        player_.Close();
        
        isPlaying_ = false;
        shouldExit_ = false;
        loopPlaying_ = false;
        isFading = false;
    }

  private:
    void StartLoopVideo() {
        if (!gfx_)
            return;
        if (loopVideoPath_.empty())
            return;
        player_.Stop();
        if (!player_.Open(*gfx_, loopVideoPath_.c_str())) {
            shouldExit_ = true;
            return;
        }
        player_.SetLoop(true);
        player_.Play();
        isPlaying_ = true;
        loopPlaying_ = true;
        DEBUGLOG(std::string("VideoScene: Loop playback started - ") + loopVideoPath_);
    }

    void CreateUI(World &world, float screenW, float screenH) {
        Entity canvas = world.Create().With<UICanvas>().Build();
        uiOwnedEntities_.push_back(canvas);

        Entity uiRenderSystem = world.Create().With<UIRenderSystem>().Build();
        if (auto *renderSys = world.TryGet<UIRenderSystem>(uiRenderSystem)) {
            renderSys->SetTextSystem(&textSystem_);
            renderSys->SetImageSystem(&imageSystem_);
            renderSys->SetScreenSize(screenW, screenH);
        }
        uiOwnedEntities_.push_back(uiRenderSystem);

        Entity uiInteractionSystem = world.Create().With<UIInteractionSystem>().Build();
        if (auto *interaction = world.TryGet<UIInteractionSystem>(uiInteractionSystem)) {
            interaction->SetScreenSize(screenW, screenH);
            interaction->input_ = nullptr;
        }
        uiOwnedEntities_.push_back(uiInteractionSystem);

        TextSystem::TextFormat hud;
        hud.fontSize = 60.0f;
        hud.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        hud.alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        textSystem_.CreateTextFormat("hud", hud);

        UITransform crossTextTr;
        crossTextTr.position = {120.0f, 650.0f};
        crossTextTr.size = {300.0f, 50.0f};
        crossTextTr.anchor = {0.0f, 0.0f};
        crossTextTr.pivot = {0.0f, 0.0f};

       /* UIText crossText{L"title"};
        crossText.color = {1.0f, 1.0f, 1.0f, 1.0f};
        crossText.formatId = "hud";*/

      /*  Entity crossTextEntity = world.Create().With<UITransform>(crossTextTr).With<UIText>(crossText).Build();
        uiOwnedEntities_.push_back(crossTextEntity);*/

        UITransform crossImgTr;
        crossImgTr.position = {0.0f, 480.0f};
        crossImgTr.size = {400.0f, 400.0f};
        crossImgTr.anchor = {0.0f, 0.0f};
        crossImgTr.pivot = {0.0f, 0.0f};

        UIImage crossImg{L"./Assets/Textures/UI/StageUI/stageui4.png"};
        crossImg.opacity = 1.0f;
        crossImg.keepAspect = true;

        Entity crossImageEntity = world.Create().With<UITransform>(crossImgTr).With<UIImage>(crossImg).Build();
        uiOwnedEntities_.push_back(crossImageEntity);

        // === デス回数 + ランクUI ===
        int deaths = StageSave::GetLastSavedDeaths();
        std::wstring rankStr = StageSave::GetRankW(deaths);

        TextSystem::TextFormat rankFmt;
        rankFmt.fontSize = 72.0f;
        rankFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        rankFmt.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        textSystem_.CreateTextFormat("rank", rankFmt);

        TextSystem::TextFormat deathFmt;
        deathFmt.fontSize = 36.0f;
        deathFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        deathFmt.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        textSystem_.CreateTextFormat("death", deathFmt);

        // ランク表示（右下）
        UITransform rankTr;
        rankTr.position = {screenW - 200.0f, screenH - 180.0f};
        rankTr.size = {180.0f, 90.0f};
        rankTr.anchor = {0.0f, 0.0f};
        rankTr.pivot = {0.0f, 0.0f};

        UIText rankText{L"RANK: " + rankStr};
        rankText.color = {1.0f, 0.85f, 0.0f, 0.0f}; // 初期透明（フェードイン用）
        rankText.formatId = "rank";

        rankEntity_ = world.Create().With<UITransform>(rankTr).With<UIText>(rankText).Build();
        uiOwnedEntities_.push_back(rankEntity_);

        // デス回数表示（ランクの下）
        UITransform deathTr;
        deathTr.position = {screenW - 200.0f, screenH - 90.0f};
        deathTr.size = {180.0f, 50.0f};
        deathTr.anchor = {0.0f, 0.0f};
        deathTr.pivot = {0.0f, 0.0f};

        std::wstring deathStr = L"DEATH: " + std::to_wstring(deaths);
        UIText deathText{deathStr};
        deathText.color = {1.0f, 1.0f, 1.0f, 0.0f}; // 初期透明
        deathText.formatId = "death";

        deathEntity_ = world.Create().With<UITransform>(deathTr).With<UIText>(deathText).Build();
        uiOwnedEntities_.push_back(deathEntity_);

        rankFadeTimer_ = 0.0f;
    }

    void ShutdownUI(World &world) {
        for (const auto &e : uiOwnedEntities_) {
            if (world.IsAlive(e)) {
                world.DestroyEntityWithCause(e, World::Cause::SceneUnload);
            }
        }
        uiOwnedEntities_.clear();
    }

    bool CheckExitInput(InputSystem &input) {
        if (input.GetKeyDown(VK_RETURN) || input.GetKeyDown(VK_SPACE) || input.GetKeyDown(VK_ESCAPE)) {
            return true;
        }

        auto *gamepad = ServiceLocator::TryGet<GamepadSystem>();
        if (gamepad) {
            if (gamepad->GetAnyButtonDown({GamepadSystem::Button_A,
                                           GamepadSystem::Button_Start})) {
                return true;
            }
        }
        return false;
    }

    bool InitializeRendering() {
        if (!gfx_)
            return false;

        const char *vsCode = R"(
            struct VS_INPUT {
                float3 pos : POSITION;
                float2 uv : TEXCOORD;
            };
            struct VS_OUTPUT {
                float4 pos : SV_POSITION;
                float2 uv : TEXCOORD;
            };
            VS_OUTPUT main(VS_INPUT input) {
                VS_OUTPUT output;
                output.pos = float4(input.pos, 1.0);
                output.uv = input.uv;
                return output;
            }
        )";

        const char *psCode = R"(
            Texture2D tex : register(t0);
            SamplerState samp : register(s0);
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 uv : TEXCOORD;
            };
            float4 main(PS_INPUT input) : SV_TARGET {
                return tex.Sample(samp, input.uv);
            }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

        HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
                                "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("VideoScene: Vertex shader compile failed");
            return false;
        }

        hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
                        "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            DEBUGLOG_ERROR("VideoScene: Pixel shader compile failed");
            return false;
        }

        hr = gfx_->Dev()->CreateVertexShader(
            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
            nullptr, &vertexShader_);
        if (FAILED(hr))
            return false;

        hr = gfx_->Dev()->CreatePixelShader(
            psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
            nullptr, &pixelShader_);
        if (FAILED(hr))
            return false;

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}};
        hr = gfx_->Dev()->CreateInputLayout(
            layout, 2,
            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
            &inputLayout_);
        if (FAILED(hr))
            return false;

        struct Vertex {
            float x, y, z;
            float u, v;
        };

        Vertex vertices[] = {
            {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, -1.0f, 0.0f, 1.0f, 1.0f}};

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(vertices);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = vertices;

        hr = gfx_->Dev()->CreateBuffer(&bd, &initData, &vertexBuffer_);
        if (FAILED(hr))
            return false;

        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        hr = gfx_->Dev()->CreateSamplerState(&sd, &samplerState_);
        if (FAILED(hr))
            return false;

        return true;
    }

    void ShutdownRendering() {
        vertexShader_.Reset();
        pixelShader_.Reset();
        inputLayout_.Reset();
        vertexBuffer_.Reset();
        samplerState_.Reset();
    }

    void RenderVideoFrame(ID3D11ShaderResourceView *srv) {
        auto *ctx = gfx_->Ctx();

        ctx->IASetInputLayout(inputLayout_.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(float) * 5;
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);

        ctx->VSSetShader(vertexShader_.Get(), nullptr, 0);
        ctx->PSSetShader(pixelShader_.Get(), nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->PSSetSamplers(0, 1, samplerState_.GetAddressOf());

        ctx->Draw(4, 0);

        ID3D11ShaderResourceView *nullSRV = nullptr;
        ctx->PSSetShaderResources(0, 1, &nullSRV);
    }

    void TransitionToNextScene(World &world) {
        if (auto *mgr = ServiceLocator::TryGet<SceneManager>()) {
            int targetWorld = 1;
            bool hasStageProgress = false;
            world.ForEach<StageProgress>([&](Entity, StageProgress &stats) {
                hasStageProgress = true;
                stats.IsClearBack = true;
                stats.clearedThisStage = false;
                stats.goalTransitioning = false;
                stats.requestAdvance = false;
                targetWorld = std::clamp(stats.worldCount, 1, 4);
                g_LastStageProgress = stats;
            });
            std::string target = "World" + std::to_string(targetWorld) + "_StageSelect";
            if (!nextSceneName_.empty()) {
                if (nextSceneName_ != cfg_VideoNextScene.Get() || !hasStageProgress) {
                    target = nextSceneName_;
                }
            }
            DEBUGLOG(std::string("VideoScene: ChangeScene -> ") + target +
                     " world=" + std::to_string(targetWorld) +
                     " hasStageProgress=" + std::to_string(hasStageProgress));
            mgr->ChangeScene(target.c_str(), world);
        }
    }

    void StartFadeInNormal(World &world) {
        StartSpriteFade(world, fadeEntity_, 1, false);
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

    GfxDevice *gfx_ = nullptr;

    VideoPlayer player_;

    std::string videoPath_;
    std::string nextSceneName_;
    bool skipEnabled_ = true;

    float screenWidth_ = 0.0f;
    float screenHeight_ = 0.0f;

    bool isPlaying_ = false;
    bool shouldExit_ = false;
    bool loopPlaying_ = false;
    bool isFading = false;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;

    TextSystem textSystem_;
    ImageSystem imageSystem_;
    std::vector<Entity> uiOwnedEntities_;

    std::string loopVideoPath_;
    bool loopVideoPathSet_ = false;

    std::string bgmPath_;
    bool bgmPathSet_ = false;

    Entity fadeEntity_;

    // ランクUIフェードイン用
    Entity rankEntity_;
    Entity deathEntity_;
    float rankFadeTimer_ = 0.0f;
    static constexpr float kRankFadeDelay = 1.5f;    // 表示開始までの遅延（秒）
    static constexpr float kRankFadeDuration = 1.0f; // フェードイン時間（秒）

    /// easeOutCubic: 滞らかに減速して完了
    static float EaseOutCubic(float t) {
        float f = 1.0f - t;
        return 1.0f - f * f * f;
    }

    /// ランク/デステキストのフェードイン更新
    void UpdateRankFadeIn(World &world, float dt) {
        rankFadeTimer_ += dt;
        float elapsed = rankFadeTimer_ - kRankFadeDelay;
        if (elapsed < 0.0f) return; // 遅延中

        float t = std::clamp(elapsed / kRankFadeDuration, 0.0f, 1.0f);
        float alpha = EaseOutCubic(t);

        if (auto *rankText = world.TryGet<UIText>(rankEntity_)) {
            rankText->color.w = alpha; // 金色のalpha
        }
        if (auto *deathText = world.TryGet<UIText>(deathEntity_)) {
            deathText->color.w = alpha;
        }
    }
};
