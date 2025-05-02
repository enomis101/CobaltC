#pragma once
#include <string>

class NameGenerator{
public:
    static NameGenerator& instance();
    std::string make_temporary(const std::string& name = "tmp");
private:
    NameGenerator(){}

    int m_counter{0};
};