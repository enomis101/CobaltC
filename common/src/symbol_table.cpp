#include "common/data/symbol_table.h"

std::optional<InitialValueType> SymbolTable::convert_constant_type(const ConstantType& value, const Type& target_type)
{
    // Handle std::monostate input
    if (std::holds_alternative<std::monostate>(value)) {
        return std::nullopt;
    }

    if (dynamic_cast<const IntType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        } else if (std::holds_alternative<long>(value)) {
            long long_val = std::get<long>(value);
            // Convert long to int using modulo 2^32 (assuming 32-bit int)
            // This truncates the value, keeping only the lower bits
            // TODO: add warning?
            return static_cast<int>(long_val);
        }
    } else if (dynamic_cast<const LongType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return static_cast<long>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return std::get<long>(value);
        }
    }

    return std::nullopt; // Unsupported target type
}
