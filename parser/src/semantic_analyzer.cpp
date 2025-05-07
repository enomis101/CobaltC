#include "parser/semantic_analyzer.h"
#include <format>
#include "parser/variable_resolution_pass.h"
#include "parser/loop_labeling_pass.h"

using namespace parser;

void SemanticAnalyzer::analyze()
{
    VariableResolutionPass var_pass(m_ast);
    var_pass.run();
    LoopLabelingPass loop_pass(m_ast);
    loop_pass.run();
}
