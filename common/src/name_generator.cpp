#include "common/data/name_generator.h"

// Meyer's Singleton implementation
NameGenerator& NameGenerator::instance()
{
    // Static local variable is initialized only once in a thread-safe manner (C++11 and later)
    static NameGenerator instance;
    return instance;
}

// Implementation of make_temporary() method
std::string NameGenerator::make_temporary(const std::string& name)
{
    // Generate a unique temporary name based on the counter
    return name + "." + std::to_string(m_counter++);
}

std::string NameGenerator::make_label(const std::string& label)
{
    return label + "." + std::to_string(m_label_counter++);
}
