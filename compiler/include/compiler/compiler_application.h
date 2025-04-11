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

class CompilerApplication {
public:
    void run(const std::string& input_file, const std::string& operation);

private:
    // Function to get the base name of a file (without directory and extension)
    std::string get_base_name(const std::string& file_path);
    int preprocess_file(const std::string& input_file, const std::string& output_file);
    int assemble_and_link(const std::string& input_file, const std::string& output_file);
    void cleanup_files(const std::vector<std::string>& files);
    bool create_stub_assembly_file(const std::string& filename);
};
