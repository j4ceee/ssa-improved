#include "log.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>

#include "config.h"

namespace ssa
{
    namespace fs = std::filesystem;

    class Logger
    {
    private:
        static std::string s_logFilePath;
        static bool s_initialized;
        static constexpr int MAX_LOG_FILES = 3;

        static std::string GetLogDirectory()
        {
            char modulePath[MAX_PATH];
            GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

            const fs::path exePath(modulePath);
            const fs::path logDir = exePath.parent_path() / "ssa-improved" / "logs";

            fs::create_directories(logDir);
            return logDir.string();
        }

        static void CleanupOldLogs()
        {
            const std::string logDir = GetLogDirectory();
            std::vector<fs::path> logFiles;

            for (const auto& entry : fs::directory_iterator(logDir))
            {
                if (entry.path().extension() == ".txt")
                {
                    logFiles.push_back(entry.path());
                }
            }

            if (logFiles.size() >= MAX_LOG_FILES)
            {
                std::sort(logFiles.begin(), logFiles.end(), [](const fs::path& a, const fs::path& b)
                {
                    return fs::last_write_time(a) < fs::last_write_time(b);
                });

                for (size_t i = 0; i < logFiles.size() - MAX_LOG_FILES + 1; ++i)
                {
                    fs::remove(logFiles[i]);
                }
            }
        }

        static std::string GetTimestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const auto time = std::chrono::system_clock::to_time_t(now);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ) % 1000;

            tm localTime;
            localtime_s(&localTime, &time);

            std::ostringstream oss;
            oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        static bool Initialize()
        {
            if (s_initialized) return true;

            try
            {
                CleanupOldLogs();
            }
            catch (...)
            {
            } // non-fatal

            const auto now = std::chrono::system_clock::now();
            const auto time = std::chrono::system_clock::to_time_t(now);

            tm localTime;
            localtime_s(&localTime, &time);

            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &localTime);

            try
            {
                s_logFilePath = GetLogDirectory() + "\\ssa_log_" + timestamp + ".txt";
                std::ofstream log(s_logFilePath);
                if (!log.is_open()) return false; // no write access
                log << "=== Skylanders Spyro's Adventure Improved Log ===" << std::endl;
                log << " Session started: " << timestamp << std::endl;
                log << "=================================================" << std::endl << std::endl;
            }
            catch (...)
            {
                return false;
            }

            s_initialized = true;
            return true;
        }

    public:
        static void Write(LogLevel level, const char* format, va_list args)
        {
            if (level > g_config.logLevel) return;
            if (!Initialize()) return;

            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, args);

            std::ofstream log(s_logFilePath, std::ios::app);
            log << "[" << GetTimestamp() << "] " << buffer << std::endl;
        }
    };

    std::string Logger::s_logFilePath;
    bool Logger::s_initialized = false;

    void Log(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger::Write(LogLevel::INFO, format, args);
        va_end(args);
    }

    void LogF(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger::Write(LogLevel::OFF, format, args);
        va_end(args);
    }

    void LogDebug(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger::Write(LogLevel::DEBUG, format, args);
        va_end(args);
    }

    void LogVerbose(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger::Write(LogLevel::VERBOSE, format, args);
        va_end(args);
    }
} // namespace ssa
