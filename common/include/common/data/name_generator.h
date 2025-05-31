#pragma once
#include <string>

class NameGenerator {
public:
    std::string make_temporary(const std::string& name = "tmp");
    std::string make_label(const std::string& in_label);
    NameGenerator() = default;

private:
    int m_counter { 0 };
    int m_label_counter { 0 };
};
