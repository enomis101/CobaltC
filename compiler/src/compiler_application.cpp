#include "compiler/compiler_application.h"
#include "common/data/token.h"
#include "common/log/log.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem> // Requires C++17 or later
#include <format>
#include <fstream>
#include <sstream>
#include <vector>

CompilerApplication::CompilerApplication()
{
    // Force init the logger
    try {
        logging::LogManager::init();
    } catch (std::exception& e) {
        throw CompilerError(e.what());
    }
}

void CompilerApplication::run(const std::string& input_file, const std::string& operation)
{
    // Check if operation is valid
    std::vector<std::string> valid_operations = { "--lex", "--parse", "--codegen", "-S", "" };
    if (std::find(valid_operations.begin(), valid_operations.end(), operation) == valid_operations.end()) {
        throw CompilerError(std::format("Error: Invalid operation {} Use --lex, --parse, --codegen, or -S\n", operation));
    }

    // Check if input file has .c extension
    if (input_file.length() < 3 || input_file.substr(input_file.length() - 2) != ".c") {
        throw CompilerError("Error: Input file must have .c extension\n");
    }
    std::string base_name = get_base_name(input_file);
    std::string preprocessed_output_file = base_name + ".i";
    int result = preprocess_file(input_file, preprocessed_output_file);
    if (result != 0) {
        throw CompilerError(std::format("Error: Failed preprocessing of file {}\n", input_file));
    }

    FileCleaner file_cleaner;
    file_cleaner.push_back(preprocessed_output_file);

    std::vector<Token> tokens;

    // lexer
    try {
        Lexer lexer(preprocessed_output_file);
        tokens = lexer.tokenize();
        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string token_list = "Token List:\n";
            for (const Token& t : tokens) {
                token_list += t.to_string() + "\n";
            }
            LOG_DEBUG(LOG_CONTEXT, token_list);
        }
    } catch (std::exception& e) {
        throw CompilerError(std::format("Error: {}\n", e.what()));
    }

    if (operation == "--lex") {
        // Stop after lexer
        return;
    }

    // parser
    try {
        parser::Parser parser(tokens);
        std::unique_ptr<parser::Program> program_ast = parser.parse_program();
        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
        }
    } catch (std::exception& e) {
        throw CompilerError(std::format("Error: {}\n", e.what()));
    }
    
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

    file_cleaner.push_back(assembly_file);

    std::string output_file = base_name;
    int assemble_and_link_result = assemble_and_link(assembly_file, output_file);
    if (assemble_and_link_result) {
        throw CompilerError(std::format("Failed to assemble and link assembly file {}\n", assembly_file));
    }
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

    file << "\t.file\t\"return_2.c\"" << std::endl;
    file << "\t.text" << std::endl;
    file << "\t.globl\tmain" << std::endl;
    file << "\t.type\tmain, @function" << std::endl;
    file << "main:" << std::endl;
    file << "\tmovl\t$2, %eax" << std::endl;
    file << "\tret" << std::endl;
    file << "\t.size\tmain, .-main" << std::endl;
    file << "\t.ident\t\"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0\"" << std::endl;
    file << "\t.section\t.note.GNU-stack,\"\",@progbits" << std::endl;

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

void FileCleaner::cleanup_files()
{
    for (const auto& file : m_files) {
        std::remove(file.c_str());
    }
    m_files.clear();
}
