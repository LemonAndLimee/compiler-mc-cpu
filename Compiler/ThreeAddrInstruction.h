/**
 * Contains declaration of struct representing a three-address-code instruction.
 */

#pragma once

#include <string>
#include <variant>
#include <unordered_map>
#include <memory>

#include "Grammar.h"

namespace TAC
{

    // Data type to represent a literal numeric value - in this case a byte.
    using Literal = uint8_t;

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

    // Map of grammar terminal symbols to their corresponding opcodes (if they map directly).
    const std::unordered_map< GrammarSymbols::Symbol, Opcode > g_symbolsToOpcodesMap{
        { GrammarSymbols::T::PLUS, Opcode::ADD },
        { GrammarSymbols::T::MINUS, Opcode::SUB },
        { GrammarSymbols::T::BITWISE_AND, Opcode::AND },
        { GrammarSymbols::T::BITWISE_OR, Opcode::OR },
        { GrammarSymbols::T::LSHIFT, Opcode::LS },
        { GrammarSymbols::T::RSHIFT, Opcode::RS }
    };

    // Operand can either be a string identifier/label, or a numeric value (or can be empty i.e. monostate)
    using Operand = std::variant< std::monostate, std::string, Literal >;

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

    using Instructions = std::vector< ThreeAddrInstruction::Ptr >;

} // namespace TAC