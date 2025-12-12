/**
 * @file StageComponents.h
 * @brief ステージ進行用のタグと状態コンポーネント
 * @author 山内陽
 * @date 2025
 * @version 5.0
 */
#pragma once

#include "components/Component.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <array>
#include <optional>
#include <filesystem>
#include <string_view>
#include <algorithm>
#include "config/ConfigVar.h"
#include "app/DebugLog.h" // DEBUGLOG_ERRORのために追加
#include <DirectXMath.h> // GoalAttractor 用
#include "components/Transform.h" // GoalAttractor 用
#include "components/GameStats.h"

using namespace std;

/**
 * @struct StartTag
 * @brief ステージの開始地点を示すタグ
 */
struct StartTag : IComponent {};

/**
 * @struct GoalTag
 * @brief ステージのゴール地点を示すタグ
 */
struct GoalTag : IComponent {};

/**
 * @struct StageProgress
 * @brief ステージ番号と進行フラグを管理
 */
struct StageProgress : IComponent {
    int currentStage = 1;
    int selectStage = 1;
    int currentRoom = 1; // 現在のルーム番号（同一ステージ内の部屋）
    bool requestAdvance = false;
};

/**
 * @brief 利用可能なステージ数を探索する
 * @details Assets/StageData/StageCollision/ 配下の DebugStageN / StageN を走査し、room1.csv が存在する最大番号を返す
 */
inline int GetAvailableStageCount() {
    namespace fs = std::filesystem;
    const fs::path baseDir{"Assets/StageData/StageCollision"};
    static std::optional<int> cachedCount;
    static bool hasLoggedMissingDir = false;
    std::error_code ec;

    if (!fs::exists(baseDir, ec) || ec) {
        if (!hasLoggedMissingDir) {
            DEBUGLOG_WARNING("[Stage] StageCollision ディレクトリが見つかりません。既定で1を使用します");
            hasLoggedMissingDir = true;
        }
        return cachedCount.value_or(1);
    }

    hasLoggedMissingDir = false;
    int maxStage = 1;
    for (const auto& entry : fs::directory_iterator(baseDir, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        const std::string name = entry.path().filename().string();
        auto tryUpdate = [&](std::string_view prefix) {
            if (name.rfind(prefix, 0) == 0) {
                const std::string numberPart = name.substr(prefix.size());
                try {
                    const int stageIdx = std::stoi(numberPart);
                    const fs::path csvPath = entry.path() / "room1.csv";
                    if (fs::exists(csvPath, ec) && !ec) {
                        maxStage = std::max(maxStage, stageIdx);
                    }
                } catch (const std::exception&) {
                    // 数値パースに失敗した場合はスキップ
                }
            }
        };

        tryUpdate("DebugStage");
        tryUpdate("Stage");
    }

    cachedCount = maxStage;
    return maxStage;
}

/**
 * @brief ステージ番号からCSVパスを解決する
 * @details DebugStageN -> StageN の順に room1.csv を探索して最初に見つかったパスを返す
 */
inline std::optional<std::string> ResolveStageCsvPath(int stage) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const std::string pathStr = "Assets/StageData/StageCollision/Stage" + std::to_string(stage) + "/room1.csv";
    const fs::path path{pathStr};
    if (fs::exists(path, ec) && !ec) {
        return pathStr;
    }

    return std::nullopt;
}

/**
 * @brief ステージ番号とルーム番号からCSVパスを解決する
 * @details 指定されたステージとルームに対応する CSV ファイルのパスを返す
 */
inline std::optional<std::string> ResolveStageRoomCsvPath(int stage, int room) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const std::string pathStr = "Assets/StageData/StageCollision/Stage" + std::to_string(stage) + "/room" + std::to_string(room) + ".csv";
    const fs::path path{pathStr};
    if (fs::exists(path, ec) && !ec) {
        return pathStr;
    }
    return std::nullopt;
}

/**
 * @struct StageCreate
 * @brief CSVファイルからデータを読み込み、ステージを生成
 */
struct StageCreate : IComponent {
    /**
     * @brief ステージファイルストリーム
     */
    ifstream m_file;
    /**
     * @brief 読み込んだCSVファイルのパス
     */
    std::string csvPath;

    /**
     * @brief ステージデータを格納する2次元ベクター
     */
    vector<vector<int>> stageMap;

    /**
     * @brief コンストラクタ
     * @details CSVファイルをオープンし、データを読み込む
     */
        explicit StageCreate(const std::string& csvPathIn) : csvPath(csvPathIn) {
            DEBUGLOG("[StageCreate] Constructor called with path: " + csvPathIn);
            if (csvPathIn.empty()) {
                DEBUGLOG_ERROR("[StageCreate] Stage CSV path is empty.");
                return;
            }
            m_file.open(csvPathIn);
            if (!m_file.is_open()) {
                DEBUGLOG_ERROR("[StageCreate] Failed to open CSV file: " + csvPathIn);
            } else {
                DEBUGLOG("[StageCreate] Successfully opened CSV file. Starting to load data...");
                loadStageData();
            }
        }
    
        StageCreate() = delete;
        /**
         * @brief ステージデータを読み込む
         * @details CSVファイルからデータをパースし、stageMapに格納
         */
        void loadStageData() {
            string line;
            int lineCount = 0;
            while (getline(m_file, line)) {
                lineCount++;
                DEBUGLOG("[StageCreate] Read line " + std::to_string(lineCount) + ": " + line);
                vector<int> row;
                stringstream sstream(line);
                string cell;
    
                while (getline(sstream, cell, ',')) {
                    try {
                        row.push_back(stoi(cell));
                    } catch (const std::invalid_argument &error) {
                        cerr << "無効な数値: " << cell << " (" << error.what() << ")" << endl;
                    } catch (const std::out_of_range &error) {
                        cerr << "範囲外の数値: " << cell << " (" << error.what() << ")" << endl;
                    }
                }
                stageMap.push_back(row);
            }
            m_file.close();
            DEBUGLOG("[StageCreate] Finished loading data. Total lines: " + std::to_string(lineCount));
        }

    StageCreate(const StageCreate &) = delete;
    StageCreate& operator=(const StageCreate&) = delete;
};

struct TimeLoad : IComponent{
    /**
     * @brief タイムファイルストリーム
     */
    ifstream m_file;

    /*
    *  @brief 時間のデータを格納するベクター
    */
    vector<vector<int>> stageTime;

    /**
     * @brief　コンストラクタ
     */
    TimeLoad() {
        m_file.open("Assets/StageData/StageTime/stage1.csv");
        if (!m_file.is_open())
            cerr << "Error: Could not open Assets/StageData/StageTime/stage1.csv" << endl;
        else {
            loadStageTime();        
        }
    }


    void loadStageTime() {
        string line;
        while (getline(m_file, line)) {
            vector<int> row;
            stringstream sstream(line);
            string cell;

            while (getline(sstream, cell, ',')) {
                try {
                    row.push_back(stoi(cell));
                } catch (const std::invalid_argument &error) {
                    cerr << "無効な数値: " << cell << " (" << error.what() << ")" << endl;
                } catch (const std::out_of_range &error) {
                    cerr << "範囲外の数値: " << cell << " (" << error.what() << ")" << endl;
                }
            }
            stageTime.push_back(row);
        }
        m_file.close();

    }

    TimeLoad(const TimeLoad &) = delete;
    TimeLoad& operator=(const TimeLoad &) = delete;
};

/**
 * @struct LoadMove
 * @brief 移動するブロックの移動量をCSVファイルから読み込むコンポーネント
 */
struct LoadMove : IComponent {
    /**
     * @brief ファイルストリーム
     */
    ifstream m_file;

    /**
     * @brief 動くブロックのデータを格納するベクター
     */
    vector<vector<int>> moveBlock;

     /**
     * @brief　コンストラクタ
     */
    LoadMove() {
        m_file.open("Assets/StageData/UniqueObj/Move/DebugStage1/room1.csv");
        if (!m_file.is_open())
            cerr << "Error: Could not open Assets/StageData/UniqueObj/Move/DebugStage1/room1.csv" << endl;
        else {
            loadBlockMove();
        }
    }

    void loadBlockMove() {
        string line;
        while (getline(m_file, line)) {
            vector<int> row;
            stringstream sstream(line);
            string cell;

            while (getline(sstream, cell, ',')) {
                try {
                    row.push_back(stoi(cell));
                } catch (const std::invalid_argument &error) {
                    cerr << "無効な数値: " << cell << " (" << error.what() << ")" << endl;
                } catch (const std::out_of_range &error) {
                    cerr << "範囲外の数値: " << cell << " (" << error.what() << ")" << endl;
                }
            }
            moveBlock.push_back(row);
        }
        m_file.close();
    }

    LoadMove(const LoadMove &) = delete;
    LoadMove &operator=(const LoadMove &) = delete;


};

/**
 * @struct MoveBlockStatus
 * @brief 移動するブロックのステータス管理
 */
struct MoveBlockStatus : IComponent {
    int blockID;        //各ブロックのステージID
    int initPosX;       //ブロックの初期x座標
    int initPosY;       //ブロックの初期y座標
    int PosX;           //ブロックの目指すx座標
    int PosY;           //ブロックの目指すy座標
    int waittime;       //ブロックの初期待機時間
    int idletime;       //ブロックの到着後待機時間
    int movetime;       //ブロックの移動にかかる時間
};

struct UpdateMoveBlock : Behaviour {

    void UpdateMove(World& w, int blockType) {
        w.ForEach<LoadMove>([](Entity, LoadMove &loadmove) {
            for (int y = 0; y < loadmove.moveBlock.size() - 1; ++y) {
                for (int x = 0; x < loadmove.moveBlock.size() - 1; ++x) {

                    int posx = static_cast<int>(loadmove.moveBlock[0][y]);
                    int posy = static_cast<int>(loadmove.moveBlock[1][y]);
                    int waittime = static_cast<int>(loadmove.moveBlock[2][y]);
                    int idletime = static_cast<int>(loadmove.moveBlock[3][y]);
                    int movetime = static_cast<int>(loadmove.moveBlock[4][y]);
                }
                
            }

        });

         w.ForEach<StageCreate>([](Entity, StageCreate &stagecreate) {
            for (int y = 0; y < stagecreate.stageMap.size(); ++y) {
                for (int x = 0; x < stagecreate.stageMap[y].size(); ++x) {

                    if (stagecreate.stageMap[x][y] >= 30 && stagecreate.stageMap[x][y] < 40) {
                    
                    }
                }
            }
        });
    }

};

/**
 * @struct LoadAngle
 * @brief 加速板の角度を取得する 
 */
struct LoadAngle : IComponent {
    /**
     * @brief アングルファイルストリーム
     */
    vector<vector<int>> stageAngle;

    LoadAngle() = default;
    LoadAngle(const LoadAngle &) = default;
    LoadAngle &operator=(const LoadAngle &) = default;
};

/**
 * @struct DashBoardAngle
 * @brief 加速板のステータス管理
 */
struct DashBoardStatus : IComponent {
    int blockID = 0;        //各ブロックのステージID
    float accelAngle = 0.0f;     //加速の方向
};

/**
 * @struct MovingObstaclePattern
 * @brief 動く障害物の往復パターン情報
 */
struct MovingObstaclePattern {
    float dirX = 0.0f;
    float dirY = 0.0f;
    float waitAtStart = 0.0f; ///< ゲーム開始時/始点での待機時間
    float waitAtEnd = 0.0f;   ///< 終点での停止時間
    float travelTime = 0.0f;  ///< 始点→終点（または終点→始点）にかける時間
};

/**
 * @struct LoadMovingObstacle
 * @brief 動く障害物のCSVから読み取ったパターンを保持
 */
struct LoadMovingObstacle : IComponent {
    std::vector<MovingObstaclePattern> patterns;

    LoadMovingObstacle() = default;
    LoadMovingObstacle(const LoadMovingObstacle &) = default;
    LoadMovingObstacle &operator=(const LoadMovingObstacle &) = default;
};

/**
 * @struct GoalAttractor
 * @brief ゴール中心へプレイヤーを一定時間で吸引する演出用Behaviour
 */
struct GoalAttractor : Behaviour {
    DirectX::XMFLOAT3 target{0.0f, 0.0f, 0.0f};
    float duration = 0.15f;                       ///< 吸引にかける時間(秒)
    float elapsed = 0.0f;                        ///< 経過時間
    DirectX::XMFLOAT3 startPos{0.0f, 0.0f, 0.0f};///< 開始位置

    void OnStart(World &w, Entity self) override {
        if (auto *t = w.TryGet<Transform>(self)) {
            startPos = t->position;
        }
    }

    void OnUpdate(World &w, Entity self, float dt) override {
        auto *t = w.TryGet<Transform>(self);
        if (!t) return;

        elapsed += std::max(0.0f, dt);
        float tNorm = duration > 1e-6f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        // ちょっと柔らかいイージング（smoothstep）
        float u = tNorm * tNorm * (3.0f - 2.0f * tNorm);

        DirectX::XMFLOAT3 pos{
            startPos.x + (target.x - startPos.x) * u,
            startPos.y + (target.y - startPos.y) * u,
            startPos.z + (target.z - startPos.z) * u
        };
        t->position = pos;

        // 進行完了: 次のルームへ進めるリクエストを立てる
        if (tNorm >= 1.0f) {
            // ゴールイン直後に制限時間をフルに戻して次ルームへ備える
            w.ForEach<GameStatus>([](Entity, GameStatus &stats) {
                stats.elapsedTime = cfg_LimitTime;
                stats.timerRunning = false;
                stats.waitingForPlayerMove = true;
            });
            w.ForEach<StageProgress>([](Entity, StageProgress &sp) {
                sp.requestAdvance = true;
            });
            // 自身のBehaviourを取り外す
            w.Remove<GoalAttractor>(self);
        }
    }
};

/**
 * @struct MovingObstacle
 * @brief 動く障害物の挙動を制御するコンポーネント
 */
struct MovingObstacle : Behaviour {
    DirectX::XMFLOAT3 startPos{};
    DirectX::XMFLOAT3 endPos{};
    DirectX::XMFLOAT3 delta{};
    DirectX::XMFLOAT3 baseScale{1.0f, 1.0f, 1.0f};
    float waitAtStart = 0.0f;
    float waitAtEnd = 0.0f;
    float travelTime = 1.0f;
    float timer = 0.0f;
    bool firstLoop = true;
    enum class State { WaitStart, MoveForward, WaitEnd, MoveBack } state = State::WaitStart;

    void OnUpdate(World &w, Entity self, float dt) override {
        auto *t = w.TryGet<Transform>(self);
        if (!t) return;

        // スケールが他所で変更されないよう固定
        t->scale = baseScale;

        timer += dt;

        auto lerpVec = [](const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b, float r) {
            return DirectX::XMFLOAT3{
                a.x + (b.x - a.x) * r,
                a.y + (b.y - a.y) * r,
                a.z + (b.z - a.z) * r};
        };

        switch (state) {
            case State::WaitStart:
                if (timer >= (firstLoop ? waitAtStart : waitAtEnd)) {
                    timer = 0.0f;
                    firstLoop = false;
                    state = State::MoveForward;
                }
                break;
            case State::MoveForward: {
                float ratio = std::clamp(timer / std::max(travelTime, 0.0001f), 0.0f, 1.0f);
                t->position = lerpVec(startPos, endPos, ratio);
                if (timer >= travelTime) {
                    timer = 0.0f;
                    state = State::WaitEnd;
                    t->position = endPos;
                }
                break;
            }
            case State::WaitEnd:
                if (timer >= waitAtEnd) {
                    timer = 0.0f;
                    state = State::MoveBack;
                }
                break;
            case State::MoveBack: {
                float ratio = std::clamp(timer / std::max(travelTime, 0.0001f), 0.0f, 1.0f);
                t->position = lerpVec(endPos, startPos, ratio);
                if (timer >= travelTime) {
                    timer = 0.0f;
                    state = State::WaitStart;
                    t->position = startPos;
                }
                break;
            }
        }
    }
};






