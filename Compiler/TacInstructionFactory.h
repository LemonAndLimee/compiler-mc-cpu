/**
 * Contains declaration of factory class for Three-Address-Code instructions.
 */

#pragma once

#include "ThreeAddrInstruction.h"
#include "Logger.h"

using namespace TAC;

class TacInstructionFactory
{
public:
    using Ptr = std::shared_ptr< TacInstructionFactory >;
    using Instructions = std::vector< ThreeAddrInstruction::Ptr >;

    TacInstructionFactory();

    virtual std::string GetNewTempVar( std::string hrfName = "temp" );
    virtual std::string GetNewLabel( std::string hrfName = "label" );

    virtual void SetNextInstructionLabel( const std::string& label );

    virtual void AddInstruction( std::string target, Opcode opcode, Operand operand1, Operand operand2 );
    virtual void AddSingleOperandInstruction( std::string target, Opcode opcode, Operand operand );
    virtual void AddAssignmentInstruction( std::string target, Operand operand );

    virtual void SetInstructionBranchToNextLabel( ThreeAddrInstruction::Ptr instruction, std::string labelIfNotExists );

    virtual ThreeAddrInstruction::Ptr GetLatestInstruction();
    virtual Instructions GetInstructions();

    // There is no chance of this accidentally being used by a real value, as all vars/labels have numbers in their
    // UUIDs.
    static inline const std::string PLACEHOLDER = "PLACEHOLDER";

protected:
    // Storage of created instructions.
    Instructions m_instructions;

    // Counter of the number of temporary variables currently in use.
    size_t m_tempVarsInUse;
    // Counter of the number of branch labels currently in use.
    size_t m_labelsInUse;
    // If non-empty, stores the value of the label to be attached to the next created instruction.
    std::string m_nextInstrLabel;
};