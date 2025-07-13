#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include <climits>
#include <cmath>
#include <format>

StaticInitialValueType::StaticInitialValueType(ConstantType constant_value)
{
    if (std::holds_alternative<std::monostate>(constant_value)) {
        m_value = ZeroInit { 0 };
    } else if (std::holds_alternative<int>(constant_value)) {
        int val = std::get<int>(constant_value);
        if (val == 0) {
            m_value = ZeroInit { TypeSizes::INT_SIZE };
        } else {
            m_value = constant_value;
        }
    } else if (std::holds_alternative<long>(constant_value)) {
        long val = std::get<long>(constant_value);
        if (val == 0L) {
            m_value = ZeroInit { TypeSizes::LONG_SIZE };
        } else {
            m_value = constant_value;
        }
    } else if (std::holds_alternative<unsigned int>(constant_value)) {
        unsigned int val = std::get<unsigned int>(constant_value);
        if (val == 0U) {
            m_value = ZeroInit { TypeSizes::UNSIGNED_INT_SIZE };
        } else {
            m_value = constant_value;
        }
    } else if (std::holds_alternative<unsigned long>(constant_value)) {
        unsigned long val = std::get<unsigned long>(constant_value);
        if (val == 0UL) {
            m_value = ZeroInit { TypeSizes::UNSIGNED_LONG_SIZE };
        } else {
            m_value = constant_value;
        }
    } else if (std::holds_alternative<double>(constant_value)) {
        double val = std::get<double>(constant_value);
        if (val == 0.0 && !std::signbit(val)) {
            m_value = ZeroInit { TypeSizes::DOUBLE_SIZE };
        } else {
            m_value = constant_value;
        }
    }
}

std::expected<ConstantType, std::string> SymbolTable::convert_constant_type(const ConstantType& value, const Type& target_type, std::function<void(const std::string&)> warning_callback)
{
    // Handle std::monostate input
    if (std::holds_alternative<std::monostate>(value)) {
        return std::unexpected("constant holds invalid value");
    }

    if (is_type<PointerType>(target_type)) {
        if (is_null_pointer_constant(value)) {
            return 0ul; // use unsigned long 0 as pointer are 64-bit unsigned integers
        }
        return std::unexpected("Cannot convert non-zero constant to pointer type");
    }

    // Helper to get type name from value
    auto get_source_type_name = [](const ConstantType& val) -> std::string {
        if (std::holds_alternative<int>(val))
            return "int";
        if (std::holds_alternative<long>(val))
            return "long";
        if (std::holds_alternative<unsigned int>(val))
            return "unsigned int";
        if (std::holds_alternative<unsigned long>(val))
            return "unsigned long";
        if (std::holds_alternative<double>(val))
            return "double";
        return "unknown";
    };

    // Helper to get target type name
    auto get_target_type_name = [](const Type& target) -> std::string {
        if (is_type<IntType>(target))
            return "int";
        if (is_type<LongType>(target))
            return "long";
        if (is_type<UnsignedIntType>(target))
            return "unsigned int";
        if (is_type<UnsignedLongType>(target))
            return "unsigned long";
        if (is_type<DoubleType>(target))
            return "double";
        return "unknown";
    };

    // Helper to check if conversion is needed and warn
    auto warn_if_conversion_needed = [&](const std::string& source_type, const std::string& target_type_name) {
        if (source_type != target_type_name && warning_callback) {
            warning_callback(std::format("converting from {} to {}", source_type, target_type_name));
        }
    };

    std::string source_type = get_source_type_name(value);
    std::string target_type_name = get_target_type_name(target_type);

    warn_if_conversion_needed(source_type, target_type_name);

    // Handle IntType target
    if (is_type<IntType>(target_type)) {

        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        } else if (std::holds_alternative<long>(value)) {
            return static_cast<int>(std::get<long>(value));
        } else if (std::holds_alternative<unsigned int>(value)) {
            return static_cast<int>(std::get<unsigned int>(value));
        } else if (std::holds_alternative<unsigned long>(value)) {
            return static_cast<int>(std::get<unsigned long>(value));
        } else if (std::holds_alternative<double>(value)) {
            auto val = std::get<double>(value);
            if (val > INT_MAX || val < INT_MIN || std::isnan(val) || std::isinf(val)) {
                return std::unexpected("Conversion from double constant to int overflow");
            }
            return static_cast<int>(std::get<double>(value));
        }
    }
    // Handle LongType target
    else if (is_type<LongType>(target_type)) {

        if (std::holds_alternative<int>(value)) {
            return static_cast<long>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return std::get<long>(value);
        } else if (std::holds_alternative<unsigned int>(value)) {
            return static_cast<long>(std::get<unsigned int>(value));
        } else if (std::holds_alternative<unsigned long>(value)) {
            return static_cast<long>(std::get<unsigned long>(value));
        } else if (std::holds_alternative<double>(value)) {
            auto val = std::get<double>(value);
            if (val > LONG_MAX || val < LONG_MIN || std::isnan(val) || std::isinf(val)) {
                return std::unexpected("Conversion from double constant to long overflow");
            }
            return static_cast<long>(std::get<double>(value));
        }
    }
    // Handle UnsignedIntType target
    else if (is_type<UnsignedIntType>(target_type)) {

        if (std::holds_alternative<int>(value)) {
            return static_cast<unsigned int>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return static_cast<unsigned int>(std::get<long>(value));
        } else if (std::holds_alternative<unsigned int>(value)) {
            return std::get<unsigned int>(value);
        } else if (std::holds_alternative<unsigned long>(value)) {
            return static_cast<unsigned int>(std::get<unsigned long>(value));
        } else if (std::holds_alternative<double>(value)) {
            auto val = std::get<double>(value);
            if (val > UINT_MAX || val < 0.0 || std::isnan(val) || std::isinf(val)) {
                return std::unexpected("Conversion from double constant to unsigned overflow");
            }
            return static_cast<unsigned int>(std::get<double>(value));
        }
    }
    // Handle UnsignedLongType target
    else if (is_type<UnsignedLongType>(target_type)) {

        if (std::holds_alternative<int>(value)) {
            return static_cast<unsigned long>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return static_cast<unsigned long>(std::get<long>(value));
        } else if (std::holds_alternative<unsigned int>(value)) {
            return static_cast<unsigned long>(std::get<unsigned int>(value));
        } else if (std::holds_alternative<unsigned long>(value)) {
            return std::get<unsigned long>(value);
        } else if (std::holds_alternative<double>(value)) {
            auto val = std::get<double>(value);
            if (val > ULONG_MAX || val < 0.0 || std::isnan(val) || std::isinf(val)) {
                return std::unexpected("Conversion from double constant to unsigned long overflow");
            }
            return static_cast<unsigned long>(std::get<double>(value));
        }
    } else if (is_type<DoubleType>(target_type)) {
        if (std::holds_alternative<int>(value)) {
            return static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return static_cast<double>(std::get<long>(value));
        } else if (std::holds_alternative<unsigned int>(value)) {
            return static_cast<double>(std::get<unsigned int>(value));
        } else if (std::holds_alternative<unsigned long>(value)) {
            return static_cast<double>(std::get<unsigned long>(value));
        } else if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }
    }

    return std::unexpected("Unsupported target type");
}

bool SymbolTable::is_null_pointer_constant(const ConstantType& constant)
{
    if (std::holds_alternative<int>(constant)) {
        return std::get<int>(constant) == 0;
    } else if (std::holds_alternative<unsigned int>(constant)) {
        return std::get<unsigned int>(constant) == 0;
    } else if (std::holds_alternative<long>(constant)) {
        return std::get<long>(constant) == 0;
    } else if (std::holds_alternative<unsigned long>(constant)) {
        return std::get<unsigned long>(constant) == 0;
    }
    return false;
}
