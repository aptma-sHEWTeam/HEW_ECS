#include "config/ConfigManager.h"
#include "config/ConfigVar.h"
#include "app/DebugLog.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <optional>

namespace fs = std::filesystem;

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::Register(IConfigVar* var) {
    m_Vars.push_back(var);
}

void ConfigManager::Initialize(const std::string& assetPath) {
    m_AssetPath = assetPath;
    // Always enable hot reload in both debug and release builds
    m_IsDebug = true;

    // Always load TOML regardless of build type
    LoadTOML();
    // If config.toml was missing or some vars were missing, m_IsDirty will be true.
    // In that case we must create/save config.toml unconditionally so it always exists.
    if (m_IsDirty) {
        SaveTOML();
        m_IsDirty = false;
    }
}

void ConfigManager::Update() {
    if (!m_IsDebug) return;

    fs::path tomlPath = fs::path(m_AssetPath) / "Settings/config.toml";
    if (!fs::exists(tomlPath)) {
        // If the config file is missing at any time, recreate it from current vars.
        m_IsDirty = true;
        SaveTOML();
        m_IsDirty = false;
        return;
    }

    try {
        auto currentWriteTime = fs::last_write_time(tomlPath);
        if (currentWriteTime > m_LastWriteTime) {
            // Debounce slightly or just load
            LoadTOML();
            m_LastWriteTime = currentWriteTime;
            std::cout << "[ConfigManager] Hot reloaded config.toml" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] Error checking file time: " << e.what() << std::endl;
    }
}

void ConfigManager::ForceReload() {
    LoadTOML();
    std::cout << "[ConfigManager] Force reloaded config.toml" << std::endl;
}

void ConfigManager::Load() {
    // Deprecated - now directly called from Initialize
    LoadTOML();
    if (m_IsDirty) {
        SaveTOML();
        m_IsDirty = false;
    }
}

// Helper to trim whitespace
static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::optional<fs::path> FindConfigToml(const fs::path& preferred) {
    if (fs::exists(preferred)) return preferred;

    fs::path cur = fs::current_path();
    for (int i = 0; i < 5; ++i) {
        fs::path candidate = cur / "Assets" / "Settings" / "config.toml";
        if (fs::exists(candidate)) return candidate;
        if (!cur.has_parent_path()) break;
        cur = cur.parent_path();
    }
    return std::nullopt;
}

void ConfigManager::LoadTOML() {
    fs::path preferred = fs::path(m_AssetPath) / "Settings" / "config.toml";
    auto resolved = FindConfigToml(preferred);
    if (!resolved) {
        DEBUGLOG_WARNING("[ConfigManager] config.toml が見つかりませんでした。現在の作業ディレクトリ付近を探索しましたが失敗しました。");
        // Mark dirty so caller can decide to save defaults. Do not early-save here to
        // keep responsibilities of loading vs saving separate.
        m_IsDirty = true;
        return;
    }
    fs::path tomlPath = *resolved;
    // 見つけたパスに合わせて AssetPath を補正
    m_AssetPath = tomlPath.parent_path().parent_path().string();

    std::ifstream file(tomlPath);
    if (!file.is_open()) {
        // File path exists but couldn't be opened; mark dirty so we can try to recreate it.
        m_IsDirty = true;
        return;
    }

    std::string line;
    std::string currentSection = "";
    std::map<std::string, std::map<std::string, std::string>> parsedData;

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
        } else {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = Trim(line.substr(0, eqPos));
                std::string value = Trim(line.substr(eqPos + 1));
                // Remove quotes if string
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                parsedData[currentSection][key] = value;
            }
        }
    }

    m_LastWriteTime = fs::last_write_time(tomlPath);

    // Apply to vars
    bool missingVars = false;
    for (auto* var : m_Vars) {
        if (parsedData.count(var->GetSection()) && parsedData[var->GetSection()].count(var->GetName())) {
            var->SetValueFromString(parsedData[var->GetSection()][var->GetName()]);
        } else {
            missingVars = true;
        }
    }

    if (missingVars) {
        m_IsDirty = true;
    }
}

void ConfigManager::SaveTOML() {
    fs::path tomlPath = fs::path(m_AssetPath) / "Settings/config.toml";
    std::error_code ec;
    fs::create_directories(tomlPath.parent_path(), ec);
    if (ec) {
        DEBUGLOG_ERROR(std::string("[ConfigManager] 設定ディレクトリの作成に失敗しました: ") + ec.message());
        return;
    }

    std::ofstream file(tomlPath);
    if (!file.is_open()) return;

    // Group by section
    std::map<std::string, std::vector<IConfigVar*>> sections;
    for (auto* var : m_Vars) {
        sections[var->GetSection()].push_back(var);
    }

    for (const auto& [sectionName, vars] : sections) {
        file << "[" << sectionName << "]\n";
        for (auto* var : vars) {
            std::string val = var->GetValueAsString();
            // Quote strings if needed (simple check)
            // For this simple parser, we assume strings need quotes if they aren't numbers/bools
            // But GetValueAsString implementation for string already returns raw string.
            // We should probably add quotes for string types.
            // Since we don't have type info easily here without casting, let's rely on the fact
            // that SetValueFromString handles unquoting.
            // A robust way would be to have GetValueAsTOMLString in interface.
            // For now, let's just write it. If it's a string type, we might want to quote it.
            // But wait, GetValueAsString for bool returns "true"/"false".
            // Let's just write it as is. If it's a string value "hello", it writes hello.
            // The parser handles quotes. We should add them if it's a string.
            // For simplicity, let's just write raw.

            // Actually, to be safe with our parser:
            // If the value contains spaces and isn't quoted, our parser might be fine since we take everything after =
            // But standard TOML requires quotes for strings.
            // Let's try to detect if it's a number or bool, otherwise quote.
            bool isNumber = !val.empty() && val.find_first_not_of("0123456789.-") == std::string::npos;
            bool isBool = val == "true" || val == "false";

            if (!isNumber && !isBool) {
                file << var->GetName() << " = \"" << val << "\"\n";
            } else {
                file << var->GetName() << " = " << val << "\n";
            }
        }
        file << "\n";
    }

    // Update time so we don't hot-reload our own save
    file.close();
    m_LastWriteTime = fs::last_write_time(tomlPath);
}

void ConfigManager::LoadBinary() {
    fs::path binPath = fs::path(m_AssetPath) / "config.bin";
    std::ifstream file(binPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Failed to load binary config: " << binPath << std::endl;
        return;
    }

    // Simple format: [SectionHash][NameHash][Size][Data]... or just sequential if order is deterministic.
    // Since registration order might vary across compilers/linkers (global constructors),
    // we cannot rely on order. We need identifiers.
    // To keep it simple and robust:
    // [SectionLen][SectionStr][NameLen][NameStr][DataLen][Data]

    while (file.peek() != EOF) {
        size_t len;
        if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        std::string section(len, '\0');
        file.read(&section[0], len);

        if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        std::string name(len, '\0');
        file.read(&name[0], len);

        size_t dataSize;
        if (!file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize))) break;

        std::vector<char> buffer(dataSize);
        file.read(buffer.data(), dataSize);

        // Find the var
        for (auto* var : m_Vars) {
            if (var->GetSection() == section && var->GetName() == name) {
                if (var->GetBinarySize() == dataSize) {
                    var->SetValueFromBinary(buffer.data());
                }
                break;
            }
        }
    }
}

void ConfigManager::ExportBinary(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return;

    for (auto* var : m_Vars) {
        std::string section = var->GetSection();
        size_t len = section.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(section.c_str(), len);

        std::string name = var->GetName();
        len = name.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(name.c_str(), len);

        size_t dataSize = var->GetBinarySize();
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

        // This is a bit hacky for types that aren't POD, but for int/float/bool it works.
        // For strings, we returned size 0 in header, so it won't write anything.
        if (dataSize > 0) {
            std::vector<char> buffer(dataSize);
            var->GetValueAsBinary(buffer.data());
            file.write(buffer.data(), dataSize);
        }
    }
    std::cout << "[ConfigManager] Exported binary config to " << path << std::endl;
}
