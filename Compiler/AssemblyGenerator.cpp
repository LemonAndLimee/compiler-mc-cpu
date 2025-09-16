/**
 * Contains definition of class responsible for converting TAC into assembly code.
 */

#include "AssemblyGenerator.h"

AssemblyGenerator::AssemblyGenerator(
    const AssemblyGenerator::TacInstructions& tacInstructions
)
: m_tacInstructions( tacInstructions )
{
}

/**
 * \brief  Calculates the boundaries of basic blocks in the TAC program, populating m_basicBlockStarts.
 */
void
AssemblyGenerator::CalculateBasicBlocks()
{
    // Initialise with 0, the start index.
    m_basicBlockStarts = { 0u };

    // Start from 0 because we are considering the next index to be the start of a new block - so there is a possibility
    // that the 0th instruction is the end of a block, thus making 1 the start of the next. End at one less than the
    // last index, because if the end of a block is the last index, there is no next block.
    for ( size_t index = 0; index < m_tacInstructions.size() - 1; ++index )
    {
        TAC::ThreeAddrInstruction::Ptr instr = m_tacInstructions[index];

        bool isBlockBoundary{ false };
        // A basic block boundary occurs if there is a branch instruction, or an instruction with a label.
        if ( "" != instr->m_label )
        {
            isBlockBoundary = true;
        }
        else if ( instr->IsOperation() )
        {
            TAC::Operation::Ptr operation = instr->GetOperation();
            if ( TAC::ThreeAddrInstruction::IsOpcodeBranch( operation->opcode ) )
            {
                isBlockBoundary = true;
            }
        }

        if ( isBlockBoundary )
        {
            // Store the next instruction index, as it is the start of the next block.
            m_basicBlockStarts.push_back( index + 1 );
        }
    }
}

/**
 * \brief  Calculates the live intervals of all variables in the TAC program.
 */
void
AssemblyGenerator::CalculateLiveIntervals()
{
    for ( size_t index = 0; index < m_tacInstructions.size(); ++index )
    {
        TAC::ThreeAddrInstruction::Ptr instr = m_tacInstructions[index];

        // If instruction is storing an operation, consider the live interval of the target (as long as it is not a
        // branch instruction), and any non-empty operands.
        if ( instr->IsOperation() )
        {
            TAC::Operation::Ptr operation = instr->GetOperation();
            if ( !TAC::ThreeAddrInstruction::IsOpcodeBranch( operation->opcode ) )
            {
                RecordVarUse( instr->m_target, index );
            }
            RecordVarUse( operation->operand1, index );
            RecordVarUse( operation->operand2, index );
        }
        else
        {
            RecordVarUse( instr->m_target, index );
            TAC::Operand rhsOperand = std::get< TAC::Operand >( instr->m_rhs );
            if ( std::holds_alternative< std::string >( rhsOperand ) )
            {
                RecordVarUse( std::get< std::string >( rhsOperand ), index );
            }
        }
    }
}

/**
 * \brief  Records an instance of a variable being used, in the live interval records. If this is a new variable,
 *         creates a new live intervals entry. Otherwise, extends the end index to include this instance.
 *
 * \param[in]  identifier  The identifier of the variable that was referenced.
 * \param[in]  indexOfUse  The instruction index where the variable was referenced.
 */
void
AssemblyGenerator::RecordVarUse(
    const std::string& identifier,
    size_t indexOfUse
)
{
    // Ignore if the variable is an empty string, as it refers to an operand not being used/holding zero.
    if ( "" == identifier )
    {
        return;
    }

    // If an entry for this variable doesn't exist, make one.
    if ( 0u == m_liveIntervals.count( identifier ) )
    {
        m_liveIntervals.insert( { identifier, { indexOfUse, indexOfUse } } );
    }
    else
    {
        m_liveIntervals[identifier].second = indexOfUse;
    }
}