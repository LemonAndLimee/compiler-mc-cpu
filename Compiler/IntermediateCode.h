/**
 * Contains declaration of class responsible for converting an abstract syntax tree into three-address code.
 */

#pragma once

#include "TacExpressionGenerator.h"

#include <tuple>

using namespace TAC;

/**
 * \brief  Class responsible for traversing an abstract syntax tree and converting it into a list of three-address code
 *         instructions. This is a higher level organisation class, and delegates any complex cases that need new
 *         instructions generating to its TAC expression generator member.
 */
class IntermediateCode
{
public:
    using UPtr = std::unique_ptr< IntermediateCode >;
    using Ptr = std::shared_ptr< IntermediateCode >;

    IntermediateCode( TacInstructionFactory::Ptr instrFactory, ITacExpressionGenerator::Ptr tacExprGenerator );

    void GenerateIntermediateCode( AstNode::Ptr astNode );

private:
    void ConvertAstToInstructions( AstNode::Ptr astNode, SymbolTable::Ptr currentSt );

    void ConvertAssign( AstNode::Ptr astNode, SymbolTable::Ptr currentSt );
    std::string GetIdentifierFromLhsNode( AstNode::Ptr lhsNode );

    void ConvertIfElse( AstNode::Ptr astNode, SymbolTable::Ptr currentSt );
    void ConvertForLoop( AstNode::Ptr astNode, SymbolTable::Ptr currentSt );
    void ConvertWhileLoop( AstNode::Ptr astNode, SymbolTable::Ptr currentSt );

    std::string CalculateUniqueIdentifier( const std::string& currentIdentifier, SymbolTable::Ptr symbolTable );

    using ExpressionInfo = std::tuple< Opcode, Operand, Operand >;
    ExpressionInfo GetExpressionInfo( AstNode::Ptr expressionNode, SymbolTable::Ptr currentSt );
    Operand GetOperandFromExpressionInfo( ExpressionInfo info );

    // Factory class for creating instructions.
    TacInstructionFactory::Ptr m_instructionFactory;
    // Object responsible for converting complex expression operations and creating new instructions.
    ITacExpressionGenerator::Ptr m_tacExpressionGenerator;
};