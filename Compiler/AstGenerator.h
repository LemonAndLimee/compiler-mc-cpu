/**
 * Declaration of class responsible for generating an Abstract Syntax Tree.
 */

#pragma once
#include "Grammar.h"
#include "AstNode.h"

using namespace GrammarRules;

class AstGenerator
{
public:
    using Ptr = std::shared_ptr< AstGenerator >;
    using UPtr = std::unique_ptr< AstGenerator >;
    AstGenerator() = default;

    AstNode::Ptr GenerateAst( const Tokens& tokens, GrammarSymbols::NT startingNt, bool allowLeftoverTokens );

private:
    bool TryRule( const Tokens& tokens, const Rule& rule, std::vector< AstNode::Child >& children, size_t& tokenIndex );

    AstNode::Ptr CreateNodeFromChildren( const std::vector< AstNode::Child >& children );
};