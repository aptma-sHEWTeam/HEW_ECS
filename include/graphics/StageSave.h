#pragma once
#include <fstream>
#include <iostream>
#include <filesystem>

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
};

class StageSave {
  public:
    // ファイルから読み込み
    static inline void Load() {
        std::ifstream ifs("Assets/Save/stage.txt");
        if (!ifs)
            return;

        std::string line;
        if (std::getline(ifs, line)) {
            try {
                s_data.maxClearedPss = std::stoi(line);
            } catch (...) {
                s_data.maxClearedPss = 0; 
            }
        }
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

    // データ削除
    static inline void Delete() {
        s_data.maxClearedPss = 0;
        if (std::filesystem::exists("Assets/Save/stage.txt"))
            std::filesystem::remove("Assets/Save/stage.txt");
    }

  private:
    static inline StageClearData s_data{};

    // 保存
    static inline void Save() {
        std::filesystem::create_directories("Assets/Save");
        std::ofstream ofs("Assets/Save/stage.txt", std::ios::out);
        if (!ofs) {
            std::cerr << "StageSave: ファイルを開けませんでした！\n";
            return;
        }

        ofs << std::to_string(s_data.maxClearedPss) << "\n";

    }
};
