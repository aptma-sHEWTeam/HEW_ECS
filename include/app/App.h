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
// 【ゲーム内容】シンプルなシューティングゲーム
// 【操作方法】A/D: 移動, スペース: 弾発射, ESC: 終了
// ========================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <DirectXMath.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

// DirectX11 & ECS システム
#include "graphics/GfxDevice.h"
#include "graphics/RenderSystem.h"
#include "ecs/World.h"
#include "graphics/Camera.h"
#include "input/InputSystem.h"
#include "graphics/TextureManager.h"
#include "graphics/DebugDraw.h"

#ifdef _DEBUG
#include "app/DebugLog.h"
#endif

// コンポーネント
#include "components/Transform.h"
#include "components/MeshRenderer.h"

// ゲームシステム
#include "scenes/SceneManager.h"
#include "scenes/MiniGame.h"

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

    // DirectX11システム
    GfxDevice gfx_; ///< グラフィックスデバイス
    RenderSystem renderer_; ///< 描画システム
    TextureManager texManager_; ///< テクスチャ管理

    // ECSシステム
    World world_; ///< ECSワールド
    Camera camera_; ///< カメラ
    InputSystem input_; ///< 入力システム

    // シーン管理
    SceneManager sceneManager_; ///< シーンマネージャー
    GameScene* gameScene_ = nullptr; ///< 現在のゲームシーン

#ifdef _DEBUG
    DebugDraw debugDraw_; ///< デバッグ描画用
#endif

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
    FrameMetrics avgMetrics_;           ///< 平均メトリクス
    int metricsFrameCount_ = 0;         ///< メトリクス計測フレーム数
    const int metricsUpdateInterval_ = 30; ///< メトリクス更新間隔（フレーム）

    // 詳細統計用（最大1000フレーム分のサンプル）
    std::vector<float> frameTotalSamples_; ///< フレーム合計時間のサンプル
    std::vector<float> updateSamples_;     ///< Update時間のサンプル
    std::vector<float> renderSamples_;     ///< Render時間のサンプル
    std::vector<float> presentSamples_;    ///< Present時間のサンプル
#ifdef _DEBUG
    std::vector<float> gpuSamples_;        ///< GPUレンダ時間のサンプル(ms)
#endif
    
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
    bool Init(HINSTANCE hInst, int width = 1280, int height = 720) {
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
        
        while (msg.message != WM_QUIT) {
            // フレーム番号をログシステムに設定
            DebugLog::GetInstance().SetFrame(frameCount);
            
            // Windowsメッセージ処理
            if (ProcessWindowsMessages(msg)) {
                continue;
            }
            
            // フレーム開始時刻
            auto frameStartTime = std::chrono::high_resolution_clock::now();
            
            // 時間の計算
            float deltaTime = CalculateDeltaTime(previousTime);
            
            // デルタタイムの異常値チェック
            if (deltaTime > 1.0f) {
                DEBUGLOG("[WARNING] 異常なdeltaTimeを検出: " + std::to_string(deltaTime) + "s (0.1sにクランプ)");
                deltaTime = 0.1f;
            }
            
            // ========== UPDATE PHASE ==========
            auto updateStartTime = std::chrono::high_resolution_clock::now();
            
            // 入力の更新
            input_.Update();
            
            // ESCキーで終了
            if (input_.GetKeyDown(VK_ESCAPE)) {
                DEBUGLOG_CATEGORY(DebugLog::Category::System, "ESCキーが押されました - アプリケーション終了要求（ユーザー操作）");
                PostQuitMessage(0);
            }
            
            // シーンの更新
            try {
                sceneManager_.Update(world_, input_, deltaTime);
            } catch (const std::exception& e) {
                DEBUGLOG("[CRITICAL ERROR] シーン更新中に例外が発生: " + std::string(e.what()));
                PostQuitMessage(-1);
            }
            
            auto updateEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> updateDuration = updateEndTime - updateStartTime;
            currentMetrics_.updateTime = updateDuration.count();
            
            // ========== RENDER PHASE ==========
            auto renderStartTime = std::chrono::high_resolution_clock::now();
            
            // BeginFrameとレンダリング処理
            gfx_.BeginFrame();

#ifdef _DEBUG
            // GPUタイムスタンプ: 描画区間を計測
            gfx_.BeginGpuTiming();
#endif
            
#ifdef _DEBUG
            DrawDebugInfo();
#endif
            
            renderer_.Render(gfx_, world_, camera_);
            
#ifdef _DEBUG
            debugDraw_.Render(gfx_, camera_);
            // GPUタイミング終了（Presentの前）
            gfx_.EndGpuTiming();
#endif
            
            auto renderEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> renderDuration = renderEndTime - renderStartTime;
            currentMetrics_.renderTime = renderDuration.count();
            
            // ========== PRESENT PHASE ==========
            auto presentStartTime = std::chrono::high_resolution_clock::now();
            
            // Present実行（VSync待機含む）
            gfx_.EndFrame();
            
            auto presentEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> presentDuration = presentEndTime - presentStartTime;
            currentMetrics_.presentTime = presentDuration.count();
            
#ifdef _DEBUG
            // GPU時間を解決（ms）
            double gpuMs = -1.0;
            if (gfx_.ResolveGpuTiming(gpuMs)) {
                if (metricsCollecting_ && gpuSamples_.size() < maxSamples_) {
                    gpuSamples_.push_back(static_cast<float>(gpuMs));
                }
            }
#endif

            // フレーム合計時間（Update+Render+Presentを包含）
            std::chrono::duration<float> frameDuration = presentEndTime - frameStartTime;
            currentMetrics_.totalTime = frameDuration.count();
            
            // メトリクス集計
            avgMetrics_.updateTime += currentMetrics_.updateTime;
            avgMetrics_.renderTime += currentMetrics_.renderTime;
            avgMetrics_.presentTime += currentMetrics_.presentTime;
            avgMetrics_.totalTime += currentMetrics_.totalTime;
            metricsFrameCount_++;
            
            // サンプル収集（最大1000フレーム）
            if (metricsCollecting_ && frameTotalSamples_.size() < maxSamples_) {
                frameTotalSamples_.push_back(currentMetrics_.totalTime);
                updateSamples_.push_back(currentMetrics_.updateTime);
                renderSamples_.push_back(currentMetrics_.renderTime);
                presentSamples_.push_back(currentMetrics_.presentTime);
            }
            
            // ウィンドウタイトルにスコアとFPS表示
            if (metricsFrameCount_ >= metricsUpdateInterval_) {
                UpdateWindowTitleWithMetrics();
                
                // リセット
                avgMetrics_ = FrameMetrics{};
                metricsFrameCount_ = 0;
            }

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
        gameScene_ = nullptr;  // SceneManagerが所有しているのでnullptrに設定するだけ
        
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
        
#ifdef _DEBUG
        // Phase 4: デバッグ描画解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 4: DebugDrawを解放");
        debugDraw_.Shutdown();
#endif
        
        // Phase 5: レンダリングシステム解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 5: RenderSystemを解放");
        renderer_.Shutdown();
        
        // Phase 6: テクスチャマネージャー解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 6: TextureManagerを解放");
        texManager_.Shutdown();
        
        // Phase 7: 入力システム解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 7: InputSystemを解放");
        input_.Shutdown();
        
        // Phase 8: グラフィックスデバイス解放
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 8: GfxDeviceを解放");
        gfx_.Shutdown();
        
        // Phase 9: COM終了（最後）
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Phase 9: COMを終了");
        CoUninitialize();
        
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "App::Shutdown() 正常に完了");
    }

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
            L"シューティングゲーム - A/D:移動 スペース:発射 ESC:終了",
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
        
        if (!texManager_.Init(gfx_)) {
            DEBUGLOG("[ERROR] TextureManager::Init() 失敗");
            MessageBoxA(nullptr, "TextureManagerの初期化に失敗", "エラー", MB_OK | MB_ICONERROR);
            return false;
        }
        DEBUGLOG("TextureManagerを正常に初期化");
        
        if (!renderer_.Init(gfx_, texManager_)) {
            DEBUGLOG("[ERROR] RenderSystem::Init() 失敗");
            MessageBoxA(nullptr, "RenderSystemの初期化に失敗", "エラー", MB_OK | MB_ICONERROR);
            return false;
        }
        DEBUGLOG("RenderSystemを正常に初期化");

        input_.Init();
        DEBUGLOG("InputSystemを初期化");

#ifdef _DEBUG
        DEBUGLOG("DebugDrawを初期化中 (DEBUGビルド)");
        if (!debugDraw_.Init(gfx_)) {
            DEBUGLOG("[WARNING] DebugDraw::Init() 失敗 - デバッグビジュアライゼーションは利用できません");
            MessageBoxA(nullptr, "DebugDrawの初期化に失敗", "警告", MB_OK | MB_ICONWARNING);
        } else {
            DEBUGLOG("DebugDrawを正常に初期化");
        }
#endif

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
        
        camera_ = Camera::LookAtLH(
            DirectX::XM_PIDIV4,
            aspectRatio,
            0.1f,
            100.0f,
            DirectX::XMFLOAT3{ 0, 0, -20 },  // カメラを引いて全体が見えるように
            DirectX::XMFLOAT3{ 0, 0, 0 },
            DirectX::XMFLOAT3{ 0, 1, 0 }
        );
        
        DEBUGLOG("カメラ設定完了 (位置: 0, 0, -20 | ターゲット: 0, 0, 0)");
    }
    
    /**
     * @brief ゲーム関連の初期化
     */
    void InitializeGame() {
        DEBUGLOG("InitializeGame() 開始");
        
        // ゲームシーンを作成
        gameScene_ = new GameScene(texManager_);
        DEBUGLOG("GameSceneインスタンスを作成");
        
        // シーンマネージャーに登録
        sceneManager_.RegisterScene("Game", gameScene_);
        DEBUGLOG("GameSceneをSceneManagerに登録");
        
        sceneManager_.Init(gameScene_, world_);
        DEBUGLOG("SceneManagerをGameSceneで初期化");
        
        DEBUGLOG("InitializeGame() 正常に完了");
    }

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
     * @brief ウィンドウタイトルを更新する
     */
    void UpdateWindowTitle() {
        if (gameScene_) {
            std::wstringstream ss;
            ss << L"シューティングゲーム - スコア: " << gameScene_->GetScore()
               << L" | A/D:移動 スペース:発射 ESC:終了";
            SetWindowTextW(hwnd_, ss.str().c_str());
        }
    }

    /**
     * @brief ウィンドウタイトルにメトリクスを含めて更新する
     */
    void UpdateWindowTitleWithMetrics() {
        if (gameScene_ && metricsFrameCount_ > 0) {
            float avgUpdate = avgMetrics_.updateTime / metricsFrameCount_ * 1000.0f; // ms
            float avgRender = avgMetrics_.renderTime / metricsFrameCount_ * 1000.0f; // ms
            float avgPresent = avgMetrics_.presentTime / metricsFrameCount_ * 1000.0f; // ms
            float avgTotal = avgMetrics_.totalTime / metricsFrameCount_; // s
            float fps = (avgTotal > 0.0f) ? (1.0f / avgTotal) : 0.0f;
            
            std::wstringstream ss;
            ss << L"シューティングゲーム - スコア: " << gameScene_->GetScore()
               << L" | FPS: " << static_cast<int>(fps)
               << L" (U:" << std::fixed << std::setprecision(1) << avgUpdate
               << L"ms R:" << avgRender
               << L"ms P:" << avgPresent << L"ms";
#ifdef _DEBUG
            if (!gpuSamples_.empty()) {
                // 直近GPU平均
                float gpuAvg = 0.0f;
                for (float v : gpuSamples_) gpuAvg += v;
                gpuAvg = gpuAvg / static_cast<float>(gpuSamples_.size());
                ss << L" G:" << std::fixed << std::setprecision(2) << gpuAvg << L"ms";
            }
#endif
            ss << L")";
            SetWindowTextW(hwnd_, ss.str().c_str());
        }
    }

#ifdef _DEBUG
    /**
     * @brief デバッグ情報を描画する
     */
    void DrawDebugInfo() {
        debugDraw_.Clear();
        
        // グリッドを少し下げて描画（Y = -0.01）
        debugDraw_.DrawGrid(20.0f, 20, DirectX::XMFLOAT3{0.2f, 0.2f, 0.2f});
        
        // 座標軸を後から描画して見やすくする
        debugDraw_.DrawAxes(5.0f);
    }
#endif

    /**
     * @brief フレーム統計を出力する
     * @details
     * アプリケーション終了時に、収集したフレームメトリクスの統計
     * （平均、最小、最大、99%タイル）をログに出力します。
     * すべてのメトリクスで同一の外れ値除外マスク（FrameTotalベース）を適用します。
     */
    void OutputFrameStatistics() {
        if (frameTotalSamples_.empty()) {
            DEBUGLOG_WARNING("フレーム統計データが収集されていません");
            return;
        }
        
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "フレーム統計サマリ (サンプル数: " + std::to_string(frameTotalSamples_.size()) + ")");
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");
        
        // 共通の外れ値除外インデックス（FrameTotalベース）を作成
        const size_t N = frameTotalSamples_.size();
        std::vector<size_t> indices(N);
        for (size_t i = 0; i < N; ++i) indices[i] = i;
        std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b){ return frameTotalSamples_[a] < frameTotalSamples_[b]; });
        
        size_t removeCount = static_cast<size_t>(static_cast<double>(N) * 0.01);
        size_t beginKeep = (removeCount * 2 < N) ? removeCount : 0;
        size_t endKeep = (removeCount * 2 < N) ? (N - removeCount) : N;
        
        std::vector<size_t> kept;
        kept.reserve(N);
        for (size_t i = beginKeep; i < endKeep; ++i) kept.push_back(indices[i]);
        if (kept.empty()) {
            DEBUGLOG_WARNING("外れ値除外後にサンプルが空になりました（共通マスク）");
            return;
        }
        
        // 各メトリクスを共通マスクで抽出
        auto extract = [&kept](const std::vector<float>& src){
            std::vector<float> out; out.reserve(kept.size());
            for (size_t idx : kept) out.push_back(src[idx]);
            return out;
        };
        std::vector<float> keptTotal = extract(frameTotalSamples_);
        std::vector<float> keptUpdate = extract(updateSamples_);
        std::vector<float> keptRender = extract(renderSamples_);
        std::vector<float> keptPresent = extract(presentSamples_);
        
        // 統計出力
        OutputMetricStatistics("フレーム合計時間", keptTotal, "ms", true);
        OutputMetricStatistics("Update時間", keptUpdate, "ms", true);
        OutputMetricStatistics("Render時間", keptRender, "ms", true);
        OutputMetricStatistics("Present時間", keptPresent, "ms", true);
        
        // FPSはFrameTotalから算出（同一マスク）
        std::vector<float> fpsSamples;
        fpsSamples.reserve(keptTotal.size());
        for (float t : keptTotal) if (t > 0.0f) fpsSamples.push_back(1.0f / t);
        OutputMetricStatistics("FPS", fpsSamples, "", false);
#ifdef _DEBUG
        // GPU時間（ms）も統計（同一マスクは適用不能のため参考値として単独出力）
        if (!gpuSamples_.empty()) {
            OutputMetricStatistics("GPU時間", gpuSamples_, "ms", true);
        }
#endif
        
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "========================================");
    }
    
    /**
     * @brief メトリクス統計を出力する
     * @param name メトリクス名
     * @param samples サンプルデータ
     * @param unit 単位（"ms" or ""）
     * @param toMs trueなら値をms表示（内部秒→ms換算）、falseならそのまま
     */
    void OutputMetricStatistics(const std::string& name, std::vector<float> samples, const std::string& unit, bool toMs) {
        if (samples.empty()) return;
        
        std::sort(samples.begin(), samples.end());
        
        // 基本統計
        float sum = 0.0f;
        for (float s : samples) sum += s;
        float avg = sum / samples.size();
        float min = samples.front();
        float max = samples.back();
        
        // パーセンタイル
        size_t p99Index = static_cast<size_t>(samples.size() * 0.99);
        if (p99Index >= samples.size()) p99Index = samples.size() - 1;
        float p99 = samples[p99Index];
        size_t p50Index = samples.size() / 2;
        float p50 = samples[p50Index];
        size_t p01Index = static_cast<size_t>(samples.size() * 0.01);
        float p01 = samples[p01Index];
        
        float mul = 1.0f;
        if (toMs && unit == "ms") mul = 1000.0f; // 秒→ミリ秒
        
        std::ostringstream oss;
        oss << name << " (外れ値除外後サンプル数: " << samples.size() << "): "
            << "平均=" << std::fixed << std::setprecision(2) << (avg * mul) << unit
            << ", 最小=" << (min * mul) << unit
            << ", 1%タイル=" << (p01 * mul) << unit
            << ", 中央値=" << (p50 * mul) << unit
            << ", 99%タイル=" << (p99 * mul) << unit
            << ", 最大=" << (max * mul) << unit;
        
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
