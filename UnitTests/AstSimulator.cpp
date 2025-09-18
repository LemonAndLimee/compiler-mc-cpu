/**
 * Contains utility methods for creating simulated ASTs for test cases.
 */

#include "AstSimulator.h"

/**
 * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from a byte value.
 *         Delegates to CreateAssignStatementSubtree().
 */
AstNode::Ptr
AstSimulator::CreateAssignNodeFromByteValue(
    const std::string& varName,
    uint8_t value,
    IsDeclaration isDeclaration
)
{
    Token::Ptr valueToken = std::make_shared< Token >( TokenType::BYTE, value );
    return CreateAssignStatementSubtree( varName, valueToken, isDeclaration );
}

/**
 * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from another variable.
 *         Delegates to CreateAssignStatementSubtree().
 */
AstNode::Ptr
AstSimulator::CreateAssignNodeFromVar(
    const std::string& varName,
    const std::string& valueVar,
    IsDeclaration isDeclaration
)
{
    Token::Ptr valueToken = std::make_shared< Token >( TokenType::IDENTIFIER, valueVar );
    return CreateAssignStatementSubtree( varName, valueToken, isDeclaration );
}

/**
 * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from a value specified
 *         by a token.
 *
 * \param[in]  varName        The name of the new variable.
 * \param[in]  valueToken     Token representing value to assign to the variable. Can be literal or identifier.
 * \param[in]  isDeclaration  Whether the LHS variable is new, i.e. needs declaring.
 *
 * \return  Assignment AST node. Root of the created subtree.
 */
AstNode::Ptr
AstSimulator::CreateAssignStatementSubtree(
    const std::string& varName,
    Token::Ptr valueToken,
    IsDeclaration isDeclaration
)
{
    // LHS
    AstNode::Ptr lhsNode;

    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, varName );
    AstNode::Ptr idNode = std::make_shared< AstNode >( TokenType::IDENTIFIER, idToken );

    // If is new var, nest it inside a variable subtree.
    if ( isDeclaration )
    {
        Token::Ptr dataTypeToken = std::make_shared< Token >( TokenType::DATA_TYPE, DataType::DT_BYTE );
        AstNode::Ptr dataTypeNode = std::make_shared< AstNode >( TokenType::DATA_TYPE, dataTypeToken );

        AstNode::Children varNodeChildren{ dataTypeNode, idNode };
        AstNode::Ptr variableNode = std::make_shared< AstNode >( NT::Variable, varNodeChildren );

        lhsNode = variableNode;
    }
    else
    {
        lhsNode = idNode;
    }

    // RHS
    AstNode::Ptr valueNode = std::make_shared< AstNode >( valueToken->m_type, valueToken );

    // Construct parent node
    AstNode::Children children{ lhsNode, valueNode };
    AstNode::Ptr assignNode = std::make_shared< AstNode >( TokenType::ASSIGN, children );

    return assignNode;
}

/**
 * \brief  Constructs block node(s) around the given child AST nodes, in the expected max-2-children format.
 *
 * \param[in]  nodes  Nodes to wrap in a block node structure.
 *
 * \return  The top-most created block node.
 */
AstNode::Ptr
AstSimulator::WrapNodesInBlocks(
    AstNode::Children nodes
)
{
    if ( 1u == nodes.size() )
    {
        return std::make_shared< AstNode >( NT::Block, nodes );
    }

    AstNode::Ptr createdChild;
    for ( auto it = nodes.rbegin(); it != nodes.rend(); it++ )
    {
        if ( nullptr == createdChild )
        {
            createdChild = *it;
        }
        else
        {
            AstNode::Children blockChildren{ *it, createdChild };
            createdChild = std::make_shared< AstNode >( NT::Block, blockChildren );
        }
    }
    return createdChild;
}