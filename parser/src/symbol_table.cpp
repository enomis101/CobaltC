#include "parser/symbol_table.h"

using namespace parser;

// Meyer's Singleton implementation
SymbolTable& SymbolTable::instance()
{
    // Static local variable is initialized only once in a thread-safe manner (C++11 and later)
    static SymbolTable instance;
    return instance;
}
