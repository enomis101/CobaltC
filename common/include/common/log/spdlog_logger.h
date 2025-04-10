#pragma once

#include "common/log/log.h"
#include <nlohmann/json.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

namespace logging {

class SpdLogger : public ILogger {
public:
    // Configuration structure for each context
    struct ContextConfig {
        bool enabled = true;
        LogLevel level = LogLevel::INFO;
        bool console = false;
        std::string file_path;
        size_t max_size_mb = 5;
        size_t max_files = 3;
    };

    // Configuration structure for the entire logger
    struct LoggerConfig {
        LogLevel default_level = LogLevel::INFO;
        std::unordered_map<std::string, ContextConfig> contexts;
    };

    // Constructor
    SpdLogger();
    virtual ~SpdLogger();

    // ILogger implementation
    void log(const std::string& context, LogLevel level, const std::string& message) override;
    void trace(const std::string& context, const std::string& message) override;
    void debug(const std::string& context, const std::string& message) override;
    void info(const std::string& context, const std::string& message) override;
    void warn(const std::string& context, const std::string& message) override;
    void error(const std::string& context, const std::string& message) override;
    void critical(const std::string& context, const std::string& message) override;

    bool is_enabled(const std::string& context, LogLevel level) const override;

    // Configure from JSON file
    void configure(const std::string& configFile) override;

    // Configure directly from a LoggerConfig structure
    void configure(const LoggerConfig& config);

    void flushAll();

private:
    // Read configuration from a JSON file
    LoggerConfig readConfigFromFile(const std::string& configFile);

    // Add a context log file
    void addContextLogFile(const std::string& context, const std::string& filePath,
        size_t maxFileSize, size_t maxFiles);

    // Configure a specific context
    void configureContext(const std::string& contextName, const ContextConfig& config);

    // Convert between LogLevel and spdlog level
    spdlog::level::level_enum toSpdLogLevel(LogLevel level) const;
    LogLevel fromSpdLogLevel(spdlog::level::level_enum level) const;

    // Get or create a logger for a context
    std::shared_ptr<spdlog::logger> getContextLogger(const std::string& context);

    // Map of context names to loggers
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;

    // Map of context names to file sinks
    std::unordered_map<std::string, std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>> m_file_sinks;

    // Shared console sink
    std::shared_ptr<spdlog::sinks::ostream_sink<std::mutex>> m_console_sink;

    // Map of context names to their configurations
    std::unordered_map<std::string, ContextConfig> m_context_configs;

    // Default level
    LogLevel m_default_level;
    static constexpr const char* DEFAULT_CONTEXT = "main";
};

} // namespace logging
