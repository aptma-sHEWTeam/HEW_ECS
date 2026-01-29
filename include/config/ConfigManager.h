#pragma once
#include <vector>
#include <string>
#include <filesystem>

/**
 * @file ConfigManager.h
 * @brief TOML形式を使用した設定管理システム
 * @details デバッグビルドとリリースビルドの両方で常にTOMLから読み込みます。
 *          ランタイム設定変更のためのホットリロードはすべてのビルドでアクティブです。
 */

class IConfigVar;

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Register(IConfigVar* var);
    void Initialize(const std::string& assetPath);
    void Update(); // ホットリロードのために毎フレーム呼び出します
    void ForceReload(); // TOMLを手動で再読み込みします
    void ExportBinary(const std::string& path); // 将来の最適化のために

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    void Load(); // 内部読み込みメソッド
    void LoadTOML();
    void LoadBinary();
    void SaveTOML();
    
    void ParseLine(const std::string& line, std::string& currentSection);

    std::vector<IConfigVar*> m_Vars;
    std::string m_AssetPath;
    std::filesystem::file_time_type m_LastWriteTime;
    bool m_IsDirty = false;
    bool m_IsDebug = true; // 常にtrue - すべてのビルドでホットリロードが有効
};
