/**
 * Contains declaration of class responsible for converting an abstract syntax tree into three-address code.
 */

#pragma once

#include "AstNode.h"
#include "ThreeAddrInstruction.h"
#include "TacGenerator.h"

#include <tuple>

/**
 * \brief  Class responsible for traversing an abstract syntax tree and converting it into a list of three-address code
 *         instructions. This is a higher level organisation class, and delegates any complex cases that need new
 *         instructions generating to its TAC generator member.
 */
class IntermediateCode
{
public:
    using UPtr = std::unique_ptr< IntermediateCode >;
    using Instructions = std::vector< TAC::ThreeAddrInstruction::Ptr >;

    IntermediateCode( TacGenerator::Ptr tacGenerator );

    Instructions GenerateIntermediateCode( AstNode::Ptr astNode );

private:
    void ConvertAstToInstructions( AstNode::Ptr astNode, Instructions& instructions, SymbolTable::Ptr currentSt );

    void ConvertAssign( AstNode::Ptr astNode, Instructions& instructions, SymbolTable::Ptr currentSt );
    std::string GetIdentifierFromLhsNode( AstNode::Ptr lhsNode );

    void ConvertIfElse( AstNode::Ptr astNode, Instructions& instructions );
    void ConvertForLoop( AstNode::Ptr astNode, Instructions& instructions );
    void ConvertWhileLoop( AstNode::Ptr astNode, Instructions& instructions );

    std::string CalculateUniqueIdentifier( const std::string& currentIdentifier, SymbolTable::Ptr symbolTable );

    using ExpressionInfo = std::tuple< TAC::Opcode, TAC::Operand, TAC::Operand >;
    ExpressionInfo GetExpressionInfo( AstNode::Ptr expressionNode,
                                      SymbolTable::Ptr currentSt,
                                      Instructions& preInstructions );
    TAC::Operand GetOperandFromExpressionInfo( ExpressionInfo info, Instructions& instructions );

    std::string GetNewTempVar();

    // Object responsible for converting complex operations and creating new instructions.
    TacGenerator::Ptr m_tacGenerator;

    // Counter of the number of temporary variables currently in use.
    size_t m_tempVarsInUse;
};