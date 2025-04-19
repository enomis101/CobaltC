#include "asmgen/asmgen_builder.h"
#include <fstream>
#include <string>
#include <cassert>


using namespace asmgen;

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<parser::ParserAST> ast)
    : m_ast{ast}
{
    if(!m_ast || !dynamic_cast<parser::Program*>(m_ast.get())){
        throw AsmGenError("AssemblyGenerator: Invalid AST");
    }
}

void AssemblyGenerator::visit(parser::Identifier& node)
{
    m_result = std::make_unique<Identifier>(node.name);
}

void AssemblyGenerator::visit(parser::ConstantExpression& node)
{
    m_result = std::make_unique<ImmediateValue>(node.value);
}

void AssemblyGenerator::visit(parser::ReturnStatement& node)
{
    if (!node.expression) {
        throw AsmGenError("Invalid parser::ReturnStatement node found during Assembly Generation");
    }
    node.expression->accept(*this);
    std::unique_ptr<Operand> src = consume_result<Operand>();
    std::unique_ptr<Operand> dst = std::make_unique<Register>();
    assert(m_instructions_result.empty() && "m_instructions_result must be empty");
    m_instructions_result.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));
    m_instructions_result.emplace_back(std::make_unique<ReturnInstruction>());
}

void AssemblyGenerator::visit(parser::Function& node)
{
    if (!node.name) {
        throw AsmGenError("Invalid parser::Function node found during Assembly Generation");
    }
    node.name->accept(*this);
    std::unique_ptr<Identifier> identifier = consume_result<Identifier>();

    if (!node.body) {
        throw AsmGenError("Invalid parser::Function node found during Assembly Generation");
    }
    node.body->accept(*this);
    std::vector<std::unique_ptr<Instruction>> instructions_result = consume_instructions_result();
    m_result = std::make_unique<Function>(std::move(identifier), std::move(instructions_result));
}

void AssemblyGenerator::visit(parser::Program& node)
{
    if (!node.function) {
        throw AsmGenError("Invalid parser::Program node found during Assembly Generation");
    }
    node.function->accept(*this);
    std::unique_ptr<Function> function = consume_result<Function>();
    m_result = std::make_unique<Program>(std::move(function));
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::consume_instructions_result()
{
    if(m_instructions_result.empty()){
        throw AsmGenError("Expected instructions result found empty");
    }
    std::vector<std::unique_ptr<Instruction>> res =  std::move(m_instructions_result);
    m_instructions_result.clear();
    return res;
}

std::unique_ptr<AsmGenAST> AssemblyGenerator::generate()
{
    m_ast->accept(*this);

    std::unique_ptr<Program> program = consume_result<Program>();
    return std::move(program);
}
