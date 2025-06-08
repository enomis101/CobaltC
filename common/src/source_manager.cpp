#include "common/data/source_manager.h"
#include <fstream>
#include <sstream>

std::string SourceManager::get_source_line(const SourceLocation& location) const
{
    std::ostringstream result;

    // Try to open the file
    std::ifstream file(location.file_name);
    if (!file.is_open()) {
        return "ERROR!";
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

std::string SourceManager::get_source_line(const SourceLocationIndex& location) const
{
    return get_source_line(m_token_list->at(location.index).source_location());
}

SourceLocationIndex SourceManager::get_index(const Token& token) const
{
    // Calculate index based on address
    size_t index = &token - &m_token_list->at(0);
    // Add bounds checking for safety
    if (index >= m_token_list->size()) {
        throw std::out_of_range("Token not found in token list");
    }
    return SourceLocationIndex(index);
}
