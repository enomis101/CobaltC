#pragma once
#include <string>

struct SourceLocation {
    std::string file_name;
    size_t line_number;
    size_t column_number;

    SourceLocation(const std::string& fname, size_t line = 1, size_t col = 1)
        : file_name(fname)
        , line_number(line)
        , column_number(col)
    {
    }
};

struct SourceLocationIndex {
    size_t index;
    explicit SourceLocationIndex(size_t index)
        : index(index)
    {
    }
};
