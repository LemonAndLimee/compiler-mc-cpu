/**
 * Contains utility methods for creating simulated ASTs for test cases.
 */

#include "AstNode.h"

#pragma once

namespace AstSimulator
{
    enum IsDeclaration
    {
        TRUE = true,
        FALSE = false
    };
    AstNode::Ptr CreateAssignNodeFromByteValue( const std::string& varName, uint8_t value, IsDeclaration isDeclaration );
    AstNode::Ptr CreateAssignNodeFromVar( const std::string& varName, const std::string& valueVar, IsDeclaration isDeclaration );
    AstNode::Ptr CreateAssignStatementFromToken( const std::string& varName, Token::Ptr valueToken, IsDeclaration isDeclaration );

    AstNode::Ptr GetLhsIdNode( const std::string& varName, IsDeclaration isDeclaration );

    /**
     * \brief  Constructs assign statement with a RHS two-operand expression.
     *
     * \param[in]  lhs            The identifier of the LHS variable being written to.
     * \param[in]  isDeclaration  Whether the LHS variable is being declared for this first time.
     * \param[in]  operation      The expression operation type - this will be the node label.
     * \param[in]  operand1       The LHS operand of the expression.
     * \param[in]  operand2       The RHS operand of the expression.
     *
     * \return  The created assign node.
     */
    template < typename A, typename B >
    AstNode::Ptr
    CreateTwoOperandStatement(
        const std::string& lhs,
        IsDeclaration isDeclaration,
        T operation,
        A operand1,
        B operand2
    )
    {
        AstNode::Ptr lhsNode = GetLhsIdNode( lhs, isDeclaration );
        AstNode::Ptr expressionNode = CreateTwoOpExpression( operation, operand1, operand2 );

        AstNode::Children assignChildren{ lhsNode, expressionNode };
        AstNode::Ptr assignNode = std::make_shared< AstNode >( T::ASSIGN, assignChildren );
        return assignNode;
    }
    AstNode::Ptr CreateTwoOpExpression( T operation, uint8_t operand1, uint8_t operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, uint8_t operand1, const std::string& operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, const std::string& operand1, uint8_t operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, const std::string& operand1, const std::string& operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, uint8_t operand1, AstNode::Ptr operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, AstNode::Ptr operand1, uint8_t operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, const std::string& operand1, AstNode::Ptr operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, AstNode::Ptr operand1, const std::string& operand2 );
    AstNode::Ptr CreateTwoOpExpression( T operation, AstNode::Ptr operand1, AstNode::Ptr operand2 );

    AstNode::Ptr WrapNodesInBlocks( AstNode::Children nodes );
    void CreateAndAttachFakeSymbolTable( AstNode::Ptr scopeNode,
                                         std::vector< std::string > identifiers,
                                         SymbolTable::Ptr parentTable = nullptr );
}