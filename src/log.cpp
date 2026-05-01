#include <iostream>
#include <fstream>
#include <cstdarg>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>

namespace ssa
{
namespace fs = std::filesystem;

class Logger {
private:
    static std::string s_logFilePath;
    static bool s_initialized;
    static constexpr int MAX_LOG_FILES = 3;

    static std::string GetLogDirectory() {
        char modulePath[MAX_PATH];
        GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

        fs::path exePath(modulePath);
        fs::path logDir = exePath.parent_path() / "logs";

        fs::create_directories(logDir);
        return logDir.string();
    }

    static void CleanupOldLogs() {
        std::string logDir = GetLogDirectory();
        std::vector<fs::path> logFiles;

        for (const auto& entry : fs::directory_iterator(logDir)) {
            if (entry.path().extension() == ".txt") {
                logFiles.push_back(entry.path());
            }
        }

        if (logFiles.size() >= MAX_LOG_FILES) {
            std::sort(logFiles.begin(), logFiles.end(), [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) < fs::last_write_time(b);
            });

            for (size_t i = 0; i < logFiles.size() - MAX_LOG_FILES + 1; ++i) {
                fs::remove(logFiles[i]);
            }
        }
    }

    static std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    static void Initialize() {
        if (s_initialized) return;

        CleanupOldLogs();

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        tm localTime;
        localtime_s(&localTime, &time);

        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &localTime);

        s_logFilePath = GetLogDirectory() + "\\ssa_log_" + timestamp + ".txt";
        s_initialized = true;

        std::ofstream log(s_logFilePath);
        log << "=== Skylanders Spyro's Adventure LibUSB Log ===" << std::endl;
        log << "Session started: " << timestamp << std::endl;
        log << "=========================================" << std::endl << std::endl;
    }

public:
    static void Write(const char* format, ...) {
        if (!s_initialized) {
            Initialize();
        }

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::ofstream log(s_logFilePath, std::ios::app);
        log << "[" << GetTimestamp() << "] " << buffer << std::endl;
    }
};

std::string Logger::s_logFilePath;
bool Logger::s_initialized = false;

void Log(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Logger::Write("%s", buffer);
}
} // namespace ssa
