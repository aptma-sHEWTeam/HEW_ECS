/**
 * @file App.h
 * @brief ミニゲームのメインアプリケーションクラス
 * @author 山内 陽
 * @date 2025
 * @version 5.1
 */
#pragma once
// ========================================================
// App.h - ミニゲームのアプリケーション
// ========================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <DirectXMath.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <thread>
#include <filesystem>
#include <fstream>

#include "app/BuildConfig.h"

// DirectX11 & ECS システム
#include "graphics/GfxDevice.h"
#include "graphics/RenderSystem.h"
#include "ecs/World.h"
#include "graphics/Camera.h"
#include "input/InputSystem.h"
#include "graphics/TextureManager.h"
#if ENABLE_DEBUG_VISUALS
#include "graphics/DebugDraw.h"
#endif
#include "app/ResourceManager.h"
#include "app/ServiceLocator.h"
#include "config/ConfigManager.h"
#include "config/ConfigVar.h"

#ifdef _DEBUG
#include "app/DebugLog.h"
#endif

// コンポーネント
#include "components/Transform.h"
#include "components/MeshRenderer.h"

// ゲームシステム
#include "scenes/SceneManager.h"
#include "scenes/Game.h"
#include "scenes/StageSelect.h"

/**
 * @struct App
 * @brief ミニゲームのメインアプリケーションクラス
 * @details
 * ウィンドウ作成、DirectX11の初期化、ECSワールドの管理、
 * メインループの実行など、アプリケーション全体のライフサイクルを管理します。
 */
struct App {
    // Windows関連
    HWND hwnd_ = nullptr; ///< メインウィンドウのハンドル
    bool isWindowFocused_ = true; ///< ウィンドウがフォーカスされているかどうか

    // DirectX11システム
    GfxDevice gfx_; ///< グラフィックスデバイス
    RenderSystem renderer_; ///< 描画システム
    TextureManager texManager_; ///< テクスチャ管理
    ResourceManager resManager_; ///< リソース管理

    // ECSシステム
    World world_; ///< ECSワールド
    Camera camera_; ///< カメラ
    InputSystem input_; ///< 入力システム
    GamepadSystem gamepad_; ///< ゲームパッド入力システム

    // シーン管理
    SceneManager sceneManager_; ///< シーンマネージャー

#if ENABLE_DEBUG_VISUALS
    DebugDraw debugDraw_; ///< デバッグ描画用
#endif

    // Camera設定（TOMLで編集可能）
    inline static ConfigVar<float> cfg_CamFovDeg{"Camera.Base", "FovDegrees", 45.0f};
    inline static ConfigVar<float> cfg_CamNear{"Camera.Base", "Near", 0.1f};
    inline static ConfigVar<float> cfg_CamFar{"Camera.Base", "Far", 100.0f};
    inline static ConfigVar<float> cfg_CamPosX{"Camera.Base", "PosX", 0.0f};
    inline static ConfigVar<float> cfg_CamPosY{"Camera.Base", "PosY", 20.0f};
    inline static ConfigVar<float> cfg_CamPosZ{"Camera.Base", "PosZ", -5.0f};
    inline static ConfigVar<float> cfg_CamTargetX{"Camera.Base", "TargetX", 0.0f};
    inline static ConfigVar<float> cfg_CamTargetY{"Camera.Base", "TargetY", 0.0f};
    inline static ConfigVar<float> cfg_CamTargetZ{"Camera.Base", "TargetZ", -0.2f};
    inline static ConfigVar<float> cfg_CamUpX{"Camera.Base", "UpX", 0.0f};
    inline static ConfigVar<float> cfg_CamUpY{"Camera.Base", "UpY", 0.0f};
    inline static ConfigVar<float> cfg_CamUpZ{"Camera.Base", "UpZ", 1.0f};

    void InitializeGame() {
        DEBUGLOG("InitializeGame() begin");

        auto gameScene = std::make_unique<GameScene>();
        // App側のカメラ設定（TOML）をゲームシーンに引き渡す
        const float fovRad = DirectX::XMConvertToRadians(cfg_CamFovDeg.Get());
        const float nearZ = cfg_CamNear.Get();
        const float farZ = cfg_CamFar.Get();
        const DirectX::XMFLOAT3 camPos{ cfg_CamPosX.Get(), cfg_CamPosY.Get(), cfg_CamPosZ.Get() };
        const DirectX::XMFLOAT3 camTarget{ cfg_CamTargetX.Get(), cfg_CamTargetY.Get(), cfg_CamTargetZ.Get() };
        const DirectX::XMFLOAT3 camUp{ cfg_CamUpX.Get(), cfg_CamUpY.Get(), cfg_CamUpZ.Get() };
        gameScene->ConfigureBaseCamera(camPos, camTarget, camUp, fovRad, nearZ, farZ);
        DEBUGLOG("GameScene instance created");

        sceneManager_.RegisterScene("Game", std::move(gameScene));
        DEBUGLOG("GameScene registered to SceneManager");

        auto stageslectScene = std::make_unique<StageSlectScene>();
        DEBUGLOG("StageSelectScene instance created");

        sceneManager_.RegisterScene("StageSelect", std::move(stageslectScene));
        DEBUGLOG("StageSelectScene registered to SceneManager");

        sceneManager_.Init("StageSelect", world_);
        DEBUGLOG("SceneManager initialised with Game scene");

        DEBUGLOG("InitializeGame() complete");
    }

    // ========================================================
    // パフォーマンス計測
    // ========================================================
    struct FrameMetrics {
        float updateTime = 0.0f;  ///< Update時間（秒）
        float renderTime = 0.0f;  ///< Render時間（秒）
        float presentTime = 0.0f; ///< Present時間（秒）
        float totalTime = 0.0f;   ///< 合計フレーム時間（秒）
    };

    FrameMetrics currentMetrics_;       ///< 現在のフレームメトリクス

    // 詳細統計用（最大1000フレーム分のサンプル）
    std::vector<float> frameTotalSamples_; ///< フレーム合計時間のサンプル
    std::vector<float> updateSamples_;     ///< Update時間のサンプル
    std::vector<float> renderSamples_;     ///< Render時間のサンプル
    std::vector<float> presentSamples_;    ///< Present時間のサンプル

    const int maxSamples_ = 1000;          ///< 最大サンプル数
    bool metricsCollecting_ = true;        ///< メトリクス収集中フラグ

    // ========================================================
    // 初期化
    // ========================================================
    /**
     * @brief アプリケーションの初期化
     * @param[in] hInst アプリケーションのインスタンスハンドル
     * @param[in] width ウィンドウの幅
     * @param[in] height ウィンドウの高さ
     * @return bool 初期化が成功した場合は true, それ以外は false
     */
    bool Init(HINSTANCE hInst, int width = 1080, int height = 720) {
        DEBUGLOG("========================================");
        DEBUGLOG("App::Init() 開始");
        DEBUGLOG("ウィンドウサイズ: " + std::to_string(width) + "x" + std::to_string(height));

        // P2: COM初期化モードを明示的に記録
        HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(hrCom)) {
            DEBUGLOG("COMライブラリを初期化 (モード: COINIT_MULTITHREADED, HRESULT: 0x" + std::to_string(hrCom) + ")");
            if (hrCom == S_FALSE) {
                DEBUGLOG("COMは既に初期化されていました (S_FALSE)");
            }
        } else {
            DEBUGLOG_ERROR("COMライブラリの初期化失敗 (HRESULT: 0x" + std::to_string(hrCom) + ")");
            return false;
        }

        if (!CreateAppWindow(hInst, width, height)) {
            DEBUGLOG("[ERROR] CreateAppWindow() 失敗");
            return false;
        }

        if (!InitializeGraphics(width, height)) {
            DEBUGLOG("[ERROR] InitializeGraphics() 失敗");
            return false;
        }

        // サービスロケータに登録（GfxDeviceとTextureManagerはInitializeGraphics内で登録済み）
        ServiceLocator::Register(&input_);
        ServiceLocator::Register(&gamepad_);
        ServiceLocator::Register(&world_);
        ServiceLocator::Register(&renderer_);
        ServiceLocator::Register(&resManager_);
        ServiceLocator::Register(&sceneManager_);

        SetupCamera(width, height);

        // ゲームシーンの初期化
        InitializeGame();

        DEBUGLOG("App::Init() 正常に完了");
        DEBUGLOG("========================================");
        return true;
    }

    // ========================================================
    // メインループ
    // ========================================================
    /**
     * @brief メインループの実行
     * @details
     * アプリケーションが終了するまで、メッセージ処理、更新、描画を繰り返します。
     */
    void Run() {
        DEBUGLOG("App::Run() - メインループ開始");

        MSG msg{};
        auto previousTime = std::chrono::high_resolution_clock::now();
        int frameCount = 0;

        const float fixedDeltaTime = 1.0f / 60.0f; // Fixed update step (60Hz)
        float accumulator = 0.0f;

        while (msg.message != WM_QUIT) {
            // フレーム番号をログシステムに設定
            DebugLog::GetInstance().SetFrame(frameCount);

            // Windowsメッセージ処理
            if (ProcessWindowsMessages(msg)) {
                continue;
            }

            // フレーム開始時刻
            auto frameStartTime = std::chrono::high_resolution_clock::now();

#if ENABLE_DEBUG_VISUALS
            debugDraw_.Clear();
#endif

            // 時間の計算
            float deltaTime = CalculateDeltaTime(previousTime);

            // デルタタイムの異常値チェック (Spiral of Death prevention)
            if (deltaTime > 0.25f) {
                DEBUGLOG("[WARNING] 異常なdeltaTimeを検出: " + std::to_string(deltaTime) + "s (0.25sにクランプ)");
                deltaTime = 0.25f;
            }

            accumulator += deltaTime;
            // スパイラル回避: 溜まり過ぎを抑制（最大5フレーム分まで）
            const float maxAccum = fixedDeltaTime * 5.0f;
            if (accumulator > maxAccum) {
                accumulator = maxAccum;
            }

            // ========== UPDATE PHASE (Fixed Step) ==========
            while (accumulator >= fixedDeltaTime) {
                auto updateStartTime = std::chrono::high_resolution_clock::now();

                // 入力の更新（フォーカスがある場合のみ）
                if (isWindowFocused_) {
                    input_.Update();
                }

                // ゲームパッドの更新（フォーカスがある場合のみ）
                if (isWindowFocused_) {
                    gamepad_.Update();
                }

                // ConfigManagerの更新（ホットリロード処理）
                ConfigManager::Instance().Update();

                // F5キーで手動リロード（フォーカスがある場合のみ）
                if (isWindowFocused_ && input_.GetKeyDown(VK_F5)) {
                    ConfigManager::Instance().ForceReload();
                    DEBUGLOG_CATEGORY(DebugLog::Category::System, "F5キーが押されました - 設定ファイルを強制リロード");
                }
#if ENABLE_DEBUG_VISUALS
                if (isWindowFocused_) {
                    UpdateDebugCamera(fixedDeltaTime);
                }
#endif // ENABLE_DEBUG_VISUALS

                // ESCキーで終了（フォーカスがある場合のみ）
                if (isWindowFocused_ && input_.GetKeyDown(VK_ESCAPE)) {
                    DEBUGLOG_CATEGORY(DebugLog::Category::System, "ESCキーが押されました - アプリケーション終了要求（ユーザー操作）");
                    PostQuitMessage(0);
                }

                // シーンの更新
                // フォーカスがない場合は更新をスキップするか、ポーズ状態にするのが一般的だが
                // ここでは元のロジックに従い、フォーカスがない場合はeffectiveDeltaTimeを0にするか、
                // あるいはUpdate自体を呼ばないか。
                // 元のロジック: float effectiveDeltaTime = isWindowFocused_ ? deltaTime : 0.0f;
                // 固定ステップなので、Updateを呼ぶならfixedDeltaTimeを渡すべき。
                // フォーカスがないときに時間を進めないなら、accumulatorを加算しない、あるいはここでスキップする。

                if (isWindowFocused_) {
                    try {
                        // GameSceneにWorld参照を渡す（遅延リスポーンなどで使用）
                        if (auto* gs = dynamic_cast<GameScene*>(sceneManager_.GetCurrentScene())) {
                            gs->SetWorldRef(&world_);
                        }
                        sceneManager_.Update(world_, input_, fixedDeltaTime);
                    } catch (const std::exception& e) {
                        DEBUGLOG("[CRITICAL ERROR] シーン更新中に例外が発生: " + std::string(e.what()));
                        PostQuitMessage(-1);
                    }
                }

            accumulator -= fixedDeltaTime;

            auto updateEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> updateDuration = updateEndTime - updateStartTime;
            currentMetrics_.updateTime = updateDuration.count();
            }

            // ========== RENDER PHASE (Variable Step) ==========
            auto renderStartTime = std::chrono::high_resolution_clock::now();

            // BeginFrameとレンダリング処理
            gfx_.BeginFrame();

#if ENABLE_DEBUG_VISUALS
            DrawDebugInfo();
#endif // ENABLE_DEBUG_VISUALS

            // アクティブなシーンがGameSceneなら、そのカメラを使用
            if (auto* gs = dynamic_cast<GameScene*>(sceneManager_.GetCurrentScene())) {
                camera_ = gs->GetCamera();
            }

            renderer_.Render(world_, camera_);
            sceneManager_.Render(world_);

#if ENABLE_DEBUG_VISUALS
            debugDraw_.Render(gfx_, camera_);
#endif // ENABLE_DEBUG_VISUALS

            auto renderEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> renderDuration = renderEndTime - renderStartTime;
            currentMetrics_.renderTime = renderDuration.count();

            // ========== PRESENT PHASE ==========
            auto presentStartTime = std::chrono::high_resolution_clock::now();

            // Present実行（VSync待機含む - VSyncが有効ならここで待つことになる）
            gfx_.EndFrame();

            auto presentEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> presentDuration = presentEndTime - presentStartTime;
            currentMetrics_.presentTime = presentDuration.count();

            // フレーム合計時間
            std::chrono::duration<float> frameDuration = presentEndTime - frameStartTime;
            currentMetrics_.totalTime = frameDuration.count();

            if (metricsCollecting_ && frameTotalSamples_.size() < maxSamples_) {
                frameTotalSamples_.push_back(currentMetrics_.totalTime);
                updateSamples_.push_back(currentMetrics_.updateTime);
                renderSamples_.push_back(currentMetrics_.renderTime);
                presentSamples_.push_back(currentMetrics_.presentTime);
            }

            UpdateWindowTitle();

#if ENABLE_METRICS_LOG
            MaybeLogMetrics(frameCount, renderer_.GetStatistics());
#endif

#if ENABLE_FRAME_PACING
            const double targetFrameSec = 1.0 / static_cast<double>(FRAME_PACING_TARGET_FPS);
            double elapsedSec = frameDuration.count();
            double remainingSec = targetFrameSec - elapsedSec;
            if (remainingSec > 0.0) {
                const double spinThreshold = FRAME_PACING_SPIN_THRESHOLD_MS / 1000.0;
                if (remainingSec > spinThreshold) {
                    DWORD sleepMs = static_cast<DWORD>((remainingSec - spinThreshold) * 1000.0);
                    if (sleepMs > 0) {
                        ::Sleep(sleepMs);
                    }
                }
                // 残りわずかは軽くスピン＋譲り
                while (true) {
                    auto now = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> sinceStart = now - frameStartTime;
                    if (sinceStart.count() >= targetFrameSec) {
                        break;
                    }
                    ::Sleep(0);
                }
            }
#else
            // 軽い休止を入れてCPU占有率を抑制（スレッドを譲る）
            ::Sleep(0);
#endif

            frameCount++;
        }

        DEBUGLOG("App::Run() - メインループ終了 (総フレーム数: " + std::to_string(frameCount) + ")");
    }

    /**
     * @brief デストラクタ
     */
    ~App() {
        DEBUGLOG("App::~App() - デストラクタ呼び出し");

        // 終了前にフレーム統計を出力
        OutputFrameStatistics();

        Shutdown();
        DEBUGLOG("App 正常に破棄");
    }

private:
#if ENABLE_DEBUG_VISUALS
    void UpdateDebugCamera(float deltaTime) {
        const float moveSpeed = 10.0f;
        const float rotateSpeed = DirectX::XMConvertToRadians(90.0f);

        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera_.position);
        DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camera_.target);
        DirectX::XMVECTOR up = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&camera_.up));

        DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(target, pos);
        float forwardLenSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(forward));
        if (forwardLenSq > 0.0f) {
            forward = DirectX::XMVectorScale(forward, 1.0f / std::sqrt(forwardLenSq));
        } else {
            forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        }

        DirectX::XMVECTOR right = DirectX::XMVector3Cross(up, forward);
        float rightLenSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(right));
        if (rightLenSq > 0.0f) {
            right = DirectX::XMVectorScale(right, 1.0f / std::sqrt(rightLenSq));
        } else {
            right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        }


        DirectX::XMVECTOR moveVec = DirectX::XMVectorZero();

        if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(moveVec)) > 0.0f) {
            moveVec = DirectX::XMVector3Normalize(moveVec);
            moveVec = DirectX::XMVectorScale(moveVec, moveSpeed * deltaTime);
            pos = DirectX::XMVectorAdd(pos, moveVec);
            target = DirectX::XMVectorAdd(target, moveVec);
        }

        DirectX::XMStoreFloat3(&camera_.position, pos);
        DirectX::XMStoreFloat3(&camera_.target, target);
        camera_.Update();
    }
#endif // ENABLE_DEBUG_VISUALS
    /**
     * @brief アプリケーション終了時のクリーンアップ
     * @details 正しい順序でリソースを解放します
     */
    void Shutdown() {
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "App::Shutdown() - クリーンアップ開始");

        // Phase 0: すべてのシステムを停止（新規Spawn無効化）
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 0: すべてのシステムを停止（新規Spawn無効化）");
        world_.StopAllSystems();

        // Phase 1: シーンマネージャーの終了（シーンのOnExitを呼び出し）
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 1: SceneManagerのシャットダウン");
        sceneManager_.Shutdown(world_);

        // Phase 2: WorldのDestroyキュー/Spawnキューを明示的にフラッシュ
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 2: Worldキューをフラッシュ (エンティティ数: " + std::to_string(world_.GetAliveCount()) + ")");
        world_.FlushDestroyEndOfFrame();
        world_.FlushSpawnStartOfFrame(); // 念のため（systemsStopped_でガード済み）

        // Phase 3: World破棄前に残存エンティティを警告
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 3: World破棄前の残存エンティティチェック");
        if (world_.GetAliveCount() > 0) {
            DEBUGLOG_WARNING("World破棄前に " + std::to_string(world_.GetAliveCount()) + " 個の生存エンティティが残っています");
        } else {
            DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 3: すべてのエンティティが正常に破棄されました");
        }

#if ENABLE_DEBUG_VISUALS
        // Phase 4: デバッグ描画解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 4: DebugDrawを解放");
        debugDraw_.Shutdown();
#endif // ENABLE_DEBUG_VISUALS

        // Phase 5: レンダリングシステム解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 5: RenderSystemを解放");
        renderer_.Shutdown();

        // Phase 6: テクスチャマネージャー解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 6: TextureManagerを解放");
        texManager_.Shutdown();

        // リソースマネージャーをクリア
        resManager_.Clear();

        // Phase 7: 入力システム解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 7: InputSystemを解放");
        input_.Shutdown();

        // Phase 7.5: ゲームパッドシステム解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 7.5: GamepadSystemを解放");
        gamepad_.Shutdown();

        // Phase 8: グラフィックスデバイス解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 8: GfxDeviceを解放");
        gfx_.Shutdown();

        // Phase 9: COM終了（最後）
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 9: COMを終了");
        CoUninitialize();

        // サービスロケータをシャットダウン
        ServiceLocator::Shutdown();

        DEBUGLOG_CATEGORY(DebugLog::Category::System, "App::Shutdown() 正常に完了");
    }

#if ENABLE_METRICS_LOG
    /**
     * @brief 計測メトリクスをログ出力（一定フレーム間隔）
     */
    void MaybeLogMetrics(int frameCount, const RenderSystem::Statistics& renderStats) {
        if (frameCount % METRICS_LOG_INTERVAL_FRAMES != 0) {
            return;
        }

        auto toMs = [](float seconds) { return seconds * 1000.0f; };

        const std::filesystem::path csvPath = "metrics_log.csv";
        const bool needHeader = !std::filesystem::exists(csvPath);

        std::ofstream ofs(csvPath, std::ios::app);
        if (!ofs.is_open()) {
            DEBUGLOG_WARNING("[Metrics] metrics_log.csv を開けませんでした");
            return;
        }

        if (needHeader) {
            ofs << "frame,total_ms,update_ms,render_ms,present_ms,drawcalls,models,meshes,entities,behaviours\n";
        }

        ofs << frameCount << ','
            << std::fixed << std::setprecision(3)
            << toMs(currentMetrics_.totalTime) << ','
            << toMs(currentMetrics_.updateTime) << ','
            << toMs(currentMetrics_.renderTime) << ','
            << toMs(currentMetrics_.presentTime) << ','
            << renderStats.totalDrawCalls << ','
            << renderStats.modelsRendered << ','
            << renderStats.meshesRendered << ','
            << world_.GetAliveCount() << ','
            << world_.GetBehaviourCount() << '\n';
    }
#endif // ENABLE_METRICS_LOG

    // ========================================================
    // 初期化ヘルパー
    // ========================================================

    /**
     * @brief ウィンドウを作成する
     * @param[in] hInst インスタンスハンドル
     * @param[in] width 幅
     * @param[in] height 高さ
     * @return bool 成功した場合は true
     */
    bool CreateAppWindow(HINSTANCE hInst, int width, int height) {
        DEBUGLOG("CreateAppWindow() 開始");

        WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProcStatic;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"MiniGame_Class";

        if (!RegisterClassEx(&wc)) {
            DEBUGLOG("[ERROR] RegisterClassEx() 失敗 - エラーコード: " + std::to_string(GetLastError()));
            return false;
        }
        DEBUGLOG("ウィンドウクラスを正常に登録");

        RECT rc{ 0, 0, width, height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        DEBUGLOG("ウィンドウサイズを調整: " + std::to_string(rc.right - rc.left) + "x" + std::to_string(rc.bottom - rc.top));

        hwnd_ = CreateWindowW(
            wc.lpszClassName,
            L"はじく！",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInst, this
        );

        if (!hwnd_) {
            DEBUGLOG("[ERROR] CreateWindowW() 失敗 - エラーコード: " + std::to_string(GetLastError()));
            return false;
        }

        DEBUGLOG("ウィンドウ作成成功 (HWND: 0x" + std::to_string(reinterpret_cast<uintptr_t>(hwnd_)) + ")");

        input_.SetWindowHandle(hwnd_);
        ShowWindow(hwnd_, SW_SHOW);
        DEBUGLOG("ウィンドウを表示");
        DEBUGLOG("CreateAppWindow() 正常に完了");
        return true;
    }

    /**
     * @brief グラフィックス関連の初期化
     * @param[in] width 幅
     * @param[in] height 高さ
     * @return bool 成功した場合は true
     */
    bool InitializeGraphics(int width, int height) {
        DEBUGLOG("InitializeGraphics() 開始");

        if (!gfx_.Init(hwnd_, width, height)) {
            DEBUGLOG("[CRITICAL ERROR] GfxDevice::Init() 失敗");
            MessageBoxA(nullptr, "DirectX11の初期化に失敗", "エラー", MB_OK | MB_ICONERROR);
            return false;
        }
        DEBUGLOG("GfxDeviceを正常に初期化");

        // Register GfxDevice early so other systems (e.g. RenderSystem::Init) can retrieve it
        ServiceLocator::Register(&gfx_);

        if (!texManager_.Init(gfx_)) {
            DEBUGLOG("[ERROR] TextureManager::Init() 失敗");
            MessageBoxA(nullptr, "TextureManagerの初期化に失敗", "エラー", MB_OK | MB_ICONERROR);
            return false;
        }
        DEBUGLOG("TextureManagerを正常に初期化");

        // Register TextureManager before renderer init so RenderSystem can access textures if needed
        ServiceLocator::Register(&texManager_);

        if (!renderer_.Init()) {
            DEBUGLOG("[ERROR] RenderSystem::Init() 失敗");
            MessageBoxA(nullptr, "RenderSystemの初期化に失敗", "エラー", MB_OK | MB_ICONERROR);
            return false;
        }
        DEBUGLOG("RenderSystemを正常に初期化");

        input_.Init();
        DEBUGLOG("InputSystemを初期化");

        // GamepadSystemを初期化
     DEBUGLOG("GamepadSystemを初期化中");
        if (!gamepad_.Init()) {
            DEBUGLOG_WARNING("GamepadSystem::Init() 失敗 - ゲームパッドは利用できません");
       // 致命的ではないため続行
        } else {
  DEBUGLOG("GamepadSystemを正常に初期化");
     }

#if ENABLE_DEBUG_VISUALS
        DEBUGLOG("DebugDrawを初期化中 (DEBUGビルド)");
        if (!debugDraw_.Init(gfx_)) {
            DEBUGLOG("[WARNING] DebugDraw::Init() 失敗 - デバッグビジュアライゼーションは利用できません");
            MessageBoxA(nullptr, "DebugDrawの初期化に失敗", "警告", MB_OK | MB_ICONWARNING);
        } else {
            DEBUGLOG("DebugDrawを正常に初期化");
            ServiceLocator::Register(&debugDraw_); // Register DebugDraw in ServiceLocator
        }
#endif // ENABLE_DEBUG_VISUALS

        DEBUGLOG("InitializeGraphics() 正常に完了");
        return true;
    }

    /**
     * @brief カメラのセットアップ
     * @param[in] width 幅
     * @param[in] height 高さ
     */
    void SetupCamera(int width, int height) {
        DEBUGLOG("SetupCamera() 開始");

        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        DEBUGLOG("アスペクト比: " + std::to_string(aspectRatio));

        const float fovRad = DirectX::XMConvertToRadians(cfg_CamFovDeg.Get());
        const float nearZ = cfg_CamNear.Get();
        const float farZ = cfg_CamFar.Get();
        const DirectX::XMFLOAT3 camPos{ cfg_CamPosX.Get(), cfg_CamPosY.Get(), cfg_CamPosZ.Get() };
        const DirectX::XMFLOAT3 camTarget{ cfg_CamTargetX.Get(), cfg_CamTargetY.Get(), cfg_CamTargetZ.Get() };
        const DirectX::XMFLOAT3 camUp{ cfg_CamUpX.Get(), cfg_CamUpY.Get(), cfg_CamUpZ.Get() };

        camera_ = Camera::LookAtLH(
            fovRad,
            aspectRatio,
            nearZ,
            farZ,
            camPos,
            camTarget,
            camUp
        );
    }

    /**
     * @brief ゲーム関連の初期化
     */

    // ========================================================
    // メインループのヘルパー
    // ========================================================

    /**
     * @brief Windowsメッセージを処理する
     * @param[out] msg メッセージ構造体
     * @return bool メッセージを処理した場合は true
     */
    bool ProcessWindowsMessages(MSG& msg) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return true;
        }
        return false;
    }

    /**
     * @brief デルタタイムを計算する
     * @param[in,out] previousTime 前フレームの時刻
     * @return float デルタタイム（秒）
     */
    float CalculateDeltaTime(std::chrono::high_resolution_clock::time_point& previousTime) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        return deltaTime.count();
    }

    /**
     * @brief Updates the OS window title with a fixed label.
     */
    void UpdateWindowTitle() {
        if (hwnd_) {
            SetWindowTextW(hwnd_, L"HEW GAME");
        }
    }
#if ENABLE_DEBUG_VISUALS
    /**
     * @brief デバッグ情報を描画する
     */
    void DrawDebugInfo() {

        // グリッドを少し下げて描画（Y = -0.01）
        debugDraw_.DrawGrid(20.0f, 20, DirectX::XMFLOAT3{0.2f, 0.2f, 0.2f});

        // 座標軸を後から描画して見やすくする
        debugDraw_.DrawAxes(500.0f);
    }
#endif // ENABLE_DEBUG_VISUALS

    /**
     * @brief フレーム統計を出力する
     * @details
     * アプリケーション終了時に、収集したフレームメトリクスの統計
     * （平均、最小、最大、99%タイル）をログに出力します。
     */
    void OutputFrameStatistics() {
        if (frameTotalSamples_.empty()) {
            DEBUGLOG_WARNING("フレーム統計データが収集されていません");
            return;
        }

        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "フレーム統計サマリ (サンプル数: " + std::to_string(frameTotalSamples_.size()) + ")");
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");

        // 各メトリクスの統計を計算
        OutputMetricStatistics("フレーム合計時間", frameTotalSamples_, "ms");
        OutputMetricStatistics("Update時間", updateSamples_, "ms");
        OutputMetricStatistics("Render時間", renderSamples_, "ms");
        OutputMetricStatistics("Present時間", presentSamples_, "ms");

        // FPS統計
        std::vector<float> fpsSamples;
        fpsSamples.reserve(frameTotalSamples_.size());
        for (float t : frameTotalSamples_) {
            if (t > 0.0f) {
                fpsSamples.push_back(1.0f / t);
            }
        }
        OutputMetricStatistics("FPS", fpsSamples, "");

        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");
    }

    /**
     * @brief メトリクス統計を出力する
     * @param name メトリクス名
     * @param samples サンプルデータ
     * @param unit 単位
     */
    void OutputMetricStatistics(const std::string& name, std::vector<float> samples, const std::string& unit) {
        if (samples.empty()) return;

        // 外れ値フィルタリング（上位1%と下位1%を除外）
        std::sort(samples.begin(), samples.end());

        size_t removeCount = static_cast<size_t>(samples.size() * 0.01);
        if (removeCount > 0 && samples.size() > removeCount * 2) {
            samples.erase(samples.begin(), samples.begin() + removeCount); // 下位1%を除外
            samples.erase(samples.end() - removeCount, samples.end());     // 上位1%を除外
        }

        if (samples.empty()) {
            DEBUGLOG_WARNING(name + ": 外れ値除外後にサンプルが空になりました");
            return;
        }

        // 統計計算
        float sum = 0.0f;
        for (float s : samples) sum += s;
        float avg = sum / samples.size();
        float min = samples.front();
        float max = samples.back();

        // 99%タイル（上位1%を除外）
        size_t p99Index = static_cast<size_t>(samples.size() * 0.99);
        if (p99Index >= samples.size()) p99Index = samples.size() - 1;
        float p99 = samples[p99Index];

        // 50%タイル（中央値）
        size_t p50Index = samples.size() / 2;
        float p50 = samples[p50Index];

        // 1%タイル（下位1%）
        size_t p01Index = static_cast<size_t>(samples.size() * 0.01);
        float p01 = samples[p01Index];

        // ログ出力（msの場合は1000倍、FPSの場合はそのまま）
        float multiplier = (unit == "ms") ? 1000.0f : 1.0f;

        std::ostringstream oss;
        oss << name << " (外れ値除外後サンプル数: " << samples.size() << "): "
            << "平均=" << std::fixed << std::setprecision(2) << (avg * multiplier) << unit
            << ", 最小=" << (min * multiplier) << unit
            << ", 1%タイル=" << (p01 * multiplier) << unit
            << ", 中央値=" << (p50 * multiplier) << unit
            << ", 99%タイル=" << (p99 * multiplier) << unit
            << ", 最大=" << (max * multiplier) << unit;

        DEBUGLOG_CATEGORY(DebugLog::Category::System, oss.str());
    }

    // ========================================================
    // Windowsメッセージ処理
    // ========================================================

    /**
     * @brief ウィンドウプロシージャ（静的）
     * @details
     * ウィンドウ作成時にインスタンスをウィンドウに紐付け、
     * 以降のメッセージをメンバ関数のWndProcに転送します。
     */
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* app = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
            app = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            DEBUGLOG("WM_NCCREATE: Appインスタンスをウィンドウに関連付け");
        } else {
            app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        if (app) {
            return app->WndProc(hWnd, msg, wp, lp);
        }

        return DefWindowProc(hWnd, msg, wp, lp);
    }

    /**
     * @brief ウィンドウプロシージャ（メンバ関数）
     * @details アプリケーション固有のウィンドウメッセージを処理します。
     */
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_ACTIVATE:
            if (LOWORD(wp) == WA_INACTIVE) {
                isWindowFocused_ = false;
                DEBUGLOG_CATEGORY(DebugLog::Category::System, "ウィンドウがフォーカスを失いました - 入力と更新を停止");
            } else {
                isWindowFocused_ = true;
                DEBUGLOG_CATEGORY(DebugLog::Category::System, "ウィンドウがフォーカスを取得しました - 入力と更新を再開");
            }
            return 0;

        case WM_CLOSE:
            DEBUGLOG_CATEGORY(DebugLog::Category::System, "WM_CLOSEを受信 - ユーザーによるウィンドウ閉じる操作");
            // デフォルト処理に委ねる（WM_DESTROYが発生）
            return DefWindowProc(hWnd, msg, wp, lp);

        case WM_DESTROY:
            DEBUGLOG_CATEGORY(DebugLog::Category::System, "WM_DESTROYを受信 - 終了メッセージを投稿");
            PostQuitMessage(0);
            return 0;

        case WM_MOUSEWHEEL:
            input_.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wp));
            return 0;
        }

        return DefWindowProc(hWnd, msg, wp, lp);
    }
};

// ========================================================
// 作成者: 山内陽
// バージョン: v5.1 - Doxygenコメントを追加
// ========================================================


