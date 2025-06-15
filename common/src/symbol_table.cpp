#include "common/data/symbol_table.h"
#include <format>
#include <limits>

std::optional<StaticInitialValueType> SymbolTable::convert_constant_type(const ConstantType& value, const Type& target_type, std::function<void(const std::string&)> warning_callback)
{
    // Handle std::monostate input
    if (std::holds_alternative<std::monostate>(value)) {
        return std::nullopt;
    }

    // Helper lambda to check if a value will change when converted
    auto check_value_change = [&warning_callback](auto from_val, auto to_type_name, bool from_signed, bool to_signed) {
        using FromType = decltype(from_val);

        // Check sign change warnings
        if (from_signed && !to_signed && from_val < 0) {
            if (warning_callback) {
                warning_callback(std::format("converting negative value {} to unsigned {}", from_val, to_type_name));
            }
        } else if (!from_signed && to_signed) {
            // Check if unsigned value is too large for signed type
            if constexpr (std::is_same_v<FromType, unsigned int>) {
                if (from_val > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
                    if (warning_callback) {
                        warning_callback(std::format("large unsigned value {} may become negative when converted to signed {}", from_val, to_type_name));
                    }
                }
            } else if constexpr (std::is_same_v<FromType, unsigned long>) {
                if (from_val > static_cast<unsigned long>(std::numeric_limits<long>::max())) {
                    if (warning_callback) {
                        warning_callback(std::format("large unsigned value {} may become negative when converted to signed {}", from_val, to_type_name));
                    }
                }
            }
        }
    };

    // Handle IntType target
    if (dynamic_cast<const IntType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        } else if (std::holds_alternative<long>(value)) {
            long long_val = std::get<long>(value);
            if (warning_callback) {
                warning_callback(std::format("truncating from long to int"));
            }
            check_value_change(long_val, "int", true, true);
            return static_cast<int>(long_val);
        } else if (std::holds_alternative<unsigned int>(value)) {
            unsigned int uint_val = std::get<unsigned int>(value);
            check_value_change(uint_val, "int", false, true);
            return static_cast<int>(uint_val);
        } else if (std::holds_alternative<unsigned long>(value)) {
            unsigned long ulong_val = std::get<unsigned long>(value);
            if (warning_callback) {
                warning_callback(std::format("truncating from unsigned long to int"));
            }
            check_value_change(ulong_val, "int", false, true);
            return static_cast<int>(ulong_val);
        }
    }
    // Handle LongType target
    else if (dynamic_cast<const LongType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return static_cast<long>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return std::get<long>(value);
        } else if (std::holds_alternative<unsigned int>(value)) {
            unsigned int uint_val = std::get<unsigned int>(value);
            // unsigned int always fits in long
            return static_cast<long>(uint_val);
        } else if (std::holds_alternative<unsigned long>(value)) {
            unsigned long ulong_val = std::get<unsigned long>(value);
            check_value_change(ulong_val, "long", false, true);
            return static_cast<long>(ulong_val);
        }
    }
    // Handle UnsignedIntType target
    else if (dynamic_cast<const UnsignedIntType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            int int_val = std::get<int>(value);
            check_value_change(int_val, "unsigned int", true, false);
            return static_cast<unsigned int>(int_val);
        } else if (std::holds_alternative<long>(value)) {
            long long_val = std::get<long>(value);
            if (warning_callback) {
                warning_callback(std::format("truncating from long to unsigned int"));
            }
            check_value_change(long_val, "unsigned int", true, false);
            return static_cast<unsigned int>(long_val);
        } else if (std::holds_alternative<unsigned int>(value)) {
            return std::get<unsigned int>(value);
        } else if (std::holds_alternative<unsigned long>(value)) {
            unsigned long ulong_val = std::get<unsigned long>(value);
            if (warning_callback) {
                warning_callback(std::format("truncating from unsigned long to unsigned int"));
            }
            // Check if value is too large
            if (ulong_val > std::numeric_limits<unsigned int>::max()) {
                if (warning_callback) {
                    warning_callback(std::format("value {} exceeds unsigned int range", ulong_val));
                }
            }
            return static_cast<unsigned int>(ulong_val);
        }
    }
    // Handle UnsignedLongType target
    else if (dynamic_cast<const UnsignedLongType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            int int_val = std::get<int>(value);
            check_value_change(int_val, "unsigned long", true, false);
            return static_cast<unsigned long>(int_val);
        } else if (std::holds_alternative<long>(value)) {
            long long_val = std::get<long>(value);
            check_value_change(long_val, "unsigned long", true, false);
            return static_cast<unsigned long>(long_val);
        } else if (std::holds_alternative<unsigned int>(value)) {
            return static_cast<unsigned long>(std::get<unsigned int>(value));
        } else if (std::holds_alternative<unsigned long>(value)) {
            return std::get<unsigned long>(value);
        }
    }

    return std::nullopt; // Unsupported target type
}
