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
        
        // 実際の解像度はgfxから取得するが、UIの配置などは1280x720(論理解像度)基準に行う
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
            // 論理解像度を設定(1280x720)
            textSystem_.SetLogicalSize(1280.0f, 720.0f);
            imageSystem_.SetLogicalSize(1280.0f, 720.0f);
            
            CreateUI(world, 1280.0f, 720.0f);
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
        menuRumbleTimer_ = 0.0f;
        menuRumbleActive_ = false;
        if (auto *pad = ServiceLocator::TryGet<GamepadSystem>()) {
            pad->SetVibration(0.0f, 0.0f);
        }

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
        UpdateMenuRumble(deltaTime);
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

        if (skipEnabled_ && isPlaying_ && CheckExitInput(input)) {
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

        // 動画終了後（ループ中、あるいはループなしの完了後）の待機処理
        if (!isPlaying_) {
            if (!loopVideoPath_.empty()) {
                StartLoopVideo();
                // ループ再生を開始しても、このフレームは抜けない
            }
            // 動画本編が終わった後は、ループ中・非ループ中にかかわらず、ボタン入力で遷移
            if (CheckExitInput(input)) {
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
        StopMenuRumble();

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
        
        player_.Close(); // 前の動画リソースを解放する
        
        if (!player_.Open(*gfx_, loopVideoPath_.c_str())) {
            shouldExit_ = true;
            return;
        }
        player_.SetLoop(false);
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

        // === ランク・デスUI ===
        int pss = StageSave::GetLastSavedPss();
        int currentDeaths = StageSave::GetLastSavedDeaths();
        bool isNewRecord = StageSave::IsNewRecord();
        std::vector<int> topDeaths = StageSave::GetTopDeaths(pss);

        // フォントフォーマット作成
        TextSystem::TextFormat recordFmt;
        recordFmt.fontSize = 80.0f;
        recordFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf"; // またはインパクトのあるフォント
        textSystem_.CreateTextFormat("record", recordFmt);

        TextSystem::TextFormat youFmt;
        youFmt.fontSize = 60.0f;
        youFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        textSystem_.CreateTextFormat("you", youFmt);

        TextSystem::TextFormat numFmt;
        numFmt.fontSize = 90.0f;
        numFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
        textSystem_.CreateTextFormat("num", numFmt);

        TextSystem::TextFormat redFmt;
        redFmt.fontSize = 50.0f;
        redFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        textSystem_.CreateTextFormat("redText", redFmt);

        TextSystem::TextFormat topNumFmt;
        topNumFmt.fontSize = 65.0f;
        topNumFmt.fontFamily = L"ShipporiMincho-Bold.ttf";
        textSystem_.CreateTextFormat("topNum", topNumFmt);

        TextSystem::TextFormat rankFmt;
        rankFmt.fontSize = 35.0f;
        rankFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        textSystem_.CreateTextFormat("rank", rankFmt);

        TextSystem::TextFormat promptFmt;
        promptFmt.fontSize = 40.0f;
        promptFmt.fontFamily = L"Mamelon-5-Hi-Regular.otf";
        promptFmt.alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        textSystem_.CreateTextFormat("prompt", promptFmt);

        // 【1】 New Record!!! （左上、条件付き）
        if (isNewRecord) {
            UITransform nrTr;
            nrTr.position = {30.0f, 30.0f};
            nrTr.size = {600.0f, 100.0f};
            nrTr.anchor = {0.0f, 0.0f};
            nrTr.pivot = {0.0f, 0.0f};
            
            UIText nrText{L"New Record!!!"};
            nrText.color = {1.0f, 1.0f, 0.0f, 0.0f}; // 最初は透明
            nrText.formatId = "record";
            newRecordEntity_ = world.Create().With<UITransform>(nrTr).With<UIText>(nrText).Build();
            uiOwnedEntities_.push_back(newRecordEntity_);
        }

        // 【2】 You (右下やや上) + 今回のデス数 + "death"（下配置）
        float youBaseX = screenW - 350.0f;
        float youBaseY = screenH - 220.0f;
        
        // "You"
        UITransform youTr;
        youTr.position = {youBaseX, youBaseY};
        youTr.size = {150.0f, 80.0f};
        youTr.anchor = {0.0f, 0.0f};
        youTr.pivot = {0.0f, 0.0f};

        UIText youText{L"You"};
        youText.color = {1.0f, 1.0f, 1.0f, 0.0f};
        youText.formatId = "you";
        Entity youEnt = world.Create().With<UITransform>(youTr).With<UIText>(youText).Build();
        uiOwnedEntities_.push_back(youEnt);
        rankDataEntities_.push_back(youEnt);

        // 今回の数値
        UITransform currNumTr;
        currNumTr.position = {youBaseX + 130.0f, youBaseY - 10.0f};
        currNumTr.size = {150.0f, 100.0f};
        currNumTr.anchor = {0.0f, 0.0f};
        currNumTr.pivot = {0.0f, 0.0f};

        UIText currNumText{std::to_wstring(currentDeaths)};
        currNumText.color = {1.0f, 1.0f, 1.0f, 0.0f};
        currNumText.formatId = "num";
        Entity currNumEnt = world.Create().With<UITransform>(currNumTr).With<UIText>(currNumText).Build();
        uiOwnedEntities_.push_back(currNumEnt);
        rankDataEntities_.push_back(currNumEnt);

        // "death"（赤字で少し下に配置）
        UITransform redTr;
        redTr.position = {youBaseX + 180.0f, youBaseY + 70.0f};
        redTr.size = {150.0f, 60.0f};
        redTr.anchor = {0.0f, 0.0f};
        redTr.pivot = {0.0f, 0.0f};

        UIText redText{L"death"};
        redText.color = {1.0f, 0.0f, 0.0f, 0.0f}; // 赤、透明
        redText.formatId = "redText";
        Entity redEnt = world.Create().With<UITransform>(redTr).With<UIText>(redText).Build();
        uiOwnedEntities_.push_back(redEnt);
        rankDataEntities_.push_back(redEnt);

        // 【3】 Top 3 ランキング（画面下部、横並び）
        float rankBaseX = screenW * 0.35f; // 真ん中やや左寄りから開始
        float rankBaseY = screenH - 100.0f;
        float xOffset = 200.0f; // 1st, 2nd, 3rd の間隔

        for (size_t i = 0; i < topDeaths.size() && i < 3; ++i) {
            float curX = rankBaseX + (i * xOffset);
            
            // 数値
            UITransform topNumTr;
            topNumTr.position = {curX, rankBaseY};
            topNumTr.size = {80.0f, 80.0f};
            topNumTr.anchor = {0.0f, 0.0f};
            topNumTr.pivot = {0.0f, 0.0f};

            UIText topNumText{std::to_wstring(topDeaths[i])};
            topNumText.color = {0.0f, 0.0f, 0.0f, 0.0f}; // 黒字（透明）
            topNumText.outlineColor = {1.0f, 1.0f, 1.0f, 0.0f}; // 白いアウトライン
            topNumText.outlineThickness = 2.5f; 
            topNumText.formatId = "topNum";
            Entity topNumEnt = world.Create().With<UITransform>(topNumTr).With<UIText>(topNumText).Build();
            uiOwnedEntities_.push_back(topNumEnt);
            rankDataEntities_.push_back(topNumEnt);

            // "1st" のようなサフィックス
            std::wstring suffix[] = { L"1st", L"2nd", L"3rd" };
            DirectX::XMFLOAT4 colors[] = { 
                {1.0f, 0.84f, 0.0f, 0.0f}, // 金（透明）
                {0.75f, 0.75f, 0.75f, 0.0f}, // 銀（透明）
                {0.8f, 0.5f, 0.2f, 0.0f}  // 銅（透明）
            };

            UITransform sufTr;
            sufTr.position = {curX + 45.0f, rankBaseY + 25.0f}; // 数値のすぐ右隣
            sufTr.size = {60.0f, 40.0f};
            sufTr.anchor = {0.0f, 0.0f};
            sufTr.pivot = {0.0f, 0.0f};
            
            UIText sufText{suffix[i]};
            sufText.color = colors[i];
            sufText.outlineColor = {1.0f, 1.0f, 1.0f, 0.0f}; // 白いアウトライン
            sufText.outlineThickness = 2.0f;
            sufText.formatId = "rank";

            Entity sufEnt = world.Create().With<UITransform>(sufTr).With<UIText>(sufText).Build();
            uiOwnedEntities_.push_back(sufEnt);
            rankDataEntities_.push_back(sufEnt);
        }

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
            TriggerMenuRumble(std::clamp(cfg_ControllerRumbleSubmitStrength.Get(), 0.0f, 1.0f),
                              std::max(0.0f, cfg_ControllerRumbleSubmitDuration.Get()));
            return true;
        }

        auto *gamepad = ServiceLocator::TryGet<GamepadSystem>();
        if (gamepad) {
            if (gamepad->GetAnyButtonDown({GamepadSystem::Button_A,
                                           GamepadSystem::Button_Start})) {
                TriggerMenuRumble(std::clamp(cfg_ControllerRumbleSubmitStrength.Get(), 0.0f, 1.0f),
                                  std::max(0.0f, cfg_ControllerRumbleSubmitDuration.Get()));
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

    void TriggerMenuRumble(float strength, float duration) {
        if (strength <= 0.0f || duration <= 0.0f) {
            return;
        }
        auto *pad = ServiceLocator::TryGet<GamepadSystem>();
        if (!pad) {
            return;
        }
        pad->SetVibration(strength, strength);
        menuRumbleTimer_ = std::max(menuRumbleTimer_, duration);
        menuRumbleActive_ = true;
    }

    void UpdateMenuRumble(float dt) {
        if (!menuRumbleActive_) {
            return;
        }
        menuRumbleTimer_ = std::max(0.0f, menuRumbleTimer_ - std::max(0.0f, dt));
        if (menuRumbleTimer_ > 0.0f) {
            return;
        }
        StopMenuRumble();
    }

    void StopMenuRumble() {
        menuRumbleTimer_ = 0.0f;
        if (menuRumbleActive_) {
            if (auto *pad = ServiceLocator::TryGet<GamepadSystem>()) {
                pad->SetVibration(0.0f, 0.0f);
            }
        }
        menuRumbleActive_ = false;
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
    float menuRumbleTimer_ = 0.0f;
    bool menuRumbleActive_ = false;

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

    // ランクUIフェードイン・管理用
    Entity newRecordEntity_; // 「New Record!!!」表示エンティティ
    std::vector<Entity> rankDataEntities_; // デス数、You表記などのエンティティリスト
    
    float rankFadeTimer_ = 0.0f;
    static constexpr float kRankFadeDelay = 1.5f;    // 表示開始までの遅延（秒）
    static constexpr float kRankFadeDuration = 1.0f; // フェードイン時間（秒）

    /// easeOutCubic: 滞らかに減速して完了
    static float EaseOutCubic(float t) {
        float f = 1.0f - t;
        return 1.0f - f * f * f;
    }

    /// @brief ランク/デステキストのフェードイン処理
    /// @param world ECSワールド
    /// @param dt デルタタイム
    void UpdateRankFadeIn(World &world, float dt) {
        rankFadeTimer_ += dt;
        float elapsed = rankFadeTimer_ - kRankFadeDelay;
        if (elapsed < 0.0f) return; // 遅延中はスキップ

        float t = std::clamp(elapsed / kRankFadeDuration, 0.0f, 1.0f);
        float alpha = EaseOutCubic(t);

        // 「New Record!!!」のフェードイン（点滅や演出は別途考慮）
        if (world.IsAlive(newRecordEntity_)) {
            if (auto *nrText = world.TryGet<UIText>(newRecordEntity_)) {
                nrText->color.w = alpha;
            }
        }

        // You や 1st, 2nd, 数値テキストのフェードイン
        for (Entity e : rankDataEntities_) {
            if (world.IsAlive(e)) {
                if (auto *uText = world.TryGet<UIText>(e)) {
                    // RGBカラーは維持し、Alpha値(W)だけ更新する
                    uText->color.w = alpha; 
                    uText->outlineColor.w = alpha; // アウトラインの透明度もフェードインさせる
                }
            }
        }
    }
};
