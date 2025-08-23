/**
 * Contains declaration of struct representing a three-address-code instruction.
 */

#pragma once

#include <string>
#include <variant>

namespace TAC
{

// Opcodes of the intermediate representation - this is similar to the target assembly language, but removes the
// concept of load-store, as this is a level higher.
enum Opcode
{
    UNUSED,
    ADD,
    SUB,
    AND,  // Bitwise and
    OR,   // Bitwise or
    LS,   // Left shift
    RS,   // Right shift
    BRU,  // Unconditional branch
    BRE,  // Branch if equal
    BRLT, // Branch if less than
    BRGT, // Branch if greater than
    BRZ   // Branch if zero
};

// Operand can either be a string identifier/label, or a numeric value (or can be empty i.e. monostate)
using Operand = std::variant< std::monostate, std::string, uint8_t >;

/**
 * \brief  Represents an instruction in three-address code. Stores the result of an operation, the operation type,
 *         and up to two operands. The target and opcode may be unused in the case of branches and assignment
 *         respectively. For this intermediate representation, identifier strings are still used, and registers/
 *         memory are not considered.
 */
struct ThreeAddrInstruction
{
    using Ptr = std::shared_ptr< ThreeAddrInstruction >;

    ThreeAddrInstruction(
        std::string resultId,
        Opcode opcode,
        Operand op1,
        Operand op2,
        std::string label = "" // Only used if instruction has label attached
    )
    : m_result( resultId ),
      m_operation( opcode ),
      m_operand1( op1 ),
      m_operand2( op2 ),
      m_label( label )
    {
    }

    // The target of the operation, i.e. where the result will be stored.
    std::string m_result;

    // Operation type.
    Opcode m_operation;

    Operand m_operand1;
    Operand m_operand2;

    // Optional label assigned to this instruction.
    std::string m_label;
};

}