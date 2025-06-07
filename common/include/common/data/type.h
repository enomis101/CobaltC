#pragma once
#include <memory>
#include <typeinfo>
#include <variant>
#include <vector>

using ConstantType = std::variant<std::monostate, int, long>;

class Type {
public:
    virtual ~Type() = default;

    // Virtual equality comparison
    virtual bool equals(const Type& other) const = 0;

    // Operators deleted to force explicit use of equals()
    bool operator==(const Type& other) const = delete;
    bool operator!=(const Type& other) const = delete;
};

class IntType : public Type {
public:
    bool equals(const Type& other) const override
    {
        // Only equal if other is also IntType
        return typeid(*this) == typeid(other);
    }
};

class LongType : public Type {
public:
    bool equals(const Type& other) const override
    {
        // Only equal if other is also LongType
        return typeid(*this) == typeid(other);
    }
};

class FunctionType : public Type {
public:
    FunctionType(std::unique_ptr<Type> return_type, std::vector<std::unique_ptr<Type>> parameters_type)
        : return_type { std::move(return_type) }
        , parameters_type { std::move(parameters_type) }
    {
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
