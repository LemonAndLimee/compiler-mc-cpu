/**
 * Contains declaration of class responsible for generating three-address code for complex operations.
 */

#pragma once

#include "ITacExpressionGenerator.h"

/**
 * \brief  Class responsible for translating more complicated operations into sets of three-address code instructions.
 *         E.g. tackles unsupported opcodes such as multiply/divide, as well as handling branching.
 */
class TacExpressionGenerator : public ITacExpressionGenerator
{
public:
    using Ptr = std::shared_ptr< TacExpressionGenerator >;

    TacExpressionGenerator( TacInstructionFactory::Ptr instrFactory );

    virtual Operand Multiply( Operand op1, Operand op2 ) override;
    virtual Operand Divide( Operand op1, Operand op2 ) override;
    virtual Operand Modulo( Operand op1, Operand op2 ) override;

    virtual Operand Equals( Operand op1, Operand op2 ) override;
    virtual Operand NotEquals( Operand op1, Operand op2 ) override;
    virtual Operand Leq( Operand op1, Operand op2 ) override;
    virtual Operand Geq( Operand op1, Operand op2 ) override;
    virtual Operand LessThan( Operand op1, Operand op2 ) override;
    virtual Operand GreaterThan( Operand op1, Operand op2 ) override;

    virtual Operand LogicalNot( Operand op1 ) override;
    virtual Operand LogicalOr( Operand op1, Operand op2 ) override;
    virtual Operand LogicalAnd( Operand op1, Operand op2 ) override;

private:
    Operand AddComparisonInstructions( const std::string& resultName,
                                       Opcode branchType,
                                       Operand branchOperand1,
                                       Operand branchOperand2,
                                       Literal valueIfBranchTrue );

    // Whether a desired result is the division result or the modulo, as they share the same set of instructions.
    enum DivMod{ DIV, MOD };
    Operand AddDivModInstructions( Operand op1,
                                   Operand op2,
                                   DivMod returnType );

    TacInstructionFactory::Ptr m_instructionFactory;
};