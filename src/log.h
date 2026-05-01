#pragma once

namespace ssa
{
    // LogLevel::INFO    - connection events, errors, mode selection only
    // LogLevel::DEBUG   - extra logs not necessary for release version
    // LogLevel::VERBOSE - full per-packet traffic (produces large log files)
    // LogLevel::OFF     - silence everything
    enum class LogLevel { OFF = 0, INFO = 1, DEBUG = 2, VERBOSE = 3 };

    // standard log - always visible at INFO and above
    void Log(const char* format, ...);

    // debug log - only visible when DEBUG
    void LogDebug(const char* format, ...);

    // verbose log - only visible when VERBOSE
    void LogVerbose(const char* format, ...);

} // namespace ssa