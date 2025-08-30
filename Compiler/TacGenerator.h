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

    TacGenerator() = default;

    std::string GetNewTempVar();

    Operand Multiply( Operand op1, Operand op2, Instructions& instructions );

private:
    // Counter of the number of temporary variables currently in use.
    size_t m_tempVarsInUse;
};