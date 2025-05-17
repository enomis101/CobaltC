#pragma once
#include "parser/parser_ast.h"
#include <string>
#include <unordered_map>

namespace parser {

class SymbolTable {
public:
    struct Entry {
        Entry(std::unique_ptr<Type> t, bool d = false)
            : type { std::move(t) }
            , defined { d }
        {
        }
        std::unique_ptr<Type> type;
        bool defined;
        size_t stack_size; // used in assembly step
    };

private:
    SymbolTable() = default;

    std::unordered_map<std::string, Entry> m_table_entries;

public:
    std::unordered_map<std::string, Entry>& symbols() { return m_table_entries; }
    static SymbolTable& instance();
};

}
