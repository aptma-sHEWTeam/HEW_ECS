#pragma once

#include <filesystem>
#include <fstream>

struct StageClearData {
    int maxClearedStage = 0;
};

class StageSave {
  public:
    static inline void Load() {
        std::ifstream ifs("Assets/Save/stage.dat", std::ios::binary);
        if (ifs) {
            StageClearData tmp{};
            ifs.read(reinterpret_cast<char *>(&tmp), sizeof(StageClearData));
            if (ifs.gcount() == static_cast<std::streamsize>(sizeof(StageClearData))) {
                s_data = tmp;
            }
        }
    }

    static inline void MarkStageCleared(int stage) {
        if (stage <= 0)
            return;

        if (stage > s_data.maxClearedStage) {
            s_data.maxClearedStage = stage;
            Save();
        }
    }
    static inline void Delete() {
        s_data.maxClearedStage = 0;
        if (std::filesystem::exists("Assets/Save/stage.dat")) {
            std::filesystem::remove("Assets/Save/stage.dat");
        }
    }


    static inline int GetMaxClearedStage() {
        return s_data.maxClearedStage;
    }

  private:
    static inline StageClearData s_data{};

    static inline void Save() {
        std::filesystem::create_directories("Assets/Save");
        std::ofstream ofs("Assets/Save/stage.dat", std::ios::binary);
        if (!ofs) {
            return;
        }
        ofs.write(reinterpret_cast<const char *>(&s_data), sizeof(StageClearData));
    }
};