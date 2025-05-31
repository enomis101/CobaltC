#pragma once
#include <cstddef>

class Type {
public:
    virtual ~Type() = default;
};

class IntType : public Type {
public:
};

class FunctionType : public Type {
public:
    FunctionType(size_t pc)
        : parameters_count { pc }
    {
    }

    size_t parameters_count;

    // Implement operator== to compare two FunctionType objects
    bool operator==(const FunctionType& other) const
    {
        // Then check if parameters_count is equal
        return parameters_count == other.parameters_count;
    }
};
