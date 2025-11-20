#pragma once
#include <string>
#include <functional>
#include "ConfigManager.h"

// Base interface for ConfigVars to allow storage in a generic list
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
};

template <typename T>
class ConfigVar : public IConfigVar {
public:
    ConfigVar(const std::string& section, const std::string& name, const T& defaultValue)
        : m_Section(section), m_Name(name), m_Value(defaultValue), m_DefaultValue(defaultValue) {
        ConfigManager::Instance().Register(this);
    }

    // Implicit conversion to T
    operator T() const { return m_Value; }

    // Assignment operator
    ConfigVar<T>& operator=(const T& value) {
        m_Value = value;
        return *this;
    }

    T Get() const { return m_Value; }

    // IConfigVar implementation
    std::string GetSection() const override { return m_Section; }
    std::string GetName() const override { return m_Name; }

    void SetValueFromString(const std::string& value) override {
        // Specializations or standard conversions would go here
        // For now, we'll handle basic types in the cpp or via specializations if needed
        // But since this is a header-only template part, we need to be careful.
        // Actually, it's better to delegate the string parsing to the Manager or helper
        // to avoid cluttering this header, but for simplicity in this task:
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
            // Strings are tricky in simple binary dumps without length prefix
            // For this simple implementation, we might need a fixed size or length prefix
            // Let's assume the binary format handles strings differently or we skip them for now in binary
            // Or better: The manager handles the reading and passes the pointer.
            // For simplicity, let's assume we don't support binary strings yet or handle them in Manager.
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
            return 0; // Dynamic size not supported in this simple fixed-struct binary approach yet
        } else {
            return sizeof(T);
        }
    }

private:
    std::string m_Section;
    std::string m_Name;
    T m_Value;
    T m_DefaultValue;
};
