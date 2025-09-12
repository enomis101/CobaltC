#pragma once
#include "parser/context_stack_provider.h"

namespace parser {


class SemanticAnalyzerError : public ContextStackError {
public:
    explicit SemanticAnalyzerError(ContextStackProvider* context_provider, const std::string& message)
        : ContextStackError(context_provider, message)
    {
    }
};

}
