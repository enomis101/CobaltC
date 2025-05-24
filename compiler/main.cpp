#include "common/log/log.h"
#include "compiler/compiler_application.h"
#include <format>
#include <iostream>
#include <string>

constexpr const char* LOG_CONTEXT = "compiler";

void print_error(const std::string& message)
{
    std::cerr << "\n\033[1;31mERROR\033[0m: " << message << std::endl;
}

void print_usage(const char* program_name)
{
    std::cerr << "\nUsage: " << program_name << " INPUT_FILE.c [--operation]" << std::endl;
    std::cerr << "\nOperations:" << std::endl;
    std::cerr << "  --lex      Stop after lexical analysis" << std::endl;
    std::cerr << "  --parse    Stop after parsing" << std::endl;
    std::cerr << "  --tacky    Stop after tacky generation" << std::endl;
    std::cerr << "  --codegen  Stop after code generation" << std::endl;
    std::cerr << "  -S         Stop after assembly generation" << std::endl;
    std::cerr << "  No option  Perform full compilation" << std::endl;
    std::cerr << "\nExample:" << std::endl;
    std::cerr << "  " << program_name << " myprogram.c      # Full compilation" << std::endl;
    std::cerr << "  " << program_name << " myprogram.c -S   # Generate assembly only" << std::endl;
}

int main(int argc, char* argv[])
{
    // Check if correct number of arguments were provided
    if (argc < 2 || argc > 3) {
        print_error("Incorrect number of arguments");
        print_usage(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string operation;

    // Parse command line arguments
    if (argc == 3) {
        // The command format is: program OPERATION INPUT_FILE
        operation = argv[1];
        input_file = argv[2];

        // Check if the operation starts with '-' or '--'
        if (operation[0] != '-') {
            // Might be reversed order
            std::swap(operation, input_file);
        }
    } else {
        // Single argument must be the input file
        input_file = argv[1];
        operation = "";
    }

    LOG_DEBUG(LOG_CONTEXT, std::format("Starting compiler with input file: '{}', operation: '{}'", input_file, operation.empty() ? "full compilation" : operation));

    try {
        CompilerApplication app;

        // Display compilation start message
        LOG_INFO(LOG_CONTEXT, std::format("Compiling '{}'{}\n", input_file, operation.empty() ? "" : std::format(" with operation: {}", operation)));

        app.run(input_file, operation);

        // Display compilation success message
        LOG_INFO(LOG_CONTEXT, std::format("Successfully completed operation on '{}'\n", input_file));

    } catch (const CompilerError& e) {
        LOG_CRITICAL(LOG_CONTEXT, std::format("Compilation failed: {} for file: {}", e.what(), input_file));
        return 1;
    } catch (const std::exception& e) {
        LOG_CRITICAL(LOG_CONTEXT, std::format("Unexpected error: {} for file: {}", e.what(), input_file));
        return 1;
    }

    return 0;
}
