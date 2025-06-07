#pragma once
#include "common/data/source_location.h"
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

class SourceManager {
public:
    static std::optional<std::string> get_source_line(const SourceLocation& location)
    {
        std::ostringstream result;

        // Try to open the file
        std::ifstream file(location.file_name);
        if (!file.is_open()) {
            return std::nullopt;
        }

        // Skip to the error line
        std::string line;
        for (size_t i = 0; i < location.line_number && std::getline(file, line); ++i) {
            // Just reading lines until we reach the target
        }

        // Add the source line
        result << line << "\n";

        // Add the error marker
        for (size_t i = 1; i < location.column_number; ++i) {
            // Preserve tabs for proper alignment
            result << (i - 1 < line.length() && line[i - 1] == '\t' ? '\t' : ' ');
        }
        result << "^";

        return result.str();
    }
};
