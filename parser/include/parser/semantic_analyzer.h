#pragma once
#include "common/data/name_generator.h"
#include "common/data/source_manager.h"
#include "common/data/symbol_table.h"
#include "common/data/warning_manager.h"
#include "parser/parser_ast.h"

namespace parser {

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParserAST> ast, std::shared_ptr<NameGenerator> name_generator, std::shared_ptr<SymbolTable> symbol_table, std::shared_ptr<SourceManager> source_manager, std::shared_ptr<WarningManager> warning_manager)
        : m_ast { ast }
        , m_name_generator { name_generator }
        , m_symbol_table { symbol_table }
        , m_source_manager { source_manager }
        , m_warning_manager { warning_manager }
    {
    }
    void analyze();

private:
    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<NameGenerator> m_name_generator;
    std::shared_ptr<SymbolTable> m_symbol_table;
    std::shared_ptr<SourceManager> m_source_manager;
    std::shared_ptr<WarningManager> m_warning_manager;
};
}
