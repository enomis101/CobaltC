#pragma once
#include <string>

enum class LexerWarningType {
    GENERIC,
    CAST,
};

enum class ParserWarningType {
    GENERIC,
    CAST,
};

// This class is used to raise warnings from multiple in the compiler
// It helps in testings to detect when and where a warning is raised, for this reason the methods are virtual
// At the moment it doesn't need to be thread safe

class WarningManager {

public:
    virtual void raise_warning(LexerWarningType warning_type, const std::string& message);
    virtual void raise_warning(ParserWarningType warning_type, const std::string& message);

private:
    static constexpr const char* LEXER_LOG_CONTEXT = "lexer";
    static constexpr const char* PARSER_LOG_CONTEXT = "parser";
};
