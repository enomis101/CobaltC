#include "parser/semantic_analyzer.h"
#include "parser/loop_labeling_pass.h"
#include "parser/identifier_resolution_pass.h"
#include <format>

using namespace parser;

void SemanticAnalyzer::analyze()
{
    IdentifierResolutionPass var_pass(m_ast);
    var_pass.run();
    LoopLabelingPass loop_pass(m_ast);
    loop_pass.run();
}
