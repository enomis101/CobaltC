#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace logging {

// Custom exception for configuration errors
class LogConfigParseError : public std::runtime_error {
public:
    explicit LogConfigParseError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

// Enum for log levels
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

// Convert string to LogLevel
LogLevel log_level_from_string(const std::string& level);

// Convert LogLevel to string
std::string log_level_to_string(LogLevel level);

// Pure abstract Logger interface
class ILogger {
public:
    virtual ~ILogger() = default;

    // Core logging methods
    virtual void log(const std::string& context, LogLevel level, const std::string& message) = 0;

    // Convenience methods for different log levels
    virtual void trace(const std::string& context, const std::string& message) = 0;
    virtual void debug(const std::string& context, const std::string& message) = 0;
    virtual void info(const std::string& context, const std::string& message) = 0;
    virtual void warn(const std::string& context, const std::string& message) = 0;
    virtual void error(const std::string& context, const std::string& message) = 0;
    virtual void critical(const std::string& context, const std::string& message) = 0;

    // Check if logging is enabled for a specific context and level
    virtual bool is_enabled(const std::string& context, LogLevel level) const = 0;

    // Configure from file
    virtual void configure(const std::string& configFile) = 0;
};

class LogManager {
public:
    // Get the singleton instance
    static std::shared_ptr<ILogger> logger()
    {
        static LogManager instance; // Meyer's singleton - created once on first use
        return instance.m_logger;
    }

    static void init()
    {
        logger();
    }

private:
    // Private constructor - this is a static utility class
    LogManager();

    // Static instance holder
    std::shared_ptr<ILogger> m_logger;
};

// Helper macros for convenient logging
#define LOG_TRACE(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::TRACE)) { \
        logging::LogManager::logger()->trace(context, message);                         \
    }

#define LOG_DEBUG(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::DEBUG)) { \
        logging::LogManager::logger()->debug(context, message);                         \
    }

#define LOG_INFO(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::INFO)) { \
        logging::LogManager::logger()->info(context, message);                         \
    }

#define LOG_WARN(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::WARN)) { \
        logging::LogManager::logger()->warn(context, message);                         \
    }

#define LOG_ERROR(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::ERROR)) { \
        logging::LogManager::logger()->error(context, message);                         \
    }

#define LOG_CRITICAL(context, message)                                                     \
    if (logging::LogManager::logger()->is_enabled(context, logging::LogLevel::CRITICAL)) { \
        logging::LogManager::logger()->critical(context, message);                         \
    }

// Generic log with level
#define LOG(context, level, message)                                 \
    if (logging::LogManager::logger()->is_enabled(context, level)) { \
        logging::LogManager::logger()->log(context, level, message); \
    }

} // namespace logging
