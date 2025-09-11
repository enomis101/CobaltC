#pragma once
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

using ConstantType = std::variant<std::monostate, int, long, unsigned int, unsigned long, double, char, unsigned char>;

namespace TypeSizes {
inline constexpr size_t CHAR_SIZE = 1;
inline constexpr size_t INT_SIZE = 4;
inline constexpr size_t LONG_SIZE = 8;
inline constexpr size_t UNSIGNED_INT_SIZE = 4;
inline constexpr size_t UNSIGNED_LONG_SIZE = 8;
inline constexpr size_t DOUBLE_SIZE = 8;
}

class Type {
public:
    virtual ~Type() = default;

    // Virtual clone function - the key addition
    // Returns a unique_ptr to a deep copy of the object
    virtual std::unique_ptr<Type> clone() const = 0;

    // Virtual to_string function for polymorphic printing
    virtual std::string to_string() const = 0;

    virtual size_t alignment() const { return 0; }
    virtual size_t size() const { return 0; }
    virtual bool is_signed() const { return false; }
    virtual bool is_arithmetic() const { return false; }
    virtual bool is_integer() const { return false; }
    virtual bool is_scalar() const { return false; }
    virtual bool is_char() const { return false; }

    // Default equality comparison
    virtual bool equals(const Type& other) const
    {
        return typeid(*this) == typeid(other);
    }

    // Operators deleted to force explicit use of equals()
    bool operator==(const Type& other) const = delete;
    bool operator!=(const Type& other) const = delete;
};

class IntType : public Type {
public:
    // Clone implementation for IntType
    // Creates a new IntType object and returns it wrapped in unique_ptr
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<IntType>();
    }

    std::string to_string() const override
    {
        return "int";
    }

    bool is_signed() const override { return true; }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 4; }
    size_t size() const override { return TypeSizes::INT_SIZE; }
    bool is_integer() const override { return true; }
    bool is_scalar() const override { return true; }
};

class LongType : public Type {
public:
    // Clone implementation for LongType
    // Creates a new LongType object and returns it wrapped in unique_ptr
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<LongType>();
    }

    std::string to_string() const override
    {
        return "long";
    }

    bool is_signed() const override { return true; }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 8; }
    size_t size() const override { return TypeSizes::LONG_SIZE; }
    bool is_integer() const override { return true; }
    bool is_scalar() const override { return true; }
};

class UnsignedIntType : public Type {
public:
    // Clone implementation for UnsignedIntType
    // Creates a new UnsignedIntType object and returns it wrapped in unique_ptr
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<UnsignedIntType>();
    }

    std::string to_string() const override
    {
        return "unsigned int";
    }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 4; }
    size_t size() const override { return TypeSizes::UNSIGNED_INT_SIZE; }
    bool is_integer() const override { return true; }
    bool is_scalar() const override { return true; }
};

class UnsignedLongType : public Type {
public:
    // Clone implementation for UnsignedLongType
    // Creates a new UnsignedLongType object and returns it wrapped in unique_ptr
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<UnsignedLongType>();
    }

    std::string to_string() const override
    {
        return "unsigned long";
    }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 8; }
    size_t size() const override { return TypeSizes::UNSIGNED_LONG_SIZE; }
    bool is_integer() const override { return true; }
    bool is_scalar() const override { return true; }
};

class CharType : public Type {
public:
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<CharType>();
    }

    std::string to_string() const override
    {
        return "char";
    }

    bool is_signed() const override { return true; }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 1; }
    size_t size() const override { return TypeSizes::CHAR_SIZE; }
    bool is_scalar() const override { return true; }
    bool is_char() const override { return true; }
};

class UnsignedCharType : public CharType {
public:
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<UnsignedCharType>();
    }

    std::string to_string() const override
    {
        return "unsigned char";
    }
};

class SignedCharType : public CharType {
public:
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<SignedCharType>();
    }

    std::string to_string() const override
    {
        return "signed char";
    }
};

class DoubleType : public Type {
public:
    // Clone implementation for DoubleType
    // Creates a new DoubleType object and returns it wrapped in unique_ptr
    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<DoubleType>();
    }

    std::string to_string() const override
    {
        return "double";
    }
    bool is_arithmetic() const override { return true; }
    size_t alignment() const override { return 8; }
    size_t size() const override { return TypeSizes::DOUBLE_SIZE; }
    bool is_scalar() const override { return true; }
};

class FunctionType : public Type {
public:
    FunctionType(std::unique_ptr<Type> return_type, std::vector<std::unique_ptr<Type>> parameters_type)
        : return_type { std::move(return_type) }
        , parameters_type { std::move(parameters_type) }
    {
    }

    // Clone implementation for FunctionType
    // This is more complex because we need to deep-copy the contained types
    std::unique_ptr<Type> clone() const override
    {
        // Clone the return type
        auto cloned_return_type = return_type->clone();

        // Clone each parameter type
        std::vector<std::unique_ptr<Type>> cloned_parameters;
        cloned_parameters.reserve(parameters_type.size());

        for (const auto& param : parameters_type) {
            cloned_parameters.push_back(param->clone());
        }

        // Create and return new FunctionType with cloned components
        return std::make_unique<FunctionType>(
            std::move(cloned_return_type),
            std::move(cloned_parameters));
    }

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << return_type->to_string() << "(";

        for (size_t i = 0; i < parameters_type.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << parameters_type[i]->to_string();
        }

        ss << ")";
        return ss.str();
    }

    bool equals(const Type& other) const override
    {
        // First check if other is also a FunctionType
        const auto* other_func = dynamic_cast<const FunctionType*>(&other);
        if (!other_func) {
            return false;
        }

        // Compare return types
        if (!return_type->equals(*other_func->return_type)) {
            return false;
        }

        // Compare parameter counts
        if (parameters_type.size() != other_func->parameters_type.size()) {
            return false;
        }

        // Compare each parameter type
        for (size_t i = 0; i < parameters_type.size(); ++i) {
            if (!parameters_type[i]->equals(*other_func->parameters_type[i])) {
                return false;
            }
        }

        return true;
    }

    std::unique_ptr<Type> return_type;
    std::vector<std::unique_ptr<Type>> parameters_type;
};

class PointerType : public Type {
public:
    PointerType(std::unique_ptr<Type> referenced_type)
        : referenced_type { std::move(referenced_type) }
    {
    }

    std::unique_ptr<Type> clone() const override
    {
        // Clone the return type
        auto cloned_referenced_type = referenced_type->clone();

        // Create and return new PointerType with cloned components
        return std::make_unique<PointerType>(
            std::move(cloned_referenced_type));
    }

    std::string to_string() const override
    {
        return std::format("{}*", referenced_type->to_string());
    }

    bool equals(const Type& other) const override
    {
        // First check if other is also a PointerType
        const auto* other_pointer = dynamic_cast<const PointerType*>(&other);
        if (!other_pointer) {
            return false;
        }

        return referenced_type->equals(*other_pointer->referenced_type);
    }

    std::unique_ptr<Type> referenced_type;

    size_t size() const override { return TypeSizes::UNSIGNED_LONG_SIZE; }
    bool is_scalar() const override { return true; }
};

class ArrayType : public Type {
public:
    ArrayType(std::unique_ptr<Type> element_type, size_t array_size)
        : element_type { std::move(element_type) }
        , array_size { array_size }
    {
    }

    std::unique_ptr<Type> clone() const override
    {
        // Clone the return type
        auto cloned_referenced_type = element_type->clone();

        // Create and return new ArrayType with cloned components
        return std::make_unique<ArrayType>(
            std::move(cloned_referenced_type), array_size);
    }

    std::string to_string() const override
    {
        return std::format("[{}]{}", array_size, element_type->to_string());
    }

    bool equals(const Type& other) const override
    {
        // First check if other is also an ArrayType
        const auto* other_array = dynamic_cast<const ArrayType*>(&other);
        if (!other_array) {
            return false;
        }

        return element_type->equals(*other_array->element_type) && array_size == other_array->array_size;
    }

    std::unique_ptr<Type> element_type;
    size_t size() const override { return array_size * element_type->size(); }
    size_t alignment() const override { return element_type->alignment(); }
    size_t array_size;
};

template<typename T>
bool is_type(const Type& type)
{
    return dynamic_cast<const T*>(&type) != nullptr;
}
