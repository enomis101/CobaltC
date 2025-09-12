#pragma once
#include "common/data/source_location.h"
#include <optional>
#include <stdexcept>
#include <vector>

#define ENTER_CONTEXT(name) ContextGuard context_guard(m_context_stack, name, std::nullopt)
#define ENTER_CONTEXT_WITH_SOURCE(name) ContextGuard context_guard(m_context_stack, name, (has_tokens() ? std::optional<SourceLocation>(peek().source_location()) : std::nullopt))

namespace parser {
class ContextStackProvider
{
public:
    virtual std::string context_stack_to_string() const;
    virtual ~ContextStackProvider() = default;
protected:
    using ContextStack = std::vector<std::string>;

    ContextStack m_context_stack;

    class ContextGuard {
    public:
        ContextGuard(ContextStack& context_stack, const std::string& context, std::optional<SourceLocation> source_location);

        ~ContextGuard();

    private:
        ContextStack& m_context_stack;
    };
};

class ContextStackError : public std::runtime_error {
public:
    explicit ContextStackError(ContextStackProvider* context_provider, const std::string& message)
        : std::runtime_error(message + context_provider->context_stack_to_string())
    {
    }

};

}//namespace parser