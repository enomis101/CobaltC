#pragma once
#include <stdexcept>

namespace parser {

class SemanticAnalyzerError : public std::runtime_error {
public:
    explicit SemanticAnalyzerError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

}