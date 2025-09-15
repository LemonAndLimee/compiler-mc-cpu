/**
 * Contains definition of factory class for Three-Address-Code instructions.
 */

#include "TacInstructionFactory.h"

TacInstructionFactory::TacInstructionFactory()
: m_tempVarsInUse( 0u ),
  m_labelsInUse( 0u ),
  m_nextInstrLabel( "" )
{
}

/**
 * \brief  Gets identifier representing the next temporary variable available to use. Uses a counter to keep track
 *         of how many are currently in use.
 *
 * \param[in]  hrfName  Optional identifier name to use in combination with the counter, to allow easier debugging.
 *
 * \return  String identifier of the next available temporary variable.
 */
std::string
TacInstructionFactory::GetNewTempVar(
    std::string hrfName //= "temp"
)
{
    // Use a naming convention that isn't allowed by the grammar, to avoid naming clashes. This doesn't matter
    // at this point in compilation as any string is a valid representation.
    std::string id = std::to_string( m_tempVarsInUse ) + hrfName;
    ++m_tempVarsInUse;
    return id;
}

/**
 * \brief  Gets a new unique branch label. Uses a counter to keep track of the next available number.
 *
 * \param[in]  hrfName  Optional label name to use in combination with the counter, to allow easier debugging.
 *
 * \return  Unique label in string form.
 */
std::string
TacInstructionFactory::GetNewLabel(
    std::string hrfName //= "label"
)
{
    std::string label = std::to_string( m_labelsInUse ) + hrfName;
    ++m_labelsInUse;
    return label;
}

/**
 * \brief  Sets the next instruction label. This should only be called if one has not already been set.
 *
 * \param[in]  label  The next instruction label.
 */
void
TacInstructionFactory::SetNextInstructionLabel(
    const std::string& label
)
{
    if ( "" != m_nextInstrLabel )
    {
        LOG_ERROR_AND_THROW( "Trying to set next instruction label '" + label + "' but it is already set '"
                             + m_nextInstrLabel + "'.", std::runtime_error );
    }
    m_nextInstrLabel = label;
}

/**
 * \brief  Creates a new instruction and adds it to the stored collection.
 *
 * \param[in]  target    String representing the identifier of the target of the operation. This could be the result
 *                       of a calculation, or a target branch label.
 * \param[in]  opcode    The opcode of the operation, determining what type of instruction it is.
 * \param[in]  operand1  The first operand in the instruction. Can be identifier or literal.
 * \param[in]  operand2  The second operand in the instruction. Can be identifier or literal.
 */
void
TacInstructionFactory::AddInstruction(
    std::string target,
    Opcode opcode,
    Operand operand1,
    Operand operand2
)
{
    ThreeAddrInstruction::Ptr instr = std::make_shared< ThreeAddrInstruction >( target, opcode, operand1, operand2, m_nextInstrLabel );
    m_instructions.push_back( instr );

    if ( "" != m_nextInstrLabel )
    {
        m_nextInstrLabel = "";
    }
}

/**
 * \brief  Creates a new single-operand instruction and adds it to the stored collection.
 *
 * \param[in]  target   String representing the identifier of the target of the operation. This could be the result
 *                      of a calculation, or a target branch label.
 * \param[in]  opcode   The opcode of the operation, determining what type of instruction it is.
 * \param[in]  operand  The operand in the instruction. Can be identifier or literal.
 */
void
TacInstructionFactory::AddSingleOperandInstruction(
    std::string target,
    Opcode opcode,
    Operand operand
)
{
    Operand emptyOperand{};
    AddInstruction( target, opcode, operand, emptyOperand );
}

/**
 * \brief  Creates a new instruction with no operands and adds it to the stored collection.
 *
 * \param[in]  target   String representing the identifier of the target of the operation. This could be the result
 *                      of a calculation, or a target branch label.
 * \param[in]  opcode   The opcode of the operation, determining what type of instruction it is.
 */
void
TacInstructionFactory::AddNoOperandsInstruction(
    std::string target,
    Opcode opcode
)
{
    Operand emptyOperand{};
    AddInstruction( target, opcode, emptyOperand, emptyOperand );
}

/**
 * \brief  Creates a new instruction with no operands and adds it to the stored collection.
 *
 * \param[in]  target   String representing the identifier of the target of the operation. This could be the result
 *                      of a calculation, or a target branch label.
 * \param[in]  operand  The operand in the instruction. Can be identifier or literal.
 */
void
TacInstructionFactory::AddAssignmentInstruction(
    std::string target,
    Operand operand
)
{
    Opcode emptyOpcode{ UNUSED };
    Operand emptyOperand{};
    AddInstruction( target, emptyOpcode, operand, emptyOperand );
}

/**
 * \brief  Replaces the target of a given branch instruction so that it points to the current 'next instruction'. If
 *         there is no configured next label, it creates one and assigns this.
 *
 * \param[in]  instruction       The branch instruction to redirect.
 * \param[in]  labelIfNotExists  If there isn't a next label, this is used as the HRF prompt for creating a new one.
 */
void
TacInstructionFactory::SetInstructionBranchToNextLabel(
    ThreeAddrInstruction::Ptr instruction,
    std::string labelIfNotExists
)
{
    if ( nullptr == instruction )
    {
        LOG_ERROR_AND_THROW( "Passed a nullptr instruction.", std::invalid_argument );
    }
    switch ( instruction->m_opcode )
    {
    case BRU:
    case BRZ:
    case BRE:
    case BRLT:
    case BRGT:
        break;
    default:
        LOG_ERROR_AND_THROW( "This method can only be called on a branch instruction. Opcode: "
                             + std::to_string( instruction->m_opcode ), std::invalid_argument );
    }

    if ( "" == m_nextInstrLabel )
    {
        std::string endLabel = GetNewLabel( labelIfNotExists );
        m_nextInstrLabel = endLabel;
    }

    instruction->m_target = m_nextInstrLabel;
}

/**
 * \brief  Returns the most recently added instruction pointer.
 *
 * \return  The latest instruction pointer.
 */
ThreeAddrInstruction::Ptr
TacInstructionFactory::GetLatestInstruction()
{
    if ( m_instructions.empty() )
    {
        LOG_ERROR_AND_THROW( "Trying to access latest instruction from empty collection.", std::runtime_error );
    }
    return m_instructions.back();
}

/**
 * \brief  Returns the stored collection of instructions. If there is still a label waiting to be added, create
 *         a dummy filler instruction to hold the label.
 *
 * \return  The stored instructions.
 */
TacInstructionFactory::Instructions
TacInstructionFactory::GetInstructions()
{
    // If the 'next instruction label' is set, add a filler instruction with this label so that previous instructions
    // have this label to branch to.
    if ( "" != m_nextInstrLabel )
    {
        std::string tempVar = GetNewTempVar();
        AddAssignmentInstruction( tempVar, 0u );
    }
    return m_instructions;
}