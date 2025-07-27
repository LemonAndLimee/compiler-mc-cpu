/**
 * Declaration of class responsible for generating an Abstract Syntax Tree.
 */

#pragma once
#include "Grammar.h"
#include "AstNode.h"

using TokensVector = std::vector< Token::Ptr >;

class AstGenerator
{
public:
    using Ptr = std::shared_ptr< AstGenerator >;
    AstGenerator() = default;

    AstNode::Ptr GenerateAst( TokensVector& tokens, GrammarSymbols::NT startingNt, bool allowLeftoverTokens );

private:
    bool TryRule( const TokensVector& tokens, const Rule& rule, std::vector< AstNode::Child >& children, size_t& tokenIndex );
};