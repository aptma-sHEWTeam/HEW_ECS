/**
 * @file Random.h
 * @brief 高品質な乱数を簡単に使うためのユーティリティ
 * @author 山内陽
 * @version 1.0
 *
 * @details
 * Cの rand()/srand() の代わりに、C++の <random> を使った高品質な乱数生成を提供します。
 * 使い方はシンプルで、`Random::Float(min, max)` や `Random::Int(min, max)` を呼ぶだけです。
 * 必要に応じて `Random::Seed(seed)` でシード固定も可能です（リプレイ再現などに便利）。
 *
 * - スレッドローカルな `std::mt19937` を内部で使用
 * - 追加コストなしで安全に複数箇所から利用可能
 * - C++14対応（ヘッダオンリー）
 */
#pragma once

#include <random>
#include <cstdint>
#include <limits>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
#include <string>

#include "app/DebugLog.h"

namespace util {

class Random {
public:
    // 任意の固定シードで初期化（現在のスレッドのエンジンを再シード）
    static void Seed(uint32_t seed) {
        Engine().seed(seed);
        DEBUGLOG_CATEGORY(DebugLog::Category::System, "Randomのシード設定： " + std::to_string(seed));
    }

    // 現在時刻でシード（簡易）
    static void SeedTime() {
        uint32_t seed = static_cast<uint32_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count() & 0xFFFFFFFFULL);
        Seed(seed);
    }

    // [min, max] の実数一様乱数（閉区間）
    static float Float(float minInclusive, float maxInclusive) {
        if (minInclusive > maxInclusive) std::swap(minInclusive, maxInclusive);
        std::uniform_real_distribution<float> dist(minInclusive, nextUp(maxInclusive));
        return dist(Engine());
    }

    // [min, max] の整数一様乱数（閉区間）
    static int Int(int minInclusive, int maxInclusive) {
        if (minInclusive > maxInclusive) std::swap(minInclusive, maxInclusive);
        std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
        return dist(Engine());
    }

    // true を返す確率 p（0..1）
    static bool Bool(float p = 0.5f) {
        if (p <= 0.0f) return false;
        if (p >= 1.0f) return true;
        std::bernoulli_distribution dist(p);
        return dist(Engine());
    }

    // 正規分布 N(mean, stddev)
    static float Normal(float mean = 0.0f, float stddev = 1.0f) {
        if (stddev <= 0.0f) return mean;
        std::normal_distribution<float> dist(mean, stddev);
        return dist(Engine());
    }

    // [0,1] の明るめカラー（0.33～1.0）
    static DirectX::XMFLOAT3 ColorBright() {
        return DirectX::XMFLOAT3{ Float(0.33f, 1.0f), Float(0.33f, 1.0f), Float(0.33f, 1.0f) };
    }

    // [min,max] のカラー
    static DirectX::XMFLOAT3 Color(float minInclusive = 0.0f, float maxInclusive = 1.0f) {
        return DirectX::XMFLOAT3{ Float(minInclusive, maxInclusive),
                                   Float(minInclusive, maxInclusive),
                                   Float(minInclusive, maxInclusive) };
    }

    // 一様な単位ベクトル（近似）
    static DirectX::XMFLOAT3 UnitVec3() {
        // 球面一様分布（角度から生成）
        const float z = Float(-1.0f, 1.0f);
        const float t = Float(0.0f, 6.28318530718f); // 2π
        const float r = std::sqrt((std::max)(0.0f, 1.0f - z * z));
        return DirectX::XMFLOAT3{ r * std::cos(t), r * std::sin(t), z };
    }

private:
    // スレッドローカルなエンジン（最初の使用時にランダムデバイスで初期化）
    static std::mt19937& Engine() {
        thread_local std::mt19937 rng(seedFromDevice());
        return rng;
    }

    static uint32_t seedFromDevice() {
        std::random_device rd;
        // 一部環境で rd() が低品質な場合もあるらしいため、複数回混合
        uint32_t s = rd();
        s ^= (rd() << 1);
        s ^= (rd() << 2);
        return s;
    }

    // 上方向の隣接浮動小数（閉区間を実現するための微小オフセット）
    static float nextUp(float x) {
        if (std::isinf(x) && x > 0) return x;
        if (std::isnan(x)) return x;
        return std::nextafter(x, std::numeric_limits<float>::infinity());
    }
};

} // namespace util
