#include "common/log/spdlog_logger.h"
#include "common/log/log.h"
#include "spdlog/async.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <spdlog/sinks/ostream_sink.h>
#include <stdexcept>
namespace logging {

// Static instance holder in LogManager
std::shared_ptr<ILogger> LogManager::s_instance = nullptr;

// LogManager static methods implementation
void LogManager::initialize(const std::string& config_file)
{
    // if this flag is not set logger are automatically registered
    // to the global registry, we want to manage them by ourselves
    // spdlog::set_automatic_registration(false);
    //  Create the SpdLogger implementation
    if (!s_instance) {
        s_instance = std::make_shared<SpdLogger>();
        s_instance->configure(config_file);
    }
}

std::shared_ptr<ILogger> LogManager::logger()
{
    if (!s_instance) {
        throw std::runtime_error("logger not initialized");
    }
    return s_instance;
}

// Utility functions implementation
LogLevel log_level_from_string(const std::string& level)
{
    if (level == "trace") {
        return LogLevel::TRACE;
    }
    if (level == "debug") {
        return LogLevel::DEBUG;
    }
    if (level == "info") {
        return LogLevel::INFO;
    }
    if (level == "warn") {
        return LogLevel::WARN;
    }
    if (level == "error") {
        return LogLevel::ERROR;
    }
    if (level == "critical") {
        return LogLevel::CRITICAL;
    }
    if (level == "off") {
        return LogLevel::OFF;
    }

    // Default to Info if unknown
    return LogLevel::INFO;
}

std::string log_level_to_string(LogLevel level)
{
    switch (level) {
    case LogLevel::TRACE:
        return "trace";
    case LogLevel::DEBUG:
        return "debug";
    case LogLevel::INFO:
        return "info";
    case LogLevel::WARN:
        return "warn";
    case LogLevel::ERROR:
        return "error";
    case LogLevel::CRITICAL:
        return "critical";
    case LogLevel::OFF:
        return "off";
    default:
        return "unknown";
    }
}

// SpdLogger implementation
SpdLogger::SpdLogger()
    : m_default_level(LogLevel::INFO)
{
    // Initialize thread pool for async logging
    spdlog::init_thread_pool(8192, 1);
    auto tp = spdlog::thread_pool();
    // Create the console sink that will be shared by loggers that need console output
    m_console_sink = std::make_shared<spdlog::sinks::ostream_sink<std::mutex>>(std::cout);

    // Create the default logger with required parameters:
    // - logger name
    // - sink
    // - thread pool
    // - overflow policy
    auto default_logger = std::make_shared<spdlog::async_logger>(
        DEFAULT_CONTEXT,
        m_console_sink,
        tp,
        spdlog::async_overflow_policy::block);

    default_logger->set_level(toSpdLogLevel(m_default_level));
    m_loggers[DEFAULT_CONTEXT] = default_logger;
}

SpdLogger::~SpdLogger()
{
    // First shutdown spdlog (important!)
    spdlog::shutdown();

    // Then clear our local containers
    m_loggers.clear();
    m_file_sinks.clear();
}

void SpdLogger::log(const std::string& context, LogLevel level, const std::string& message)
{
    if (!is_enabled(context, level)) {
        return;
    }

    auto logger = getContextLogger(context);
    logger->log(toSpdLogLevel(level), message);
}

void SpdLogger::trace(const std::string& context, const std::string& message)
{
    log(context, LogLevel::TRACE, message);
}

void SpdLogger::debug(const std::string& context, const std::string& message)
{
    log(context, LogLevel::DEBUG, message);
}

void SpdLogger::info(const std::string& context, const std::string& message)
{
    log(context, LogLevel::INFO, message);
}

void SpdLogger::warn(const std::string& context, const std::string& message)
{
    log(context, LogLevel::WARN, message);
}

void SpdLogger::error(const std::string& context, const std::string& message)
{
    log(context, LogLevel::ERROR, message);
}

void SpdLogger::critical(const std::string& context, const std::string& message)
{
    log(context, LogLevel::CRITICAL, message);
}

bool SpdLogger::is_enabled(const std::string& context, LogLevel level) const
{
    // Check if the context is explicitly configured
    auto config_it = m_context_configs.find(context);
    if (config_it != m_context_configs.end()) {
        // If context is disabled, return false
        if (!config_it->second.enabled) {
            return false;
        }

        // Otherwise check the level
        return static_cast<int>(level) >= static_cast<int>(config_it->second.level);
    }

    // If no specific configuration, use default level
    return static_cast<int>(level) >= static_cast<int>(m_default_level);
}

// Read configuration from JSON file
SpdLogger::LoggerConfig SpdLogger::readConfigFromFile(const std::string& config_file)
{
    // Initialize the default configuration
    LoggerConfig config;

    // Check if file exists and can be opened
    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw LogConfigParseError("Failed to open config file: " + config_file);
    }

    // Parse JSON
    nlohmann::json jsonConfig;
    try {
        file >> jsonConfig;
    } catch (const nlohmann::json::parse_error& e) {
        throw LogConfigParseError("Invalid JSON format in config file: " + std::string(e.what()));
    }

    // Validate required fields
    if (!jsonConfig.contains("default_level")) {
        throw LogConfigParseError("Missing required field 'default_level' in config");
    }

    // Parse default level
    try {
        std::string default_level_str = jsonConfig["default_level"].get<std::string>();
        config.default_level = log_level_from_string(default_level_str);
    } catch (const std::exception& e) {
        throw LogConfigParseError("Invalid 'default_level' value: " + std::string(e.what()));
    }

    // Parse contexts
    if (jsonConfig.contains("contexts")) {
        if (!jsonConfig["contexts"].is_object()) {
            throw LogConfigParseError("'contexts' must be an object");
        }

        for (auto& [context_name, context_config] : jsonConfig["contexts"].items()) {
            bool is_default_context = context_name == DEFAULT_CONTEXT;
            ContextConfig cfg;
            cfg.enabled = context_config.value("enabled", true);
            std::string level_str = context_config.value("level", log_level_to_string(config.default_level));
            cfg.level = log_level_from_string(level_str);

            // Parse console flag - default to true for main, false for others
            cfg.console = context_config.value("console", is_default_context);
            if (!cfg.console && is_default_context) {
                throw LogConfigParseError(" main's console can't be set to false");
            }

            // Parse file path and related settings if present
            if (context_config.contains("file")) {
                if (!context_config["file"].is_string()) {
                    throw LogConfigParseError("'file' must be a string for context: " + context_name);
                }

                cfg.file_path = context_config["file"].get<std::string>();
                cfg.max_size_mb = context_config.value("max_size_mb", 5);
                cfg.max_files = context_config.value("max_files", 3);
            }
            // Add the context configuration
            config.contexts[context_name] = cfg;
        }
    }

    return config;
}

// Configure from file
void SpdLogger::configure(const std::string& config_file)
{
    try {
        // Read configuration from file
        LoggerConfig config = readConfigFromFile(config_file);

        // Apply the configuration
        configure(config);
    } catch (const std::exception& e) {
        // Catch any other unexpected exceptions
        std::string msg = "Configuration error: " + std::string(e.what());
        std::cerr << msg << std::endl;
        throw LogConfigParseError(msg);
    }
}

// Configure from LoggerConfig structure
void SpdLogger::configure(const LoggerConfig& config)
{
    // Set the default level
    m_default_level = config.default_level;

    // Apply context configurations
    for (const auto& [context_name, context_config] : config.contexts) {
        configureContext(context_name, context_config);
    }
}

// Configure a specific context
void SpdLogger::configureContext(const std::string& context_name, const ContextConfig& config)
{
    // Store the configuration
    m_context_configs[context_name] = config;

    // Skip further configuration if context is disabled
    if (!config.enabled) {
        return;
    }

    // Create the logger
    auto tp = spdlog::thread_pool();

    bool is_default_context = context_name == DEFAULT_CONTEXT;

    std::vector<spdlog::sink_ptr> sinks;

    // Set up logging to file if specified
    if (!config.file_path.empty()) {
        size_t max_size = config.max_size_mb * 1024 * 1024; // Convert to bytes
        try {
            addContextLogFile(context_name, config.file_path, max_size, config.max_files);
            sinks.push_back(m_file_sinks[context_name]);
        } catch (const std::exception& e) {
            throw LogConfigParseError("Failed to add log file for context '" + context_name + "': " + std::string(e.what()));
        }
    }

    // Add console sink if configured
    if (config.console && !is_default_context) {
        sinks.push_back(m_console_sink);
    }

    if (sinks.size() == 0 && !is_default_context) {
        throw LogConfigParseError(fmt::format("No output specified for context {}", context_name));
    }

    if (!is_default_context) {
        // Create new logger and initialize it with a list of sinks
        auto logger = std::make_shared<spdlog::async_logger>(context_name, sinks.begin(), sinks.end(), tp,
            spdlog::async_overflow_policy::block);
        logger->set_level(toSpdLogLevel(config.level));
        m_loggers[context_name] = logger;
    } else if (sinks.size() > 0) {
        assert(sinks.size() == 1);
        m_loggers[DEFAULT_CONTEXT]->sinks().push_back(sinks.back());
    }
}

// Fixed version of addContextLogFile method
void SpdLogger::addContextLogFile(const std::string& context, const std::string& filePath,
    size_t maxFileSize, size_t maxFiles)
{
    // Create the file sink
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filePath, maxFileSize, maxFiles);

    // Store the sink
    m_file_sinks[context] = file_sink;
}

spdlog::level::level_enum SpdLogger::toSpdLogLevel(LogLevel level) const
{
    switch (level) {
    case LogLevel::TRACE:
        return spdlog::level::trace;
    case LogLevel::DEBUG:
        return spdlog::level::debug;
    case LogLevel::INFO:
        return spdlog::level::info;
    case LogLevel::WARN:
        return spdlog::level::warn;
    case LogLevel::ERROR:
        return spdlog::level::err;
    case LogLevel::CRITICAL:
        return spdlog::level::critical;
    case LogLevel::OFF:
        return spdlog::level::off;
    default:
        return spdlog::level::info;
    }
}

LogLevel SpdLogger::fromSpdLogLevel(spdlog::level::level_enum level) const
{
    switch (level) {
    case spdlog::level::trace:
        return LogLevel::TRACE;
    case spdlog::level::debug:
        return LogLevel::DEBUG;
    case spdlog::level::info:
        return LogLevel::INFO;
    case spdlog::level::warn:
        return LogLevel::WARN;
    case spdlog::level::err:
        return LogLevel::ERROR;
    case spdlog::level::critical:
        return LogLevel::CRITICAL;
    case spdlog::level::off:
        return LogLevel::OFF;
    default:
        return LogLevel::INFO;
    }
}
// Fixed version of getContextLogger method
std::shared_ptr<spdlog::logger> SpdLogger::getContextLogger(const std::string& context)
{
    // Check if we already have a logger for this context
    auto it = m_loggers.find(context);
    if (it != m_loggers.end()) {
        return it->second;
    }

    auto default_logger = m_loggers[DEFAULT_CONTEXT];

    default_logger->log(spdlog::level::err, fmt::format("Log context not initialized: {}", context));

    return default_logger;
}
void SpdLogger::flushAll()
{
    for (auto pair : m_file_sinks) {
        pair.second->flush();
    }
    m_console_sink->flush();
}

} // namespace logging
