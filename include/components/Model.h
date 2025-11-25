#pragma once
#include <string>

/**
 * @file Model.h
 * @brief モデルファイルパスを保持するコンポーネント
 * @author 山内陽
 * @date 2025
 * @version 6.0
 */

struct Model {
    std::string filePath;

    Model() = default;
    explicit Model(const std::string& path) : filePath(path) {}
    explicit Model(std::string&& path) : filePath(std::move(path)) {}
};
