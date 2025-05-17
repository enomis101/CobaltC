#pragma once
#include "common/data/name_generator.h"
#include "parser/parser_ast.h"

namespace parser {

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParserAST> ast)
        : m_ast { ast }
        , m_name_generator { NameGenerator::instance() }
    {
    }
    void analyze();

private:
    std::shared_ptr<ParserAST> m_ast;
    NameGenerator& m_name_generator;
};
}
