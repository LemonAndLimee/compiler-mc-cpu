/**
 * Declaration of symbol table generator class. Responsible for traversing an AST and creating symbol tables for
 * each scope, performing some checks along the way.
 */

#pragma once

#include "SymbolTable.h"
#include "AstNode.h"

class SymbolTableGenerator
{
public:
    using UPtr = std::unique_ptr< SymbolTableGenerator >;
    using Ptr = std::shared_ptr< SymbolTableGenerator >;

    SymbolTableGenerator() = default;

    void GenerateSymbolTableForAst( AstNode::Ptr treeRootNode );
};