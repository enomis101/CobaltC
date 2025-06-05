#pragma once
#include <string>

class FileLocation
{
public:
    explicit FileLocation(const std::string& file_name)
        : m_file_name{file_name}{}

    const std::string& file_name() const {return m_file_name;}
    size_t line_number() const {return m_line_number;}
    size_t column_number() const {return m_column_number;}

    void reset(const std::string& new_file_name, size_t new_line_num){
        m_file_name = new_file_name;
        m_line_number = new_line_num;
        m_column_number = 1;
    }

    void withespace(){
        m_column_number++;
    }
    void new_line(){
        m_line_number++;
        m_column_number = 1;
    }
private:
    std::string m_file_name;
    size_t m_line_number{1};
    size_t m_column_number{1};
};