/**
 * Contains declaration of struct representing a three-address-code instruction.
 */

#pragma once

#include <string>
#include <variant>
#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "Grammar.h"

namespace TAC
{

    // Data type to represent a literal numeric value - in this case a byte.
    using Literal = uint8_t;

    // Opcodes of the intermediate representation - this is similar to the target assembly language, but removes the
    // concept of load-store, as this is a level higher.
    enum Opcode
    {
        INVALID,
        ADD,
        SUB,
        AND,  // Bitwise and
        OR,   // Bitwise or
        LS,   // Left shift
        RS,   // Right shift
        BRE,  // Branch if equal
        BRLT, // Branch if less than
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

    // Operand can either be a string identifier (empty string to represent no value), or a numeric value.
    using Operand = std::variant< std::string, Literal >;

    /**
     * \brief  Represents a RHS of an instruction in the case that an operation is being performed. This consists of
     *         an opcode and 2 identifier strings (can be left empty to represent no value).
     */
    struct Operation
    {
        using Ptr = std::shared_ptr< Operation >;

        Operation( Opcode operationCode, std::string op1, std::string op2 )
        : opcode( operationCode ), operand1( op1 ), operand2( op2 )
        {
        }

        Opcode opcode;
        std::string operand1;
        std::string operand2;
    };

    // The right hand side of an instruction can either be a single operand (identifier or literal), or an opcode with
    // two operands (i.e., an operation that is being performed).
    using RHS = std::variant< Operand, Operation::Ptr >;

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
            std::string target,
            Opcode opcode,
            std::string operand1,
            std::string operand2,
            std::string label = "" // Only used if instruction has label attached
        )
        : m_target( target ),
          m_label( label )
        {
            m_rhs = std::make_shared< Operation >( opcode, operand1, operand2 );
        }

        // Overloaded constructor for assignment instructions with a single RHS value.
        ThreeAddrInstruction(
            std::string target,
            Operand value,
            std::string label = "" // Only used if instruction has label attached
        )
        : m_target( target ),
          m_label( label )
        {
            m_rhs = value;
        }

        bool IsOperation()
        {
            return std::holds_alternative< Operation::Ptr >( m_rhs );
        }
        Operation::Ptr GetOperation()
        {
            if ( !IsOperation() )
            {
                throw std::invalid_argument( "Can only be called on operation type." );
            }
            return std::get< Operation::Ptr >( m_rhs );
        }

        /**
         * \brief  Determines if operand contains empty string or not.
         *
         * \param[in]  operand  The operand being checked.
         *
         * \return  True if the operand is holding an empty string, false otherwise.
         */
        static bool IsOperandEmpty( Operand operand )
        {
            if ( std::holds_alternative< std::string >( operand ) )
            {
                if ( "" == std::get< std::string >( operand ) )
                {
                    return true;
                }
            }
            return false;
        }
        /**
         * \brief  Determines if opcode is a branch type.
         *
         * \param[in]  opcode  The opcode being checked.
         *
         * \return  True if the opcode is a branch type, false otherwise.
         */
        static bool IsOpcodeBranch( Opcode opcode )
        {
            return Opcode::BRE == opcode || Opcode::BRLT == opcode;
        }

        // The target of the operation, i.e. where the result will be stored.
        std::string m_target;

        // The right hand side of the instruction - either a single operand or an operation.
        RHS m_rhs;

        // Optional label assigned to this instruction.
        std::string m_label;
    };

} // namespace TAC