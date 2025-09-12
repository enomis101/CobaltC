#include "parser/context_stack_provider.h"
#include <format>

namespace parser{


std::string ContextStackProvider::context_stack_to_string() const
{
    std::string context_string = std::format("\n==================\nContext Stack:\n");
    for (size_t i = 0; i < m_context_stack.size(); ++i) {
        const auto& str = m_context_stack.at(i);
        context_string += std::format("{}\n", str);
    }
    return context_string;
}

}