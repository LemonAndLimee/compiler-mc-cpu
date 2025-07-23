/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include <stdexcept>

/**
 * \brief Generates an Abstract Syntax tree from a given set of tokens. If the syntax of the tokens
 *        is invalid, it throws an error.
 *
 * \param[in]  tokens        String of tokens representing the program to be converted.
 * \param[in]  startingRule  Non-terminal symbol identifying the rule to start the tree generation from.
 *
 * \return  Pointer to the root of the generated tree.
 */
AstNode::Ptr
AstGenerator::GenerateAst( TokensVector tokens, GrammarSymbols::NT startingRule )
{
    throw std::runtime_error( "Not implemented.");
}