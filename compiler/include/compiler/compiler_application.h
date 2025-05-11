#pragma once
#include <stdexcept>
#include <string>
#include <vector>

class CompilerError : public std::runtime_error {
public:
    explicit CompilerError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class FileCleaner {
public:
    ~FileCleaner() { cleanup_files(); }
    void push_back(const std::string& file_name) { m_files.push_back(file_name); }

private:
    void cleanup_files();
    std::vector<std::string> m_files;
};

class CompilerApplication {
public:
    CompilerApplication();
    void run(const std::string& input_file, const std::string& operation);

private:
    // Function to get the base name of a file (without directory and extension)
    int preprocess_file(const std::string& input_file, const std::string& output_file);
    int assemble_and_link(const std::string& input_file, const std::string& output_file, bool skip_linking);
    bool create_stub_assembly_file(const std::string& filename);
    static constexpr const char* LOG_CONTEXT = "compiler";
};
