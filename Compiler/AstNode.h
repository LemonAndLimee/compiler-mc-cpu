/**
 * Contains declaration of class representing a node in an Abstract Syntax Tree.
 */

#pragma once
#include "Grammar.h"
#include "Token.h"
#include <vector>
#include <memory>
#include <variant>

class AstNode
{
public:
    using Ptr = std::shared_ptr< AstNode >;

    /**
     * \brief  Represents the information held by an AST node: can either be the node itself or a token, which is
     *         either skipped during tree creation, or incorporated as a child node or as the node label.
     */
    using Element = std::variant< AstNode::Ptr, Token::Ptr >;
    using Elements = std::vector< Element >;

    using Children = std::vector< Ptr >;

    AstNode( GrammarSymbols::Symbol nodeLabel, const Children& children )
    : m_nodeLabel( nodeLabel ),
      m_storage( children )
    {}
    AstNode( GrammarSymbols::Symbol nodeLabel, Token::Ptr token )
    : m_nodeLabel( nodeLabel ),
      m_storage ( token )
    {}

    static AstNode::Ptr GetNodeFromRuleElements( const Elements& elements,
                                                 GrammarSymbols::NT nodeNt );
    
    bool IsStorageInUse();
    bool IsStoringToken();

    // Describes the relationship of the node, i.e. how its children relate to each other.
    // This can be a token type e.g. PLUS, or a non-terminal symbol label (e.g. For_init).
    GrammarSymbols::Symbol m_nodeLabel;

    // The element stored by this node: A node can only store a token or a set of child nodes, not both.
    // A node acts as a wrapper around a token, or as a more complex sub-tree.
    std::variant< Token::Ptr, Children > m_storage;
};