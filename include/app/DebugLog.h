#pragma once
#include "app/BuildConfig.h"
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

// Debug log macros
#ifdef _DEBUG
#define DEBUGLOG(message) DebugLog::GetInstance().Log(message)
#define DEBUGLOG_ERROR(message) DebugLog::GetInstance().LogError(message)
#define DEBUGLOG_WARNING(message) DebugLog::GetInstance().LogWarning(message)
#define DEBUGLOG_CATEGORY(category, message) DebugLog::GetInstance().LogWithCategory(category, message)
#else
#define DEBUGLOG(message) ((void)0)
#define DEBUGLOG_ERROR(message) ((void)0)
#define DEBUGLOG_WARNING(message) ((void)0)
#define DEBUGLOG_CATEGORY(category, message) ((void)0)
#endif

/**
 * @class DebugLog
 * @brief デバッグログユーティリティクラス
 * @author 山内陽
 * @date 2025
 *
 * @details
 * UTF-8 BOM対応、カテゴリ分類、スレッドID記録、フレーム計測を備えた
 * 強化されたデバッグログシステム
 */
class DebugLog {
public:
    /**
     * @enum Category
     * @brief ログのカテゴリ
     */
    enum class Category {
        General,   ///< 一般
        ECS,       ///< ECSシステム
        Render,    ///< レンダリング
        Input,     ///< 入力
        Graphics,  ///< グラフィックス
        Scene,     ///< シーン
        System,     ///< システム
        Game       ///< ゲームロジック
    };

    static DebugLog& GetInstance() {
        static DebugLog instance;
        return instance;
    }

    // フレームタグ設定（オプション）。メインスレッドからフレームごとに1回呼び出すことを想定。
    void SetFrame(uint64_t frame) {
        currentFrame_.store(frame, std::memory_order_relaxed);
    }

    // フレーム計測開始
    void BeginFrameTiming() {
        frameStartTime_ = std::chrono::high_resolution_clock::now();
    }

    // フレーム計測終了（Δt記録）
    void EndFrameTiming(float deltaTime) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - frameStartTime_;

        frameCount_++;
        totalTime_ += deltaTime;

        // 直近100フレームの移動平均
        recentFrameTimes_[frameCount_ % 100] = deltaTime;

        // 1000フレームごとに統計を出力
        if (frameCount_ % 1000 == 0) {
            float avgDt = totalTime_ / frameCount_;
            float avgFps = (avgDt > 0.0f) ? (1.0f / avgDt) : 0.0f;

            std::lock_guard<std::mutex> lock(mutex_);
            WriteLog("INFO", "フレーム統計", Category::System,
                     "Frames=" + std::to_string(frameCount_) +
                     ", AvgFPS=" + std::to_string(avgFps) +
                     ", AvgDt=" + std::to_string(avgDt * 1000.0f) + "ms");
        }
    }

    void Log(const std::string& message) {
#ifdef _DEBUG
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("INFO", message, Category::General);
#else
        (void)message;
#endif
    }

    void LogError(const std::string& message) {
#ifdef _DEBUG
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("ERROR", message, Category::General);
#else
        (void)message;
#endif
    }

    void LogWarning(const std::string& message) {
#ifdef _DEBUG
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("WARNING", message, Category::General);
#else
        (void)message;
#endif
    }

    void LogWithCategory(Category cat, const std::string& message) {
#ifdef _DEBUG
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("INFO", message, cat);
#else
        (void)cat;
        (void)message;
#endif
    }

    /**
     * @brief 終了時統計を出力
     */
    void OutputShutdownStatistics(std::ostream& out) {
        if (frameCount_ == 0) return;

        float avgDt = totalTime_ / frameCount_;
        float avgFps = (avgDt > 0.0f) ? (1.0f / avgDt) : 0.0f;

        // 直近100フレームの平均を計算
        float recentSum = 0.0f;
        int recentValidCount = 0;
        for (int i = 0; i < 100; ++i) {
            if (recentFrameTimes_[i] > 0.0f) {
                recentSum += recentFrameTimes_[i];
                recentValidCount++;
            }
        }
        float recentAvgDt = (recentValidCount > 0) ? (recentSum / recentValidCount) : 0.0f;
        float recentAvgFps = (recentAvgDt > 0.0f) ? (1.0f / recentAvgDt) : 0.0f;

        std::ostringstream msg;
        msg << "Summary: frames=" << frameCount_
            << ", totalTimeSec=" << std::fixed << std::setprecision(2) << totalTime_
            << ", avgFps=" << avgFps
            << ", avgMs=" << (avgDt * 1000.0f)
            << ", recentAvgFps=" << recentAvgFps
            << ", recentAvgMs=" << (recentAvgDt * 1000.0f);

        out << GetTimestampString() << ','
            << frameCount_ << ','
            << EscapeCsv("main") << ','
            << EscapeCsv("System") << ','
            << EscapeCsv("STATS") << ','
            << EscapeCsv(msg.str()) << ','
            << EscapeCsv("") << '\n';
    }

private:
    DebugLog() = default;

    ~DebugLog() {
#ifdef _DEBUG
        std::lock_guard<std::mutex> lock(mutex_);
        FlushBufferedLogsLocked(true);
#endif
    }

    const char* CategoryToString(Category cat) const {
        switch (cat) {
            case Category::ECS: return "ECS";
            case Category::Render: return "Render";
            case Category::Input: return "Input";
            case Category::Graphics: return "Graphics";
            case Category::Scene: return "Scene";
            case Category::System: return "System";
            default: return "General";
        }
    }

    std::string GetTimestampString() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm bt{};
        localtime_s(&bt, &in_time_t);

        std::ostringstream ts;
        ts << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
        return ts.str();
    }

    std::string EscapeCsv(const std::string& value) const {
        std::string escaped = value;
        for (size_t pos = 0; (pos = escaped.find('"', pos)) != std::string::npos; pos += 2) {
            escaped.insert(pos, 1, '"');
        }
        if (escaped.find_first_of(",\n\r") != std::string::npos) {
            escaped = "\"" + escaped + "\"";
        }
        return escaped;
    }

    void WriteLog(const std::string& level, const std::string& message, Category cat, const std::string& extra = "") {
#ifdef _DEBUG
        // フレームプレフィックス
        uint64_t frame = currentFrame_.load(std::memory_order_relaxed);

        // スレッドID取得
        auto threadId = std::this_thread::get_id();
        std::ostringstream tidOss;
        tidOss << threadId;

        std::ostringstream line;
        line << GetTimestampString() << ','
             << frame << ','
             << EscapeCsv(tidOss.str()) << ','
             << EscapeCsv(CategoryToString(cat)) << ','
             << EscapeCsv(level) << ','
             << EscapeCsv(message) << ','
             << EscapeCsv(extra);

        bufferedLogs_.emplace_back(line.str());
        if (bufferedLogs_.size() >= flushThreshold_) {
            FlushBufferedLogsLocked(false);
        }
#else
        (void)level;
        (void)message;
        (void)cat;
        (void)extra;
#endif
    }

    void FlushBufferedLogsLocked(bool finalFlush) {
#ifdef _DEBUG
        if (bufferedLogs_.empty() && !finalFlush) {
            return;
        }

        std::ios_base::openmode mode = std::ios::out | std::ios::binary;
        mode |= logFileInitialized_ ? std::ios::app : std::ios::trunc;

        std::ofstream file(logFilePath_, mode);
        if (!file.is_open()) {
            return;
        }

        if (!logFileInitialized_) {
            const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
            file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
            file << "timestamp,frame,thread_id,category,level,message,extra\n";
            logFileInitialized_ = true;
        }

        for (const auto& entry : bufferedLogs_) {
            file << entry << '\n';
        }
        bufferedLogs_.clear();

        if (finalFlush) {
            OutputShutdownStatistics(file);
        }

        file.flush();
#endif
    }

    DebugLog(const DebugLog&) = delete;
    DebugLog& operator=(const DebugLog&) = delete;

    std::vector<std::string> bufferedLogs_;
    const std::string logFilePath_ = "debug_log.csv";
    bool logFileInitialized_ = false;
    const size_t flushThreshold_ = DEBUGLOG_AUTO_FLUSH_THRESHOLD;
    std::mutex mutex_;
    std::atomic<uint64_t> currentFrame_{0};

    // フレーム計測
    std::chrono::high_resolution_clock::time_point frameStartTime_;
    uint64_t frameCount_ = 0;
    float totalTime_ = 0.0f;
    float recentFrameTimes_[100] = {};
};
