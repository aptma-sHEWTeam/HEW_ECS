#pragma once
#include <string>
#include <functional>
#include "ConfigManager.h"

// ConfigVar を汎用リストに格納するためのベースインターフェース
class IConfigVar {
public:
    virtual ~IConfigVar() = default;
    virtual std::string GetSection() const = 0;
    virtual std::string GetName() const = 0;
    virtual void SetValueFromString(const std::string& value) = 0;
    virtual std::string GetValueAsString() const = 0;
    virtual void SetValueFromBinary(const void* data) = 0;
    virtual void GetValueAsBinary(void* data) const = 0;
    virtual size_t GetBinarySize() const = 0;
    // オプション: TOML コメントを出力する際に使用される短い説明
    virtual std::string GetComment() const = 0;
};

template <typename T>
class ConfigVar : public IConfigVar {
public:
    ConfigVar(const std::string& section, const std::string& name, const T& defaultValue,
              const std::string& comment = {})
        : m_Section(section), m_Name(name), m_Value(defaultValue), m_DefaultValue(defaultValue), m_Comment(comment) {
        ConfigManager::Instance().Register(this);
    }

    // T への暗黙変換
    operator T() const { return m_Value; }

    // 代入演算子
    ConfigVar<T>& operator=(const T& value) {
        m_Value = value;
        return *this;
    }

    T Get() const { return m_Value; }

    // IConfigVar の実装
    std::string GetSection() const override { return m_Section; }
    std::string GetName() const override { return m_Name; }
    std::string GetComment() const override { return m_Comment; }

    void SetValueFromString(const std::string& value) override {
        if constexpr (std::is_same_v<T, int>) {
            m_Value = std::stoi(value);
        } else if constexpr (std::is_same_v<T, float>) {
            m_Value = std::stof(value);
        } else if constexpr (std::is_same_v<T, bool>) {
            m_Value = (value == "true" || value == "1");
        } else if constexpr (std::is_same_v<T, std::string>) {
            m_Value = value;
        }
    }

    std::string GetValueAsString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return m_Value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return m_Value ? "true" : "false";
        } else {
            return std::to_string(m_Value);
        }
    }

    void SetValueFromBinary(const void* data) override {
        if constexpr (std::is_same_v<T, std::string>) {
            // 現在バイナリではサポートされていない
        } else {
            m_Value = *static_cast<const T*>(data);
        }
    }

    void GetValueAsBinary(void* data) const override {
         if constexpr (!std::is_same_v<T, std::string>) {
            *static_cast<T*>(data) = m_Value;
         }
    }

    size_t GetBinarySize() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return 0;
        } else {
            return sizeof(T);
        }
    }

private:
    std::string m_Section;
    std::string m_Name;
    T m_Value;
    T m_DefaultValue;
    std::string m_Comment;
};
