/**
 * Contains declaration of class responsible for generating three-address code for complex operations.
 */

#pragma once

#include "ThreeAddrInstruction.h"
#include "AstNode.h"

using namespace TAC;

/**
 * \brief  Class responsible for translating more complicated operations into sets of three-address code instructions.
 *         E.g. tackles unsupported opcodes such as multiply/divide, as well as handling branching.
 */
class TacGenerator
{
public:
    using Ptr = std::shared_ptr< TacGenerator >;

    TacGenerator();

    std::string GetNewTempVar( std::string hrfName = "temp" );
    std::string GetNewLabel( std::string hrfName = "label" );
    void SetNextLabel( const std::string& label );

    Operand Multiply( Operand op1, Operand op2, Instructions& instructions );
    Operand Divide( Operand op1, Operand op2, Instructions& instructions );
    Operand Modulo( Operand op1, Operand op2, Instructions& instructions );

    Operand Equals( Operand op1, Operand op2, Instructions& instructions );
    Operand NotEquals( Operand op1, Operand op2, Instructions& instructions );
    Operand Leq( Operand op1, Operand op2, Instructions& instructions );
    Operand Geq( Operand op1, Operand op2, Instructions& instructions );
    Operand LessThan( Operand op1, Operand op2, Instructions& instructions );
    Operand GreaterThan( Operand op1, Operand op2, Instructions& instructions );

private:
    Operand AddComparisonInstructions( Instructions& instructions,
                                       const std::string& resultName,
                                       Opcode branchType,
                                       Operand branchOperand1,
                                       Operand branchOperand2,
                                       Literal valueIfBranchTrue );

    // Whether a desired result is the division result or the modulo, as they share the same set of instructions.
    enum DivMod{ DIV, MOD };
    Operand AddDivModInstructions( Operand op1,
                                   Operand op2,
                                   Instructions& instructions,
                                   DivMod returnType );

    // Counter of the number of temporary variables currently in use.
    size_t m_tempVarsInUse;

    // Counter of the number of branch labels currently in use.
    size_t m_labelsInUse;

    // Customisable next label to return.
    std::string m_nextLabel;
};