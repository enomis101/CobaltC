#include "common/log/log.h"
#include "compiler/compiler_application.h"
#include <iostream>
#include <string>

constexpr const char* LOG_CONTEXT = "compiler";

int main(int argc, char* argv[])
{
    // Check if correct number of arguments were provided
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " INPUT_FILE.c [--lex|--parse|--codegen|-S]" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string operation = (argc == 3) ? argv[2] : "";
    CompilerApplication app;
    try {
        app.run(input_file, operation);
    } catch (CompilerError& e) {
        LOG_CRITICAL(LOG_CONTEXT, e.what());
        return 1;
    }
    return 0;
}
