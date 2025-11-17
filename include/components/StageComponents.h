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
    bool requestAdvance = false;
};

/**
 * @struct StageCreate
 * @brief CSVファイルからデータを読み込み、ステージを生成
 */
struct StageCreate : IComponent {
    /**
     * @brief ファイルストリーム
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
    StageCreate() {
        m_file.open("Assets/StageData/aaa.csv");
        if (!m_file.is_open()) {
            cerr << "ファイルのオープンに失敗しました。" << endl;
        } else {
            loadStageData();
        }
    }

    /**
     * @brief ステージデータを読み込む
     * @details CSVファイルからデータをパースし、stageMapに格納
     */
    void loadStageData() {
        string line;
        while (getline(m_file, line)) {
            vector<int> row;
            stringstream sstream(line);
            string cell;

            while (getline(sstream, cell, ',')) {
                try {
                    row.push_back(stoi(cell));
                } catch (const std::invalid_argument &error) {
                    cerr << "無効な数値: " << cell << endl;
                } catch (const std::out_of_range &error) {
                    cerr << "範囲外の数値: " << cell << endl;
                }
            }
            stageMap.push_back(row);
        }
        m_file.close();
    }

    StageCreate(const StageCreate &) = delete;
    StageCreate& operator=(const StageCreate&) = delete;
};





