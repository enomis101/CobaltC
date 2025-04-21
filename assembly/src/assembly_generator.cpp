#include "assembly/assembly_builder.h"
#include <fstream>
#include <string>
#include <cassert>


using namespace assembly;

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast)
    : m_ast{ast}
{
    if(!m_ast || !dynamic_cast<tacky::Program*>(m_ast.get())){
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid AST");
    }
}

std::unique_ptr<AssemblyAST> AssemblyGenerator::generate()
{

}
