#pragma once
#include "common/data/name_generator.h"
#include "common/data/symbol_table.h"
#include "parser/parser_ast.h"

namespace parser {

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParserAST> ast, std::shared_ptr<NameGenerator> name_generator, std::shared_ptr<SymbolTable> symbol_table)
        : m_ast { ast }
        , m_name_generator { name_generator }
        , m_symbol_table { symbol_table }
    {
    }
    void analyze();

private:
    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<NameGenerator> m_name_generator;
    std::shared_ptr<SymbolTable> m_symbol_table;
};
}
