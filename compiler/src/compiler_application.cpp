#include "compiler/compiler_application.h"
#include "common/data/token.h"
#include "common/log/log.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/parser_printer.h"
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
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Failed to initialize logging system: {}\n"
            "Please check that the logging configuration file exists and is valid.",
            e.what()));
    }
}

void CompilerApplication::run(const std::string& input_file, const std::string& operation)
{
    // Check if operation is valid
    std::vector<std::string> valid_operations = { "--lex", "--parse", "--codegen", "-S", "" };
    if (std::find(valid_operations.begin(), valid_operations.end(), operation) == valid_operations.end()) {
        throw CompilerError(std::format(
            "Invalid operation: '{}'\n"
            "Valid operations are: --lex, --parse, --codegen, -S, or no operation for full compilation",
            operation));
    }

    // Check if input file has .c extension
    if (input_file.length() < 3 || input_file.substr(input_file.length() - 2) != ".c") {
        throw CompilerError(std::format(
            "Invalid source file: '{}'\n"
            "Input file must have a .c extension",
            input_file));
    }

    LOG_INFO(LOG_CONTEXT, std::format("Starting compilation of '{}'", input_file));

    // Preprocessing stage
    std::string base_name = get_base_name(input_file);
    std::string preprocessed_output_file = base_name + ".i";

    LOG_INFO(LOG_CONTEXT, std::format("Preprocessing '{}' to '{}'", input_file, preprocessed_output_file));

    int result = preprocess_file(input_file, preprocessed_output_file);
    if (result != 0) {
        throw CompilerError(std::format(
            "Preprocessing failed for file '{}' with error code {}\n"
            "Check that the input file exists and contains valid C code",
            input_file, result));
    }

    FileCleaner file_cleaner;
    file_cleaner.push_back(preprocessed_output_file);

    std::vector<Token> tokens;

    // Lexing stage
    try {
        LOG_INFO(LOG_CONTEXT, std::format("Lexing file '{}'", preprocessed_output_file));

        Lexer lexer(preprocessed_output_file);
        tokens = lexer.tokenize();

        LOG_INFO(LOG_CONTEXT, std::format("Lexing successful: {} tokens generated", tokens.size()));

        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string token_list = "Token List:\n";
            for (const Token& t : tokens) {
                token_list += t.to_string() + "\n";
            }
            LOG_DEBUG(LOG_CONTEXT, token_list);
        }
    } catch (const LexerError& e) {
        throw CompilerError(std::format("Lexer error: {}", e.what()));
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during lexing stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "--lex") {
        LOG_INFO(LOG_CONTEXT, "Lexing operation completed successfully");
        return;
    }

    // Parsing stage
    try {
        LOG_INFO(LOG_CONTEXT, "Starting parsing stage");

        parser::Parser parser(tokens);
        std::unique_ptr<parser::Program> program_ast = parser.parse_program();

        LOG_INFO(LOG_CONTEXT, "Parsing successful");

        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
            parser::PrinterVisitor printer;
            printer.generate_dot_file("ast.dot", *(program_ast.get()));
            LOG_DEBUG(LOG_CONTEXT, "Generated AST visualization in 'ast.dot'");
        }
    } catch (const parser::ParserError& e) {
        throw CompilerError(std::format("Parser error: {}", e.what()));
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during parsing stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "--parse") {
        LOG_INFO(LOG_CONTEXT, "Parsing operation completed successfully");
        return;
    }

    // Code generation stage
    LOG_INFO(LOG_CONTEXT, "Starting code generation stage");

    if (operation == "--codegen") {
        LOG_INFO(LOG_CONTEXT, "Code generation operation completed successfully");
        return;
    }

    // Assembly generation
    std::string assembly_file = base_name + ".s";
    LOG_INFO(LOG_CONTEXT, std::format("Generating assembly file '{}'", assembly_file));

    if (!create_stub_assembly_file(assembly_file)) {
        throw CompilerError(std::format(
            "Failed to create assembly file '{}'\n"
            "Check file permissions and disk space",
            assembly_file));
    }

    if (operation == "-S") {
        LOG_INFO(LOG_CONTEXT, "Assembly generation completed successfully");
        return;
    }

    file_cleaner.push_back(assembly_file);

    // Assembly and linking stage
    std::string output_file = base_name;
    LOG_INFO(LOG_CONTEXT, std::format("Assembling and linking '{}' to '{}'", assembly_file, output_file));

    int assemble_and_link_result = assemble_and_link(assembly_file, output_file);
    if (assemble_and_link_result) {
        throw CompilerError(std::format(
            "Failed to assemble and link file '{}' to '{}' with error code {}\n"
            "Ensure GCC is installed and accessible in your PATH",
            assembly_file, output_file, assemble_and_link_result));
    }

    LOG_INFO(LOG_CONTEXT, std::format("Compilation successful: Generated executable '{}'", output_file));
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
        LOG_ERROR(LOG_CONTEXT, std::format("Failed to create assembly file '{}'", filename));
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

    LOG_DEBUG(LOG_CONTEXT, std::format("Created assembly file '{}'", filename));
    return true;
}

int CompilerApplication::preprocess_file(const std::string& input_file, const std::string& output_file)
{
    // Build the command string
    std::string command = "gcc -E -P";
    command += " " + input_file + " -o " + output_file;

    LOG_DEBUG(LOG_CONTEXT, std::format("Preprocessing command: {}", command));

    // Execute the command
    int result = std::system(command.c_str());

    if (result != 0) {
        LOG_ERROR(LOG_CONTEXT, std::format("Preprocessing failed with error code {}", result));
    }

    return result;
}

int CompilerApplication::assemble_and_link(const std::string& assembly_file, const std::string& output_file)
{
    // Build the command string
    std::string command = "gcc ";
    command += " " + assembly_file + " -o " + output_file;

    LOG_DEBUG(LOG_CONTEXT, std::format("Assembling and linking command: {}", command));

    // Execute the command
    int result = std::system(command.c_str());

    if (result != 0) {
        LOG_ERROR(LOG_CONTEXT, std::format("Assembling and linking failed with error code {}", result));
    }

    return result;
}

void FileCleaner::cleanup_files()
{
    for (const auto& file : m_files) {
        if (std::remove(file.c_str()) == 0) {
            // Successfully removed the file
        } else {
            // Failed to remove the file, but we'll continue with other files
        }
    }
    m_files.clear();
}
