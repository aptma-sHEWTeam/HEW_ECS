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
#include "config/ConfigVar.h"
#include "app/DebugLog.h" // DEBUGLOG_ERRORのために追加

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
    bool requestAdvance = false;
};

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
     * @brief ステージデータを格納する2次元ベクター
     */
    vector<vector<int>> stageMap;

    /**
     * @brief コンストラクタ
     * @details CSVファイルをオープンし、データを読み込む
     */
        explicit StageCreate(const std::string& csvPath) {
            DEBUGLOG("[StageCreate] Constructor called with path: " + csvPath);
            if (csvPath.empty()) {
                DEBUGLOG_ERROR("[StageCreate] Stage CSV path is empty.");
                return;
            }
            m_file.open(csvPath);
            if (!m_file.is_open()) {
                DEBUGLOG_ERROR("[StageCreate] Failed to open CSV file: " + csvPath);
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
    }

};




