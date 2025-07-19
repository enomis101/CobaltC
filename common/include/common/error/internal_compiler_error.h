#pragma once
#include <stdexcept>

/**
 * Indicates a bug in the compiler implementation itself, not in user code. This replaces
 * assert(false) calls to provide better error recovery and debugging info.
 *
 * If you see this exception, it's most likely a compiler bug that needs to be fixed.
 */
class InternalCompilerError : public std::runtime_error {
public:
    explicit InternalCompilerError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class UnsupportedFeatureError : public std::runtime_error {
public:
    explicit UnsupportedFeatureError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};
