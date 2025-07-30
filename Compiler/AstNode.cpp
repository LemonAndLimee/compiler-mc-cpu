#include "AstNode.h"

/**
 * \brief  Returns an AST node instance from a given set of child nodes/tokens. Assigns the node label according to
 *         the set of elements. Throws if there are more than 1 elements of the node label type, or if no elements
 *         are in the elements container. If the given elements contains only 1 child AST node, this node will be
 *         returned instead of a new one being created.
 *
 * \param[in]  elements  Child nodes or tokens belonging to the node being created. Any token is either skipped,
 *                       selected as node label, or made into a child node.
 * \param[in]  nodeNt    The NT symbol to which this node's rule belongs. If an operator cannot be decided, this is
 *                       used as the node label.
 *
 * \return  Ptr to the created AST node.
 */
AstNode::Ptr
AstNode::GetNodeFromRuleElements(
    const Elements& elements,
    GrammarSymbols::NT nodeNt
)
{
    std::string ntString = GrammarSymbols::ConvertSymbolToString( nodeNt );
    LOG_INFO_MEDIUM_LEVEL( "Creating node for " + ntString + " with " + std::to_string( elements.size() ) 
                           + " elements." );
    
    if ( elements.empty() )
    {
        LOG_ERROR( "Tried to create node from zero elements." );
        throw std::runtime_error( "Tried to create node from zero elements." );
    }

    // Set to valid value if a terminal symbol label is found - if still invalid after all elements have been
    // checked, the node NT will be used.
    GrammarSymbols::Symbol nodeLabel = T::INVALID_TOKEN;
    // Children to be assigned to created node.
    AstNode::Children nodeChildren;

    // Search through elements for node label if exists - throws if more than 1 is found.
    for ( size_t i = 0; i < elements.size(); ++i )
    {
        Element element = elements[i];
        // If child is a token
        if ( std::holds_alternative< Token::Ptr >( element ) )
        {
            Token::Ptr token = std::get< Token::Ptr >( element );
            TokenType tokenType = token->m_type;
            // If token is node label type, mark it as such.
            if ( g_nodeLabelTerminals.end() != g_nodeLabelTerminals.find( tokenType ) )
            {
                // If a node label has already been found throw error
                if ( T::INVALID_TOKEN != nodeLabel )
                {
                    std::string errMsg = "Creating node for " + GrammarSymbols::ConvertSymbolToString( nodeNt );
                    errMsg += ": gathered elements have more than one node label type: ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( static_cast< TokenType >( nodeLabel ) ) + ", ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( tokenType );
                    LOG_ERROR( errMsg );
                    throw std::runtime_error( "Creating node: elements have more than one node label type." );
                }
                LOG_INFO_LOW_LEVEL( "Found terminal node label: " + TokenTypes::ConvertTokenTypeToString( tokenType ) );
                nodeLabel = tokenType;
            }
            // Else if token type can be skipped, ignore
            else if ( g_skipForAstTerminals.end() != g_skipForAstTerminals.find( tokenType ) )
            {
                continue;
            }
            // Otherwise, create new wrapper node and add to children
            else
            {
                AstNode::Ptr wrapperNode = std::make_shared< AstNode >( tokenType, token );
                nodeChildren.push_back( wrapperNode );
            }
        }

        // Else if element is an AST node, add to children
        else
        {
            AstNode::Ptr node = std::get< AstNode::Ptr >( element );
            nodeChildren.push_back( node );
        }
    }

    std::string nodeLabelString = TokenTypes::ConvertTokenTypeToString( static_cast< TokenType >( nodeLabel ) );
    // If a terminal node label was not found, use non-terminal argument instead
    if ( T::INVALID_TOKEN == nodeLabel )
    {
        // If no node label found AND children empty, this means all elements were skipped
        if ( nodeChildren.empty() )
        {
            LOG_ERROR( "All node elements skipped: cannot create a node with no elements." );
            throw std::runtime_error( "All node elements skipped: cannot create a node with no elements." );
        }
        // If no node label found AND only one child, this means the only significant element/s
        // was an AST node. In this case, we can return that node instead
        else if ( 1u == nodeChildren.size() )
        {
            LOG_INFO_MEDIUM_LEVEL( "Method was passed one node element: returning this node." );
            return nodeChildren[0];
        }
        nodeLabel = nodeNt;
        nodeLabelString = ntString;
    }

    LOG_INFO_MEDIUM_LEVEL( "Creating node with label: " + nodeLabelString );
    return std::make_shared< AstNode >( nodeLabel, nodeChildren );
}

/**
 * \brief  Indicates whether node is storing anything, i.e. a token or child nodes.
 * 
 * \return  True if storage is in use, false otherwise.
 */
bool
AstNode::IsStorageInUse() {
    if ( std::holds_alternative< Token::Ptr >( m_storage ) )
    {
        return nullptr != std::get< Token::Ptr >( m_storage );
    }
    else
    {
        return !std::get< Children >( m_storage ).empty();
    }
}