#pragma once
#include <vector>
#include <string>
#include <filesystem>

/**
 * @file ConfigManager.h
 * @brief Configuration management system using TOML format
 * @details Always loads from TOML in both debug and release builds.
 *          Hot reload is active in all builds for runtime configuration changes.
 */

class IConfigVar;

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Register(IConfigVar* var);
    void Initialize(const std::string& assetPath);
    void Update(); // Call this every frame for hot reload
    void ForceReload(); // Manually reload TOML
    void ExportBinary(const std::string& path); // For future optimization

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    void Load(); // Internal load method
    void LoadTOML();
    void LoadBinary(); // Reserved for future use
    void SaveTOML();
    
    void ParseLine(const std::string& line, std::string& currentSection);

    std::vector<IConfigVar*> m_Vars;
    std::string m_AssetPath;
    std::filesystem::file_time_type m_LastWriteTime;
    bool m_IsDirty = false;
    bool m_IsDebug = true; // Always true - hot reload enabled for all builds
};
