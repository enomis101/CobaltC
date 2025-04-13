#include "common/log/log.h"
#include "compiler/compiler_application.h"
#include <iostream>
#include <string>
#include <format>

constexpr const char* LOG_CONTEXT = "compiler";

int main(int argc, char* argv[])
{
    // Check if correct number of arguments were provided
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " INPUT_FILE.c [--lex|--parse|--codegen|-S]" << std::endl;
        return 1;
    }

    std::string input_file;
    std::string operation;
    if(argc == 3)
    {
        input_file = argv[2];
        operation = argv[1];
    }
    else{
        input_file = argv[1];
        operation = "";
    }

    LOG_DEBUG(LOG_CONTEXT, std::format("Started compiler with arguments: {} , {}", input_file, operation));
    CompilerApplication app;
    try {
        app.run(input_file, operation);
    } catch (CompilerError& e) {
        LOG_CRITICAL(LOG_CONTEXT, e.what());
        return 1;
    }
    return 0;
}
