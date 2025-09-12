#include "compiler/compiler_application.h"
#include "backend/assembly_generator.h"
#include "backend/assembly_printer.h"
#include "backend/backend_symbol_table.h"
#include "backend/code_emitter.h"
#include "common//data/source_manager.h"
#include "common/data/compile_options.h"
#include "common/data/token.h"
#include "common/data/token_table.h"
#include "common/data/warning_manager.h"
#include "common/log/log.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/parser_printer.h"
#include "parser/semantic_analyzer.h"
#include "parser/semantic_analyzer_error.h"
#include "parser/type_check_pass.h"
#include "parser/type_validator.h"
#include "tacky/tacky_generator.h"
#include "tacky/tacky_printer.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem> // Requires C++17 or later
#include <format>
#include <fstream>
#include <memory>
#include <regex>
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
    std::vector<std::string> valid_operations = { "--lex", "--parse", "--validate", "--tacky", "--codegen", "-S", "-c", "" };
    std::regex lib_regex("-l([a-zA-Z_][a-zA-Z0-9_]*)");
    bool is_lib_operation = std::regex_match(operation, lib_regex);
    if (std::find(valid_operations.begin(), valid_operations.end(), operation) == valid_operations.end() && !is_lib_operation) {
        throw CompilerError(std::format(
            "Invalid operation: '{}'\n"
            "Valid operations are: --lex, --parse, --validate, --tacky, --codegen, -S, -c, -l<lib> or no operation for full compilation",
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
    std::filesystem::path file_path(input_file);
    std::filesystem::path parent_path = file_path.parent_path();
    std::string base_name = file_path.stem().string();

    std::string preprocessed_output_file = parent_path / (base_name + ".i");

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

    std::shared_ptr<TokenTable> token_table = std::make_shared<TokenTable>();
    std::shared_ptr<NameGenerator> name_generator = std::make_shared<NameGenerator>();
    std::shared_ptr<SymbolTable> symbol_table = std::make_shared<SymbolTable>();
    std::shared_ptr<backend::BackendSymbolTable> backend_symbol_table = std::make_shared<backend::BackendSymbolTable>();
    std::shared_ptr<CompileOptions> compile_options = std::make_shared<CompileOptions>();
    std::shared_ptr<SourceManager> source_manager = std::make_shared<SourceManager>();
    std::shared_ptr<WarningManager> warning_manager = std::make_shared<WarningManager>();
    std::shared_ptr<std::vector<Token>> tokens;

    // HARD-CODING COMPILER OPTIONS
    compile_options->enable_assembly_comments = true;

    // Lexing stage
    try {
        LOG_INFO(LOG_CONTEXT, std::format("Lexing file '{}'", preprocessed_output_file));
        LexerContext lexer_context { preprocessed_output_file, token_table, source_manager, warning_manager };
        Lexer lexer(lexer_context);
        tokens = std::make_shared<std::vector<Token>>(lexer.tokenize());
        source_manager->set_token_list(tokens);
        LOG_INFO(LOG_CONTEXT, std::format("Lexing successful: {} tokens generated", tokens->size()));

        /*
        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string token_list = "Token List:\n";
            for (const Token& t : *tokens) {
                token_list += t.to_string() + "\n";
            }
            LOG_DEBUG(LOG_CONTEXT, token_list);
        }
        */
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

    std::shared_ptr<parser::ParserAST> parser_ast;
    // Parsing stage
    try {
        LOG_INFO(LOG_CONTEXT, "Starting parsing stage");

        parser::Parser parser(*tokens, source_manager);
        parser_ast = parser.parse_program();

        LOG_INFO(LOG_CONTEXT, "Parsing successful");

        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
            parser::PrinterVisitor printer;
            std::string base_name = file_path.stem().string();
            printer.generate_dot_file("debug/" + base_name + "_parserAST.dot", *(parser_ast.get()));
            LOG_DEBUG(LOG_CONTEXT, "Generated AST visualization in 'ast.dot'");
        }
    } catch (const parser::Parser::ParserError& e) {
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

    try {
        LOG_INFO(LOG_CONTEXT, "Starting Semantic Analysis stage");

        parser::SemanticAnalyzer semantic_analyzer(parser_ast, name_generator, symbol_table, source_manager, warning_manager);
        semantic_analyzer.analyze();
        parser::TypeValidator type_validator;
        type_validator.validate_types(*parser_ast);

        LOG_INFO(LOG_CONTEXT, "Semantic Analysis");

        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
            parser::PrinterVisitor printer;
            std::string base_name = file_path.stem().string();
            printer.generate_dot_file("debug/" + base_name + "_semantic_analysisAST.dot", *(parser_ast.get()));
            LOG_DEBUG(LOG_CONTEXT, "Generated AST visualization");
        }
    } catch (const parser::SemanticAnalyzerError& e) {
        throw CompilerError(std::format("Semantic Analysis error: {}", e.what()));
    }catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during Semantic Analysis stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "--validate") {
        LOG_INFO(LOG_CONTEXT, "Semantic Analysis operation completed successfully");
        return;
    }

    // Code generation stage
    LOG_INFO(LOG_CONTEXT, "Starting tacky generation stage");

    std::shared_ptr<tacky::TackyAST> tacky_ast;
    try {
        tacky::TackyGenerator tacky_generator(parser_ast, name_generator, symbol_table);
        tacky_ast = tacky_generator.generate();
        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
            tacky::PrinterVisitor printer;
            std::string base_name = file_path.stem().string();
            printer.generate_dot_file("debug/" + base_name + "_tackyAST.dot", *(tacky_ast.get()));
            LOG_DEBUG(LOG_CONTEXT, "Generated AST visualization in 'ast.dot'");
        }
    } catch (const tacky::TackyGeneratorError& e) {
        throw CompilerError(std::format("TackyGenerator: {}", e.what()));
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during tacky generation stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "--tacky") {
        LOG_INFO(LOG_CONTEXT, "Tacky generation operation completed successfully");
        return;
    }

    // Code generation stage
    LOG_INFO(LOG_CONTEXT, "Starting assembly generation stage");

    std::shared_ptr<backend::AssemblyAST> assembly_ast;
    try {
        backend::AssemblyGenerator assembly_generator(tacky_ast, symbol_table, backend_symbol_table, compile_options, name_generator);
        assembly_ast = assembly_generator.generate();
        if (logging::LogManager::logger()->is_enabled(LOG_CONTEXT, logging::LogLevel::DEBUG)) {
            std::string debug_str = "Parsed Program\n";
            LOG_DEBUG(LOG_CONTEXT, debug_str);
            backend::PrinterVisitor printer;
            std::string base_name = file_path.stem().string();
            printer.generate_dot_file("debug/" + base_name + "_assemblyAST.dot", *(assembly_ast.get()));
            LOG_DEBUG(LOG_CONTEXT, "Generated AssemblyAST visualization in 'ast.dot'");
        }
    } catch (const backend::AssemblyGeneratorError& e) {
        throw CompilerError(std::format("AssemblyGeneration: {}", e.what()));
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during assembly generation stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "--codegen") {
        LOG_INFO(LOG_CONTEXT, "Code generation operation completed successfully");
        return;
    }

    // Assembly generation
    std::string assembly_file = parent_path / (base_name + ".s");
    LOG_INFO(LOG_CONTEXT, std::format("Generating assembly file '{}'", assembly_file));

    try {
        backend::CodeEmitter code_emitter(assembly_file, assembly_ast, backend_symbol_table);
        code_emitter.emit_code();
    } catch (const backend::CodeEmitterError& e) {
        throw CompilerError(std::format("CodeEmitter error: {}", e.what()));
    } catch (const std::exception& e) {
        throw CompilerError(std::format(
            "Unexpected error during code emission stage: {}\n"
            "This may indicate a bug in the compiler - please report this issue",
            e.what()));
    }

    if (operation == "-S") {
        LOG_INFO(LOG_CONTEXT, "Assembly generation completed successfully");
        return;
    }

    file_cleaner.push_back(assembly_file);

    // Assembly and linking stage
    std::string output_file = parent_path / base_name;

    bool skip_linking = operation == "-c";

    if (skip_linking) {
        output_file += ".o";
    }

    LOG_INFO(LOG_CONTEXT, std::format("Assembling and linking '{}' to '{}'", assembly_file, output_file));

    int assemble_and_link_result = assemble_and_link(assembly_file, output_file, skip_linking, is_lib_operation ? operation : "");
    if (assemble_and_link_result) {
        throw CompilerError(std::format(
            "Failed to assemble and link file '{}' to '{}' with error code {}\n"
            "Ensure GCC is installed and accessible in your PATH",
            assembly_file, output_file, assemble_and_link_result));
    }

    LOG_INFO(LOG_CONTEXT, std::format("Compilation successful: Generated file '{}'", output_file));
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

int CompilerApplication::assemble_and_link(const std::string& assembly_file, const std::string& output_file, bool skip_linking, const std::string& lib_operation)
{
    // Build the command string
    std::string command = "gcc";
    std::string flags = skip_linking ? " -c " : " ";
    std::string file_extension = output_file.substr(output_file.length() - 2);
    command += flags + assembly_file + " -o " + output_file;
    if (lib_operation != "") {
        command += " " + lib_operation;
    }
    if (skip_linking && file_extension != ".o") {
        throw CompilerError(std::format("Output file must have .o extension"));
    }

    LOG_DEBUG(LOG_CONTEXT, std::format("Assembling and linking command: {}", command));

    // Execute the command
    int result = std::system(command.c_str());

    if (result != 0) {
        throw CompilerError(std::format("Assembling and linking failed with error code {}", result));
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
