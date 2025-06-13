#pragma once
#include "backend/assembly_ast.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

namespace backend {

struct ObjectEntry {
    AssemblyType type;
    bool is_static;
};

struct FunctionEntry {
    size_t stack_frame_size;
    bool defined;
};

using BackendSymbolTableEntry = std::variant<ObjectEntry, FunctionEntry>;

class BackendSymbolTable {
public:
    BackendSymbolTable() = default;

    // Return const reference to allow iteration
    const std::unordered_map<std::string, BackendSymbolTableEntry>& symbols() const
    {
        return m_symbols;
    }

    // Returns reference to symbol entry, throws if not found
    BackendSymbolTableEntry& symbol_at(const std::string& name)
    {
        return m_symbols.at(name);
    }

    const BackendSymbolTableEntry& symbol_at(const std::string& name) const
    {
        return m_symbols.at(name);
    }

    // Insert only if the symbol does not exist, throws if it already exists
    void insert_symbol(const std::string& name, const BackendSymbolTableEntry& entry)
    {
        auto [it, inserted] = m_symbols.emplace(name, entry);
        if (!inserted) {
            throw std::runtime_error("Symbol '" + name + "' already exists in symbol table");
        }
    }

    void insert_or_assign_symbol(const std::string& name, const BackendSymbolTableEntry& entry)
    {
        m_symbols.insert_or_assign(name, entry);
    }

    // Check if symbol exists
    bool contains_symbol(const std::string& name) const
    {
        return m_symbols.contains(name);
    }

    static std::optional<StaticInitialValueType> convert_constant_type(const ConstantType& value, const Type& target_type);

private:
    std::unordered_map<std::string, BackendSymbolTableEntry> m_symbols;
};

}
