#pragma once
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>

inline int GetPssNumber(int world, int stageInWorld) {
    static const int pssTable[4][6] = {
        {101, 102, 103, 0, 0, 0},      // ワールド1
        {201, 202, 203, 204, 0, 0},    // ワールド2
        {301, 302, 303, 304, 305, 0},  // ワールド3
        {401, 402, 403, 404, 405, 406} // ワールド4
    };

    if (world < 1 || world > 4)
        return 0;
    if (stageInWorld < 1 || stageInWorld > 6)
        return 0;
    return pssTable[world - 1][stageInWorld - 1];
}

struct StageClearData {
    int maxClearedPss = 0;
    int lastWorld = 1;   // 最後にプレイしていたワールド (1-4)
    int lastStage = 1;   // 最後にプレイしていたステージ (1-based)
    std::map<int, int> deathCounts; // PSS番号 → デス回数
    int lastSavedPss = 0;           // 最後に保存したPSS（VideoScene表示用）
    int lastSavedDeaths = 0;        // 最後に保存したデス回数（VideoScene表示用）
};

class StageSave {
  public:
    /// ファイルから読み込み（3行: maxClearedPss, lastWorld, lastStage + デスカウントファイル）
    static inline void Load() {
        std::ifstream ifs("Assets/Save/stage.txt");
        if (!ifs)
            return;

        std::string line;
        // 1行目: maxClearedPss
        if (std::getline(ifs, line)) {
            try {
                s_data.maxClearedPss = std::stoi(line);
            } catch (...) {
                s_data.maxClearedPss = 0;
            }
        }
        // 2行目: lastWorld（欠損時はデフォルト1）
        if (std::getline(ifs, line)) {
            try {
                s_data.lastWorld = std::clamp(std::stoi(line), 1, 4);
            } catch (...) {
                s_data.lastWorld = 1;
            }
        }
        // 3行目: lastStage（欠損時はデフォルト1）
        if (std::getline(ifs, line)) {
            try {
                s_data.lastStage = std::max(std::stoi(line), 1);
            } catch (...) {
                s_data.lastStage = 1;
            }
        }

        // デスカウントファイル読み込み
        LoadDeathCounts();
    }

    static inline void MarkPssCleared(int pss) {
        if (pss <= 0)
            return;

        if (pss > s_data.maxClearedPss) {
            s_data.maxClearedPss = pss;
            Save();
        }
    }

    static inline int GetMaxClearedPss() {
        return s_data.maxClearedPss;
    }

    /// 最後にプレイしていたワールド番号を返す (1-4)
    static inline int GetLastWorld() {
        return s_data.lastWorld;
    }

    /// 最後にプレイしていたステージ番号を返す (1-based)
    static inline int GetLastStage() {
        return s_data.lastStage;
    }

    /// プレイ位置を記録して保存
    static inline void SaveProgress(int world, int stage) {
        s_data.lastWorld = std::clamp(world, 1, 4);
        s_data.lastStage = std::max(stage, 1);
        Save();
    }

    /// データ削除（全フィールドをリセット）
    static inline void Delete() {
        s_data.maxClearedPss = 0;
        s_data.lastWorld = 1;
        s_data.lastStage = 1;
        s_data.deathCounts.clear();
        s_data.lastSavedPss = 0;
        s_data.lastSavedDeaths = 0;
        if (std::filesystem::exists("Assets/Save/stage.txt"))
            std::filesystem::remove("Assets/Save/stage.txt");
        if (std::filesystem::exists("Assets/Save/deaths.txt"))
            std::filesystem::remove("Assets/Save/deaths.txt");
    }

    /// 指定ワールドが解放済みか判定
    /// ワールド1は常に解放。ワールドNは前ワールドの最終ステージのPSSがクリア済みなら解放。
    static inline bool IsWorldUnlocked(int world) {
        if (world <= 1) return true;
        if (world > 4) return false;
        // 前ワールドの最終ステージのPSS番号を取得
        // ワールドごとのステージ数: W1=3, W2=4, W3=5, W4=6
        static const int stagesPerWorld[4] = {3, 4, 5, 6};
        int prevWorld = world - 1;
        int lastStageOfPrev = stagesPerWorld[prevWorld - 1];
        int requiredPss = GetPssNumber(prevWorld, lastStageOfPrev);
        return s_data.maxClearedPss >= requiredPss;
    }

    /// 指定ワールド内のステージが解放済みか判定
    /// ステージ1は常に解放（そのワールドが解放済みの場合）。
    /// ステージNは前ステージのPSSがクリア済みなら解放。
    static inline bool IsStageUnlocked(int world, int stage) {
        if (!IsWorldUnlocked(world)) return false;
        if (stage <= 1) return true;
        int prevPss = GetPssNumber(world, stage - 1);
        return s_data.maxClearedPss >= prevPss;
    }

    /// デスカウントを保存（PSS番号ごと）
    static inline void SaveDeathCount(int pss, int count) {
        if (pss <= 0) return;
        s_data.deathCounts[pss] = count;
        s_data.lastSavedPss = pss;
        s_data.lastSavedDeaths = count;
        SaveDeathCounts();
    }

    /// 指定PSSのデス回数を取得（未記録は-1）
    static inline int GetDeathCount(int pss) {
        auto it = s_data.deathCounts.find(pss);
        if (it != s_data.deathCounts.end()) return it->second;
        return -1;
    }

    /// 最後に保存したデス回数を取得（VideoScene表示用）
    static inline int GetLastSavedDeaths() {
        return s_data.lastSavedDeaths;
    }

    /// デス回数からランクを判定 (S/A/B/C)
    static inline std::string GetRank(int deaths) {
        if (deaths <= 0) return "S";
        if (deaths <= 2) return "A";
        if (deaths <= 5) return "B";
        return "C";
    }

    /// デス回数からランクをワイド文字列で取得
    static inline std::wstring GetRankW(int deaths) {
        if (deaths <= 0) return L"S";
        if (deaths <= 2) return L"A";
        if (deaths <= 5) return L"B";
        return L"C";
    }

  private:
    static inline StageClearData s_data{};

    /// 保存（3行フォーマット）
    static inline void Save() {
        std::filesystem::create_directories("Assets/Save");
        std::ofstream ofs("Assets/Save/stage.txt", std::ios::out);
        if (!ofs) {
            std::cerr << "StageSave: ファイルを開けませんでした！\n";
            return;
        }

        ofs << std::to_string(s_data.maxClearedPss) << "\n";
        ofs << std::to_string(s_data.lastWorld) << "\n";
        ofs << std::to_string(s_data.lastStage) << "\n";
    }

    /// デスカウントファイルを保存（pss:count形式）
    static inline void SaveDeathCounts() {
        std::filesystem::create_directories("Assets/Save");
        std::ofstream ofs("Assets/Save/deaths.txt", std::ios::out);
        if (!ofs) return;
        for (const auto &[pss, count] : s_data.deathCounts) {
            ofs << pss << ":" << count << "\n";
        }
    }

    /// デスカウントファイルから読み込み
    static inline void LoadDeathCounts() {
        std::ifstream ifs("Assets/Save/deaths.txt");
        if (!ifs) return;
        s_data.deathCounts.clear();
        std::string line;
        while (std::getline(ifs, line)) {
            auto colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;
            try {
                int pss = std::stoi(line.substr(0, colonPos));
                int count = std::stoi(line.substr(colonPos + 1));
                if (pss > 0) s_data.deathCounts[pss] = count;
            } catch (...) {}
        }
    }
};
