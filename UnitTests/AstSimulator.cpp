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
    bool isNewVar,
    uint8_t value
)
{
    Token::Ptr valueToken = std::make_shared< Token >( TokenType::BYTE, value );
    return CreateAssignStatementSubtree( varName, isNewVar, valueToken );
}

/**
 * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from another variable.
 *         Delegates to CreateAssignStatementSubtree().
 */
AstNode::Ptr
AstSimulator::CreateAssignNodeFromVar(
    const std::string& varName,
    bool isNewVar,
    const std::string& valueVar
)
{
    Token::Ptr valueToken = std::make_shared< Token >( TokenType::IDENTIFIER, valueVar );
    return CreateAssignStatementSubtree( varName, isNewVar, valueToken );
}

/**
 * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from a value specified
 *         by a token.
 *
 * \param[in]  varName     The name of the new variable.
 * \param[in]  isNewVar    Whether the LHS variable is new, i.e. needs declaring.
 * \param[in]  valueToken  Token representing value to assign to the variable. Can be literal or identifier.
 *
 * \return  Assignment AST node. Root of the created subtree.
 */
AstNode::Ptr
AstSimulator::CreateAssignStatementSubtree(
    const std::string& varName,
    bool isNewVar,
    Token::Ptr valueToken
)
{
    // LHS
    AstNode::Ptr lhsNode;

    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, varName );
    AstNode::Ptr idNode = std::make_shared< AstNode >( TokenType::IDENTIFIER, idToken );

    // If is new var, nest it inside a variable subtree.
    if ( isNewVar )
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
