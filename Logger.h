#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <chrono>
#include <iomanip>

class Logger {
public:
    static void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isInitialized) return;

        // %temp% dizinini al
        auto tempPath = std::filesystem::temp_directory_path();
        
        // Klasör adı: !Yerelkesif_Logs
        std::filesystem::path logDir = tempPath / "!Yerelkesif_Logs";
        
        // Klasör yoksa oluştur
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);

        // Dosya adı klasör adıyla aynı: !Yerelkesif_Logs.txt
        std::filesystem::path logFilePath = logDir / "!Yerelkesif_Logs.txt";

        m_logFile.open(logFilePath, std::ios::out | std::ios::trunc);
        if (m_logFile.is_open()) {
            m_isInitialized = true;
            m_logFile << "=== Log Başlatıldı ===" << std::endl;
            m_logFile.flush();
        }
    }

    static void writeLog(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInitialized) {
            init();
        }

        if (m_logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::tm tmBuffer{};
#if defined(_WIN32)
            localtime_s(&tmBuffer, &in_time_t);
#else
            localtime_r(&in_time_t, &tmBuffer);
#endif

            m_logFile << "[" 
                      << std::put_time(&tmBuffer, "%H:%M:%S") 
                      << "." << std::setfill('0') << std::setw(3) << ms.count() 
                      << "] " << message << std::endl;
            m_logFile.flush();
        }
    }

    class LogStream {
    public:
        LogStream() = default;
        ~LogStream() {
            Logger::writeLog(m_ss.str());
        }

        template <typename T>
        LogStream& operator<<(const T& val) {
            m_ss << val;
            return *this;
        }

    private:
        std::ostringstream m_ss;
    };

private:
    inline static std::ofstream m_logFile;
    inline static std::mutex m_mutex;
    inline static bool m_isInitialized{false};
};

#define LOG() Logger::LogStream()