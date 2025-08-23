/**
 * Contains declaration of class responsible for generating intermediate representation of code, in three-address-
 * instruction form.
 */

#pragma once

#include "AstNode.h"
#include "ThreeAddrInstruction.h"

#include <tuple>

/**
 * \brief  Class responsible for generating an intermediate representation of code. Takes an abstract syntax tree
 *         (which contains symbol tables for its scopes) and converts it into a list of TAC instructions.
 */
class IntermediateCode
{
public:
    using UPtr = std::unique_ptr< IntermediateCode >;
    using Instructions = std::vector< TAC::ThreeAddrInstruction::Ptr >;

    IntermediateCode() = default;

    Instructions GenerateIntermediateCode( AstNode::Ptr astNode );

private:
    void ConvertAstToInstructions( AstNode::Ptr astNode, Instructions& instructions, SymbolTable::Ptr currentSt );

    void ConvertAssign( AstNode::Ptr astNode, Instructions& instructions, SymbolTable::Ptr currentSt );
    std::string GetIdentifierFromLhsNode( AstNode::Ptr lhsNode );

    using ExpressionInfo = std::tuple< TAC::Opcode, TAC::Operand, TAC::Operand >;
    ExpressionInfo ResolveExpression( AstNode::Ptr expressionNode, Instructions& preInstructions );

    void ConvertIfElse( AstNode::Ptr astNode, Instructions& instructions );
    void ConvertForLoop( AstNode::Ptr astNode, Instructions& instructions );
    void ConvertWhileLoop( AstNode::Ptr astNode, Instructions& instructions );

    // Storage of any replaced const identifiers that were holding a byte value.
    std::unordered_map< std::string, uint8_t > m_replacedIdentifiers;
};