#include "compiler/compiler_application.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem> // Requires C++17 or later
#include <format>
#include <fstream>
#include <sstream>
#include <vector>

void CompilerApplication::run(const std::string& input_file, const std::string& operation)
{
    // Check if operation is valid
    std::vector<std::string> validOperations = { "--lex", "--parse", "--codegen", "-S", "" };
    if (std::find(validOperations.begin(), validOperations.end(), operation) == validOperations.end()) {
        throw CompilerError("Error: Invalid operation. Use --lex, --parse, --codegen, or -S\n");
    }

    // Check if input file has .c extension
    if (input_file.length() < 3 || input_file.substr(input_file.length() - 2) != ".c") {
        throw CompilerError("Error: Input file must have .c extension\n");
    }
    std::string base_name = get_base_name(input_file);
    std::string preprocessed_output_file = base_name + ".i";
    int result = preprocess_file(input_file, preprocessed_output_file);
    if (result != 0) {
        throw CompilerError(std::format("Failed preprocessing of file {}\n", input_file));
    }

    std::vector<std::string> files_to_cleanup;
    files_to_cleanup.push_back(preprocessed_output_file);

    // lexer

    if (operation == "--lex") {
        // Stop after lexer
        return;
    }

    // parser

    if (operation == "--parse") {
        // Stop aftert parser
        return;
    }

    // assembly_generation

    if (operation == "--codegen") {
        // Stop after assembly generation
        return;
    }

    // fill assembly file
    std::string assembly_file = base_name + ".s";
    if (!create_stub_assembly_file(assembly_file)) {
        throw CompilerError(std::format("Failed to create assembly file {}\n", assembly_file));
    }

    if (operation == "-S") {
        // Stop after generating assembly file
        return;
    }

    files_to_cleanup.push_back(assembly_file);

    std::string output_file = base_name;
    int assemble_and_link_result = assemble_and_link(assembly_file, output_file);
    if (assemble_and_link_result) {
        throw CompilerError(std::format("Failed to assemble and link assembly file {}\n", assembly_file));
    }

    cleanup_files(files_to_cleanup);
}

// Function to get the base name of a file (without directory and extension)
std::string CompilerApplication::get_base_name(const std::string& file_path)
{
    namespace fs = std::filesystem;
    fs::path path(file_path);
    return path.stem().string();
}

bool CompilerApplication::create_stub_assembly_file(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << ".globl main" << std::endl;
    file << "main:" << std::endl;
    file << "movl" << std::endl;
    file << "$2, %eax" << std::endl;
    file << "ret" << std::endl;

    file.close();

    return true;
}

int CompilerApplication::preprocess_file(const std::string& input_file, const std::string& output_file)
{

    // Build the command string
    std::string command = "gcc -E -P";

    command += " " + input_file + " -o " + output_file;

    // Execute the command
    return std::system(command.c_str());
}

int CompilerApplication::assemble_and_link(const std::string& assembly_file, const std::string& output_file)
{
    // Build the command string
    std::string command = "gcc ";

    command += " " + assembly_file + " -o " + output_file;

    // Execute the command
    return std::system(command.c_str());
}

void CompilerApplication::cleanup_files(const std::vector<std::string>& files)
{
}
