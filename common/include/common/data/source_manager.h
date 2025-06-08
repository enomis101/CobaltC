#pragma once
#include "common/data/source_location.h"
#include "common/data/token.h"
#include <string>
#include <vector>

class SourceManager {
public:
    void set_token_list(std::shared_ptr<std::vector<Token>> token_list) { m_token_list = token_list; }

    std::string get_source_line(const SourceLocation& location) const;
    std::string get_source_line(const SourceLocationIndex& location) const;
    SourceLocationIndex get_index(const Token& token) const;

private:
    std::shared_ptr<std::vector<Token>> m_token_list;
};
