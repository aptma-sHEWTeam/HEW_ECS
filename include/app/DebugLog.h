#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <atomic>

// Debug log macros
#ifdef _DEBUG
#define DEBUGLOG(message) DebugLog::GetInstance().Log(message)
#define DEBUGLOG_ERROR(message) DebugLog::GetInstance().LogError(message)
#define DEBUGLOG_WARNING(message) DebugLog::GetInstance().LogWarning(message)
#else
#define DEBUGLOG(message) ((void)0)
#define DEBUGLOG_ERROR(message) ((void)0)
#define DEBUGLOG_WARNING(message) ((void)0)
#endif

/**
 * @class DebugLog
 * @brief �f�o�b�O���O���[�e�B���e�B�N���X
 * @author �R���z
 * @date 2025
 */
class DebugLog {
public:
    static DebugLog& GetInstance() {
        static DebugLog instance;
        return instance;
    }

    // �t���[���^�O�ݒ�i�I�v�V�����j�B���C���X���b�h����t���[�����Ƃ�1��Ăяo�����Ƃ�z��B
    void SetFrame(uint64_t frame) {
        currentFrame_.store(frame, std::memory_order_relaxed);
    }

    void Log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("INFO", message);
    }

    void LogError(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("ERROR", message);
    }

    void LogWarning(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        WriteLog("WARNING", message);
    }

private:
    DebugLog() {
        logFile_.open("debug_log.txt", std::ios::out | std::ios::trunc);
        if (logFile_.is_open()) {
            logFile_ << "========================================" << std::endl;
            logFile_ << "�f�o�b�O���O�J�n" << std::endl;
            logFile_ << "========================================" << std::endl;
            logFile_.flush();
        }
    }

    ~DebugLog() {
        if (logFile_.is_open()) {
            logFile_ << "========================================" << std::endl;
            logFile_ << "�f�o�b�O���O�I��" << std::endl;
            logFile_ << "========================================" << std::endl;
            logFile_.close();
        }
    }

    void WriteLog(const std::string& level, const std::string& message) {
        if (logFile_.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            
            std::tm bt{};
            localtime_s(&bt, &in_time_t);

            // �t���[���v���t�B�b�N�X
            uint64_t frame = currentFrame_.load(std::memory_order_relaxed);

            logFile_ << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") 
                     << " [F#" << frame << "]"
                     << " [" << level << "] " << message << std::endl;
            
            // �d�v�ȃ��O�͑����Ƀt���b�V��
            if (level == "ERROR" || level == "WARNING") {
                logFile_.flush();
            }
        }
    }

    DebugLog(const DebugLog&) = delete;
    DebugLog& operator=(const DebugLog&) = delete;

    std::ofstream logFile_;
    std::mutex mutex_;
    std::atomic<uint64_t> currentFrame_{0};
};
