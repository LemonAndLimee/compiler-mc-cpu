#include "AstNode.h"

/**
 * \brief  Creates an AST node instance from a given set of child nodes/tokens. Assigns the node label according to
 *         the set of children.
 *
 * \param[in]  children  Child nodes or tokens belonging to the node being created.
 * \param[in]  nodeNt    The NT symbol to which this node's rule belongs. If an operator cannot be decided, this is
 *                       used as the node label.
 *
 * \return  Ptr to the created AST node.
 */
AstNode::Ptr
AstNode::CreateNodeFromChildren(
    const std::vector< AstNode::Child >& children,
    GrammarSymbols::NT nodeNt
)
{
    std::string ntString = GrammarSymbols::ConvertSymbolToString( nodeNt );
    LOG_INFO_MEDIUM_LEVEL( "Creating node for " + ntString + " with " + std::to_string( children.size() ) 
                           + " children." );
    // Set to valid value if a terminal symbol label is found - if still invalid after all children have been
    // checked, the node NT will be used.
    GrammarSymbols::T terminalNodeLabel = T::INVALID_TOKEN;
    size_t terminalNodeLabelIndex;

    // Search through children for node label if exists - throws if more than 1 is found.
    for ( size_t i = 0; i < children.size(); ++i )
    {
        AstNode::Child child = children[i];
        // If child is a token
        if ( const Token::Ptr* pToken = std::get_if< Token::Ptr >( &child ) )
        {
            TokenType tokenType = ( *pToken )->m_type;
            // If token is node label type, mark it as such.
            if ( g_nodeLabelTerminals.end() != g_nodeLabelTerminals.find( tokenType ) )
            {
                if ( T::INVALID_TOKEN != terminalNodeLabel )
                {
                    std::string errMsg = "Creating node for " + GrammarSymbols::ConvertSymbolToString( nodeNt );
                    errMsg += ": gathered children have more than one node label type: ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( terminalNodeLabel ) + ", ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( tokenType );
                    LOG_ERROR( errMsg );
                    throw std::runtime_error( "Creating node: children have more than one node label type." );
                }
                LOG_INFO_LOW_LEVEL( "Found terminal node label: " + TokenTypes::ConvertTokenTypeToString( tokenType ) );
                terminalNodeLabel = tokenType;
                terminalNodeLabelIndex = i;
            }
        }
    }

    // If a terminal node label was found, use it and remove it from children
    if ( T::INVALID_TOKEN != terminalNodeLabel )
    {
        LOG_INFO_MEDIUM_LEVEL( "Creating node with label: " + TokenTypes::ConvertTokenTypeToString( terminalNodeLabel ) );
        std::vector< AstNode::Child > childrenCopy = children;
        childrenCopy.erase( childrenCopy.begin() + terminalNodeLabelIndex );
        return std::make_shared< AstNode >( terminalNodeLabel, childrenCopy );
    }
    else
    {
        LOG_INFO_MEDIUM_LEVEL( "Creating node with label: " + ntString );
        // Else use NT label and original set of children
        return std::make_shared< AstNode >( nodeNt, children );
    }
}