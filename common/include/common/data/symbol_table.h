#pragma once
#include "common/data/type.h"
#include <expected>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

using StaticInitialValueType = ConstantType; // for now they are the same

struct TentativeInit { };

struct StaticInitialValue {
    StaticInitialValueType value;
};

struct NoInit { };

using StaticInitializer = std::variant<TentativeInit, StaticInitialValue, NoInit>;

struct FunctionAttribute {
    bool defined = false;
    bool global = false;
};

struct StaticAttribute {
    StaticInitializer init;
    bool global = false;
};

struct LocalAttribute { };

using IdentifierAttribute = std::variant<FunctionAttribute, StaticAttribute, LocalAttribute>;

class SymbolTable {
public:
    struct SymbolEntry {
        SymbolEntry(std::unique_ptr<Type> type, IdentifierAttribute attribute)
            : type { std::move(type) }
            , attribute(attribute)
        {
        }
        std::unique_ptr<Type> type;
        IdentifierAttribute attribute;
    };

    SymbolTable() = default;

    // Return const reference to allow iteration
    const std::unordered_map<std::string, SymbolEntry>& symbols() const
    {
        return m_symbols;
    }

    // Returns reference to symbol entry, throws if not found
    SymbolEntry& symbol_at(const std::string& name)
    {
        return m_symbols.at(name);
    }

    const SymbolEntry& symbol_at(const std::string& name) const
    {
        return m_symbols.at(name);
    }

    // Insert only if the symbol does not exist, throws if it already exists
    void insert_symbol(const std::string& name, std::unique_ptr<Type> type, IdentifierAttribute attr)
    {
        auto [it, inserted] = m_symbols.emplace(name, SymbolEntry(std::move(type), attr));
        if (!inserted) {
            throw std::runtime_error("Symbol '" + name + "' already exists in symbol table");
        }
    }

    void insert_or_assign_symbol(const std::string& name, std::unique_ptr<Type> type, IdentifierAttribute attr)
    {
        m_symbols.insert_or_assign(name, SymbolEntry(std::move(type), attr));
    }

    // Check if symbol exists
    bool contains_symbol(const std::string& name) const
    {
        return m_symbols.contains(name);
    }

    static std::expected<StaticInitialValueType, std::string> convert_constant_type(const ConstantType& value, const Type& target_type, std::function<void(const std::string&)> warning_callback = nullptr);

    static bool is_null_pointer_constant(const ConstantType& constant);

private:
    std::unordered_map<std::string, SymbolEntry> m_symbols;
};
