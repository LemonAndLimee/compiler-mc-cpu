/**
 * Definition of symbol table generator class.
 */

#include "SymbolTableGenerator.h"

 /**
  * \brief  Creates symbol table to store information about all symbols within this scope of this AST, and stores it
  *         inside the root tree node. Recursively adds symbol tables for any scope-defining subtrees.
  *
  * \param[in]  treeRootNode  The root node of the AST defining the scope of the symbol table.
  */
void
SymbolTableGenerator::GenerateSymbolTableForAst(
    AstNode::Ptr treeRootNode
)
{

}