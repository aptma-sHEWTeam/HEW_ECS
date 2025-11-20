#pragma once
#include <vector>
#include <string>
#include <filesystem>

class IConfigVar;

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Register(IConfigVar* var);
    void Initialize(const std::string& assetPath);
    void Update(); // Call this every frame for hot reload
    void ForceReload(); // Manually reload TOML
    void ExportBinary(const std::string& path);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    void Load();
    void LoadTOML();
    void LoadBinary();
    void SaveTOML();
    
    // Helper to parse a single line
    void ParseLine(const std::string& line, std::string& currentSection);

    std::vector<IConfigVar*> m_Vars;
    std::string m_AssetPath;
    std::filesystem::file_time_type m_LastWriteTime;
    bool m_IsDirty = false; // If we need to save back to TOML
    bool m_IsDebug = true; // Toggle based on build config
};
