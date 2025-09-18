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
    return CreateAssignStatementFromToken( varName, valueToken, isDeclaration );
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
    return CreateAssignStatementFromToken( varName, valueToken, isDeclaration );
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
AstSimulator::CreateAssignStatementFromToken(
    const std::string& varName,
    Token::Ptr valueToken,
    IsDeclaration isDeclaration
)
{
    // LHS
    AstNode::Ptr lhsNode = GetLhsIdNode( varName, isDeclaration );

    // RHS
    AstNode::Ptr valueNode = std::make_shared< AstNode >( valueToken->m_type, valueToken );

    // Construct parent node
    AstNode::Children children{ lhsNode, valueNode };
    AstNode::Ptr assignNode = std::make_shared< AstNode >( TokenType::ASSIGN, children );

    return assignNode;
}

/**
 * \brief  Get AST node for a LHS identifier - holds token or declaration sub-tree.
 *
 * \param[in]  lhs            The identifier of the LHS variable being written to.
 * \param[in]  isDeclaration  Whether the LHS variable is being declared for this first time.
 *
 * \return  The created variable node.
 */
AstNode::Ptr
AstSimulator::GetLhsIdNode(
    const std::string& varName,
    IsDeclaration isDeclaration
)
{
    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, varName );
    AstNode::Ptr idNode = std::make_shared< AstNode >( TokenType::IDENTIFIER, idToken );

    // If is new var, nest it inside a variable subtree.
    if ( isDeclaration )
    {
        Token::Ptr dataTypeToken = std::make_shared< Token >( TokenType::DATA_TYPE, DataType::DT_BYTE );
        AstNode::Ptr dataTypeNode = std::make_shared< AstNode >( TokenType::DATA_TYPE, dataTypeToken );

        AstNode::Children varNodeChildren{ dataTypeNode, idNode };
        AstNode::Ptr variableNode = std::make_shared< AstNode >( NT::Variable, varNodeChildren );

        return variableNode;
    }
    else
    {
        return idNode;
    }
}

/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    uint8_t operand1,
    uint8_t operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::BYTE, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::BYTE, token1 );

    Token::Ptr token2 = std::make_shared< Token >( TokenType::BYTE, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::BYTE, token2 );

    return CreateTwoOpExpression( operation, node1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    uint8_t operand1,
    const std::string& operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::BYTE, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::BYTE, token1 );

    Token::Ptr token2 = std::make_shared< Token >( TokenType::IDENTIFIER, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::IDENTIFIER, token2 );

    return CreateTwoOpExpression( operation, node1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    const std::string& operand1,
    uint8_t operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::IDENTIFIER, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::IDENTIFIER, token1 );

    Token::Ptr token2 = std::make_shared< Token >( TokenType::BYTE, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::BYTE, token2 );

    return CreateTwoOpExpression( operation, node1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    const std::string& operand1,
    const std::string& operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::IDENTIFIER, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::IDENTIFIER, token1 );

    Token::Ptr token2 = std::make_shared< Token >( TokenType::IDENTIFIER, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::IDENTIFIER, token2 );

    return CreateTwoOpExpression( operation, node1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    uint8_t operand1,
    AstNode::Ptr operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::BYTE, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::BYTE, token1 );

    return CreateTwoOpExpression( operation, node1, operand2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    AstNode::Ptr operand1,
    uint8_t operand2
)
{
    Token::Ptr token2 = std::make_shared< Token >( TokenType::BYTE, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::BYTE, token2 );

    return CreateTwoOpExpression( operation, operand1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    AstNode::Ptr operand1,
    const std::string& operand2
)
{
    Token::Ptr token2 = std::make_shared< Token >( TokenType::IDENTIFIER, operand2 );
    AstNode::Ptr node2 = std::make_shared< AstNode >( T::IDENTIFIER, token2 );

    return CreateTwoOpExpression( operation, operand1, node2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    const std::string& operand1,
    AstNode::Ptr operand2
)
{
    Token::Ptr token1 = std::make_shared< Token >( TokenType::IDENTIFIER, operand1 );
    AstNode::Ptr node1 = std::make_shared< AstNode >( T::IDENTIFIER, token1 );

    return CreateTwoOpExpression( operation, node1, operand2 );
}
/**
 * \brief  Constructs expression node with 2 operands.
 *
 * \param[in]  operation  The expression operation type - this will be the node label.
 * \param[in]  operand1   The LHS operand of the expression.
 * \param[in]  operand2   The RHS operand of the expression.
 *
 * \return  The created expression node.
 */
AstNode::Ptr
AstSimulator::CreateTwoOpExpression(
    T operation,
    AstNode::Ptr operand1,
    AstNode::Ptr operand2
)
{
    AstNode::Children expressionChildren{ operand1, operand2 };
    AstNode::Ptr expressionNode = std::make_shared< AstNode >( operation, expressionChildren );
    return expressionNode;
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

/**
 * \brief  Creates a symbol table with mock entries for the given list of identifiers, and attaches to the given node.
 *
 * \param[in]  scopeNode    Node to attach the symbol table to.
 * \param[in]  identifiers  Collection of identifiers for which to create entries.
 * \param[in]  parentTable  The parent symbol table for the table being created. Null by default.
 */
void
AstSimulator::CreateAndAttachFakeSymbolTable(
    AstNode::Ptr scopeNode,
    std::vector< std::string > identifiers,
    SymbolTable::Ptr parentTable //= nullptr
)
{
    SymbolTable::Ptr table = std::make_shared< SymbolTable >( parentTable );
    for ( auto identifier : identifiers )
    {
        SymbolTableEntry::Ptr entry = std::make_shared< SymbolTableEntry >();
        table->AddEntry( identifier, entry );
    }
    scopeNode->m_symbolTable = table;
}