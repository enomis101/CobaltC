#pragma once
#include "parser/parser_ast.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace parser {

struct TentativeInit { };
struct InitialValue {
    int value;
};
struct NoInit { };

using Initializer = std::variant<TentativeInit, InitialValue, NoInit>;

struct FunctionAttribute {
    bool defined = false;
    bool global = false;
};

struct StaticAttribute {
    Initializer init;
    bool global = false;
};

struct LocalAttribute { };

using IdentifierAttribute = std::variant<FunctionAttribute, StaticAttribute, LocalAttribute>;

class SymbolTable {
public:
    struct Entry {
        Entry(std::unique_ptr<Type> t, IdentifierAttribute attr)
            : type { std::move(t) }
            , attribute(attr)
        {
        }
        std::unique_ptr<Type> type;
        IdentifierAttribute attribute;
        size_t stack_size; // used in assembly step
    };

private:
    SymbolTable() = default;

    std::unordered_map<std::string, Entry> m_table_entries;
    std::unordered_map<Declaration*, bool> m_top_level_tracker;

public:
    std::unordered_map<std::string, Entry>& symbols() { return m_table_entries; }
    static SymbolTable& instance();
    bool is_top_level(Declaration& decl) { return m_top_level_tracker.contains(&decl) && m_top_level_tracker[&decl]; }
    void set_top_level(Declaration& decl) { m_top_level_tracker[&decl] = true; }
};

}
