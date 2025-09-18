/**
 * Contains declaration of interface class responsible for generating three-address code for complex operations.
 */

#pragma once

#include "ThreeAddrInstruction.h"
#include "AstNode.h"
#include "TacInstructionFactory.h"

using namespace TAC;

/**
 * \brief  Pure virtual interface class to allow mocking. See \ref TacExpressionGenerator.
 */
class ITacExpressionGenerator
{
public:
    using Ptr = std::shared_ptr< ITacExpressionGenerator >;

    virtual Operand Multiply( Operand op1, Operand op2 ) = 0;
    virtual Operand Divide( Operand op1, Operand op2 ) = 0;
    virtual Operand Modulo( Operand op1, Operand op2 ) = 0;

    virtual Operand Equals( Operand op1, Operand op2 ) = 0;
    virtual Operand NotEquals( Operand op1, Operand op2 ) = 0;
    virtual Operand Leq( Operand op1, Operand op2 ) = 0;
    virtual Operand Geq( Operand op1, Operand op2 ) = 0;
    virtual Operand LessThan( Operand op1, Operand op2 ) = 0;
    virtual Operand GreaterThan( Operand op1, Operand op2 ) = 0;

    virtual Operand LogicalNot( Operand op1 ) = 0;
    virtual Operand LogicalOr( Operand op1, Operand op2 ) = 0;
    virtual Operand LogicalAnd( Operand op1, Operand op2 ) = 0;
};