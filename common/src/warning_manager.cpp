#include "common/data/warning_manager.h"
#include "common/log/log.h"

void WarningManager::raise_warning(LexerWarningType warning_type, const std::string& message)
{
    LOG_WARN(LEXER_LOG_CONTEXT, message);
}

void WarningManager::raise_warning(ParserWarningType warning_type, const std::string& message)
{
    LOG_WARN(PARSER_LOG_CONTEXT, message);
}
