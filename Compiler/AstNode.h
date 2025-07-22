/**
 * Contains declaration of class representing a node in an Abstract Syntax Tree.
 */

#pragma once
#include "GrammarRules.h"
#include "Token.h"
#include <vector>
#include <memory>

class AstNode
{
public:
    using Ptr = std::shared_ptr< AstNode >;
    using Child = std::variant< AstNode::Ptr, Token::Ptr >;

    AstNode( GrammarSymbols::Symbol nodeLabel, std::vector< Child > children )
    : m_nodeLabel( nodeLabel ),
      m_children( std::move( children ) )
    {
    }

private:
    // Describes the relationship of the node, i.e. how its children relate to each other.
    // This can be a token type e.g. PLUS, or a non-terminal symbol label (e.g. For_init).
    GrammarSymbols::Symbol m_nodeLabel;

    // Child nodes in the tree - can be AST nodes or Tokens.
    std::vector< Child > m_children;
};