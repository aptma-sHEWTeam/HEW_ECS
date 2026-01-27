#pragma once

struct StageClearData {
    int maxClearedStage = 0;
};

class StageSave {
  public:
    static inline void Load() {
        std::ifstream ifs("Save/stage.dat", std::ios::binary);
        if (ifs) {
            ifs.read(reinterpret_cast<char *>(&s_data), sizeof(StageClearData));
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
        if (std::filesystem::exists("Save/stage.dat")) {
            std::filesystem::remove("Save/stage.dat");
        }
    }


    static inline int GetMaxClearedStage() {
        return s_data.maxClearedStage;
    }

  private:
    static inline StageClearData s_data{};

    static inline void Save() {
        std::filesystem::create_directories("Save");
        std::ofstream ofs("Save/stage.dat", std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(&s_data),
                  sizeof(StageClearData));
    }
};