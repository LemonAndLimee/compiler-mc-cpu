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

    AstNode::Ptr GenerateAst( Tokens& tokens,
                              GrammarSymbols::NT startingNt,
                              bool allowLeftoverTokens );

private:
    bool TryRule( Tokens& tokens,
                  const Rule& rule,
                  bool allowLeftoverTokens,
                  std::vector< AstNode::Child >& children );

    AstNode::Ptr CreateNodeFromChildren( const std::vector< AstNode::Child >& children,
                                         GrammarSymbols::NT nodeNt );
};