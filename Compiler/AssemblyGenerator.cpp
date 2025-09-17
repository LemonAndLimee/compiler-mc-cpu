/**
 * Contains definition of class responsible for converting TAC into assembly code.
 */

#include <set>
#include <numeric>

#include "AssemblyGenerator.h"
#include "Logger.h"

using namespace Assembly;

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
        InstrStringArgs relevantVars = GetVarsFromInstruction( instr );
        std::string target = std::get< 0 >( relevantVars );
        std::string operand1 = std::get< 1 >( relevantVars );
        std::string operand2 = std::get< 2 >( relevantVars );

        RecordVarUse( target, index );
        RecordVarUse( operand1, index );
        RecordVarUse( operand2, index );
    }
}

/**
 * \brief  Where the value is relevant, extracts the target and both operands in string form from a TAC instruction.
 *         If the value isn't relevant for this instruction (e.g. a branch target, or an unused operand), an empty
 *         string is returned in its place.
 *
 * \param[in]  instruction  The TAC instruction containing identifiers.
 *
 * \return  Tuple containing strings of the target and both operands.
 */
AssemblyGenerator::InstrStringArgs
AssemblyGenerator::GetVarsFromInstruction(
    TAC::ThreeAddrInstruction::Ptr instruction
)
{
    InstrStringArgs identifiers = std::make_tuple( "", "", "" );

    if ( instruction->IsOperation() )
    {
        TAC::Operation::Ptr operation = instruction->GetOperation();
        if ( !TAC::ThreeAddrInstruction::IsOpcodeBranch( operation->opcode ) )
        {
            std::get< 0 >( identifiers ) = instruction->m_target;
        }
        std::get< 1 >( identifiers ) = operation->operand1;
        std::get< 2 >( identifiers ) = operation->operand2;
    }
    else
    {
        std::get< 0 >( identifiers ) = instruction->m_target;

        TAC::Operand rhsOperand = std::get< TAC::Operand >( instruction->m_rhs );
        if ( std::holds_alternative< std::string >( rhsOperand ) )
        {
            std::string rhsId = std::get< std::string >( rhsOperand );
            std::get< 1 >( identifiers ) = rhsId;
        }
    }

    return identifiers;
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

/**
 * \brief  Converts the stored TAC instructions into assembly instructions.
 *
 * \return  Collection of assembly instructions.
 */
Instructions
AssemblyGenerator::GenerateAssemblyInstructions()
{
    if ( !m_assemblyInstructions.empty() )
    {
        LOG_WARN( "This object already has stored assembly instructions - these will be wiped." );
    }
    m_assemblyInstructions.clear();
    // The number of assembly instructions will be >= the number of TAC instructions, so we can reserve this much in
    // advance.
    m_assemblyInstructions.reserve( m_tacInstructions.size() );

    size_t numBlocks = m_basicBlockStarts.size();
    for ( size_t index = 0; index < numBlocks; ++index )
    {
        size_t blockStart = m_basicBlockStarts[index];
        size_t nextBlockStart = index < numBlocks - 1 ? m_basicBlockStarts[index + 1] : numBlocks;
        GenerateAssemblyForBasicBlock( blockStart, nextBlockStart );
    }
    return m_assemblyInstructions;
}

/**
 * \brief  Converts TAC instructions from a specific basic block into assembly instructions, adding load and store
 *         instructions for any spilled variables (as well as documenting their memory locations).
 *
 * \param[in]   blockStart              The index of the instruction at the start of this block (inclusive).
 * \param[in]   blockEnd                The index of the end of the block (exclusive).
 */
void
AssemblyGenerator::GenerateAssemblyForBasicBlock(
    size_t blockStart,
    size_t blockEnd
)
{
    // Reset the active vars and available registers, as they are specific to this block.
    m_currentActiveVars.clear();
    m_availableRegs.clear();
    for ( uint8_t i = 0; i < NUM_AVAILABLE_REGS; ++i )
    {
        m_availableRegs.insert( i + AVAILABLE_REG_OFFSET );
    }

    for ( size_t instrIndex = blockStart; instrIndex < blockEnd; ++instrIndex )
    {
        ExpireOldIntervals( instrIndex );

        TAC::ThreeAddrInstruction::Ptr instr = m_tacInstructions[instrIndex];
        GenerateAssemblyForInstr( instr );
    }

    // Save any currently active vars that were edited.
    for ( auto it = m_currentActiveVars.begin(); it != m_currentActiveVars.end(); ++it )
    {
        ActiveVarInfo varInfo = it->second;
        bool isEdited = varInfo.second;
        if ( isEdited )
        {
            SaveActiveVar( it->first );
        }
    }
}

/**
 * \brief  Saves variable to memory, using its allocated memory address (allocating a new one if it doesn't have one
 *         yet).
 *
 * \param[in]  identifier  The variable identifier.
 */
void
AssemblyGenerator::SaveActiveVar(
    const std::string& identifier
)
{
    if ( m_currentActiveVars.end() == m_currentActiveVars.find( identifier ) )
    {
        LOG_ERROR_AND_THROW( "'" + identifier + "' not found in active variables.", std::invalid_argument );
    }
    ActiveVarInfo varInfo = m_currentActiveVars[identifier];
    uint8_t varRegister = varInfo.first;

    uint8_t memAddr;
    // If the variable already has an allocated memory location, use it.
    if ( m_memoryLocations.end() != m_memoryLocations.find( identifier ) )
    {
        memAddr = m_memoryLocations[identifier];
    }
    else
    {
        memAddr = GetNextMemoryLocation();
        m_memoryLocations.insert( { identifier, memAddr } );
    }
    SaveRegister( varRegister, memAddr );
}

/**
 * \brief  Adds store instruction to save the given register into the given memory address. Adds an LDI to get the
 *         memory address into a temporary register.
 *
 * \param[in]  registerToSave  The register containing the value to store.
 * \param[in]  memoryAddress   The memory address to save to.
 */
void
AssemblyGenerator::SaveRegister(
    uint8_t registerToSave,
    uint8_t memoryAddress
)
{
    // Load the desired memory address into a temporary register. No label since these are instructions being
    // added at the end of a block, intended to stay within the block.
    std::string label = "";

    uint8_t memAddrRegister = MEM_ADDR_TEMP_REG;
    AddLoadImmediate( label, memAddrRegister, memoryAddress );
    AddStoreInstruction( registerToSave, memAddrRegister );
}

/**
 * \brief  Splits an 8-bit immediate value into two 4-bit operands, as this is the operand length in the target
 *         architecture.
 *
 * \param[in]  immediateValue  The immediate value being split.
 */
std::pair< uint8_t, uint8_t >
AssemblyGenerator::SplitImmediateOperand(
    uint8_t immediateValue
)
{
    uint8_t mostSigBits = ( 0xF0 & immediateValue ) >> 4u;
    uint8_t leastSigBits = 0x0F & immediateValue;
    return std::make_pair( mostSigBits, leastSigBits );
}

/**
 * \brief  Adds an LDI instruction, loading an immediate value into the desired register.
 *
 * \param[in]  label           The instruction branch label, or an empty string for none.
 * \param[in]  targetRegister  The register to load the value into.
 * \param[in]  immediateValue  The value to load into the register.
 */
void
AssemblyGenerator::AddLoadImmediate(
    const std::string& label,
    uint8_t targetRegister,
    uint8_t immediateValue
)
{
    // Split up the immediate value over 2 operands as each operand is 4 bits.
    std::pair< uint8_t, uint8_t > operands = SplitImmediateOperand( immediateValue );

    Instruction ldiInstr = std::make_tuple( label, Opcode::LDI, targetRegister, operands.first, operands.second );
    m_assemblyInstructions.push_back( ldiInstr );
}

/**
 * \brief  Adds a store instruction, of a register into a target memory address.
 *
 * \param[in]  registerToStore        The register storing the target memory address.
 * \param[in]  registerHoldingTarget  The register holding the value to store in memory.
 */
void
AssemblyGenerator::AddStoreInstruction(
    uint8_t registerToStore,
    uint8_t registerHoldingTarget
)
{
    // No label, as a store instruction will only ever come after a TAC instruction, so will never be given the new
    // start of a block.
    Instruction storeInstr = std::make_tuple( "", Opcode::STR, registerToStore, registerHoldingTarget, 0u );
    m_assemblyInstructions.push_back( storeInstr );
}

/**
 * \brief  Gets the next available memory address, assuming that all memory addresses are allocated without skipping.
 *
 * \return  Next available memory address.
 */
uint8_t
AssemblyGenerator::GetNextMemoryLocation()
{
    size_t numAllocatedLocations = m_memoryLocations.size();
    return MEM_ADDR_OFFSET + numAllocatedLocations;
}

/**
 * \brief  Checks the active variables are still active, expiring any that have passed the end of their live interval.
 *
 * \param[in]  currentInstrIndex  Index of the next instruction we are looking at. Variables are expired if their
 *                                end point is < current index.
 */
void
AssemblyGenerator::ExpireOldIntervals(
    size_t currentInstrIndex
)
{
    for ( auto it = m_currentActiveVars.begin(); it != m_currentActiveVars.end(); )
    {
        std::string identifier = it->first;
        LiveInterval liveInterval = m_liveIntervals[identifier];
        if ( currentInstrIndex > liveInterval.second )
        {
            uint8_t registerToRelease = it->second.first;
            m_availableRegs.insert( registerToRelease );
            m_currentActiveVars.erase( it++ );
        }
        else
        {
            ++it;
        }
    }
}

/**
 * \brief  Generates assembly instruction(s) for a given TAC instruction.
 *
 * \param[in]  instruction  The string identifier of this variable.
 */
void
AssemblyGenerator::GenerateAssemblyForInstr(
    TAC::ThreeAddrInstruction::Ptr instruction
)
{
    // The current instruction will only have a label if it is the start of a new block - in which case we want the
    // label to be given to the next assembly instruction we add.
    std::string label = instruction->m_label;
    Opcode assemblyOpcode = GetAssemblyOpcode( instruction );
    // Initialise operands to zeros, as they represent unused values aka empty strings.
    InstructionTarget assemblyTarget{ 0u };
    uint8_t assemblyOperand1{ 0u };
    uint8_t assemblyOperand2{ 0u };


    InstrStringArgs relevantVars = GetVarsFromInstruction( instruction );

    // Step 1: resolve the target (this has slightly different behaviour because a) it could be a branch label, and
    // b) if it is spilled, it needs saving after this instruction.
    constexpr size_t targetIndex = 0u;
    std::string targetStr = std::get< targetIndex >( relevantVars );
    if ( "" == targetStr )
    {
        // If the 'relevant' target string is empty, this means it is a branch label, as it is not a var.
        assemblyTarget = instruction->m_target;
    }
    else
    {
        assemblyTarget = GetOperandRegister( targetStr, targetIndex, label );
    }

    // Step 2: resolve the operands

    // If the operation is an LDI, we need to split operand 1 (8 bits) over operand 1 and operand 2, as each
    // assembly operand is intended to be 4 bits.
    if ( Opcode::LDI == assemblyOpcode )
    {
        TAC::Literal immediateValue = std::get< TAC::Literal >( std::get< TAC::Operand >( instruction->m_rhs ) );
        std::pair< uint8_t, uint8_t > operands = SplitImmediateOperand( immediateValue );
        assemblyOperand1 = operands.first;
        assemblyOperand2 = operands.second;
    }
    else
    {
        constexpr size_t operand1Index = 1u;
        constexpr size_t operand2Index = 2u;
        assemblyOperand1 = GetOperandRegister( std::get< operand1Index >( relevantVars ), operand1Index, label );
        assemblyOperand2 = GetOperandRegister( std::get< operand2Index >( relevantVars ), operand2Index, label );
    }

    // Create and add an instruction from the gathered arguments.
    Instruction instr = std::make_tuple( label, assemblyOpcode, assemblyTarget, assemblyOperand1, assemblyOperand2 );
    m_assemblyInstructions.push_back( instr );

    // If the target is a spilled var, write it back to memory after the instruction was added.
    if ( std::holds_alternative< uint8_t >( assemblyTarget ) )
    {
        // If target is not an active var
        if ( m_currentActiveVars.end() == m_currentActiveVars.find( targetStr ) )
        {
            if ( m_memoryLocations.end() == m_memoryLocations.find( targetStr ) )
            {
                LOG_ERROR_AND_THROW( "Inactive var could not be found in memory: '" + targetStr + "'",
                                     std::runtime_error );
            }
            uint8_t memAddr = m_memoryLocations[targetStr];
            SaveRegister( std::get< uint8_t >( assemblyTarget ), memAddr );
        }
    }
}

/**
 * \brief  Determines the equivalent assembly opcode for a given TAC instruction.
 *
 * \param[in]  instruction  The TAC instruction being converted.
 *
 * \return  The equivalent assembly opcode.
 */
Opcode
AssemblyGenerator::GetAssemblyOpcode(
    TAC::ThreeAddrInstruction::Ptr instruction
)
{
    if ( instruction->IsOperation() )
    {
        TAC::Operation::Ptr operation = instruction->GetOperation();
        switch ( operation->opcode )
        {
        case TAC::Opcode::ADD:
            return ::Opcode::ADD;
        case TAC::Opcode::SUB:
            return ::Opcode::SUB;
        case TAC::Opcode::AND:
            return ::Opcode::AND;
        case TAC::Opcode::OR:
            return ::Opcode::OR;
        case TAC::Opcode::LS:
            return ::Opcode::LS;
        case TAC::Opcode::RS:
            return ::Opcode::RS;
        case TAC::Opcode::BRE:
            return ::Opcode::BRE;
        case TAC::Opcode::BRLT:
            return ::Opcode::BRLT;
        default:
            LOG_ERROR_AND_THROW( "Unknown/invalid TAC opcode: " + std::to_string( operation->opcode ),
                                 std::invalid_argument );
            break;
        }
    }
    else
    {
        TAC::Operand rhsOperand = std::get< TAC::Operand >( instruction->m_rhs );
        if ( std::holds_alternative< std::string >( rhsOperand ) )
        {
            return Opcode::LD;
        }
        else
        {
            return Opcode::LDI;
        }
    }
}

/**
 * \brief  Allocates or retrieves register or memory location corresponding to an identifier. If necessary, adds load
 *         instructions to retrieve value from memory. Returns the register that is now holding the variable.
 *
 * \param[in]      operand             The operand being requested.
 * \param[in]      operandIndex        The index of the operand within the parent instruction (0 for target, 1 and 2
 *                                     for the respective arguments). Used to determine which temporary register to use
 *                                     if the variable is spilled.
 * \param[in,out]  labelOfParentInstr  Label of the instruction the operand belongs to. If prerequisite loads are added,
 *                                     the first will inherit this label, before erasing the label value so that the
 *                                     parent instruction no longer has the label.
 */
uint8_t
AssemblyGenerator::GetOperandRegister(
    const std::string& operand,
    size_t operandIndex,
    std::string& labelOfParentInstr
)
{
    // Ignore if operand is empty/unused
    if ( "" == operand )
    {
        return 0u;
    }

    // If active, return register mapping
    if ( m_currentActiveVars.end() != m_currentActiveVars.find( operand ) )
    {
        return m_currentActiveVars[operand].first;
    }
    // If inactive and in memory, it is either spilled (if there are no more available registers) or it has been saved
    // from a previous block and needs loading in.
    // TODO: if this is a target operand, it is being written to, so we don't need to worry about loading its existing
    // value. We only need to work out and return its allocated register.
    if ( m_memoryLocations.end() != m_memoryLocations.find( operand ) )
    {
        uint8_t memAddr = m_memoryLocations[operand];
        // First load the memory address into a temporary reg
        uint8_t memAddrTempReg = MEM_ADDR_TEMP_REG;;
        AddLoadImmediate( labelOfParentInstr, memAddrTempReg, memAddr );
        // If parent instruction had a non-empty label, this has been transferred to the load instruction, so we
        // can erase it for when the parent instruction is created.
        if ( "" != labelOfParentInstr )
        {
            labelOfParentInstr = "";
        }

        uint8_t registerToLoadInto;

        if ( m_availableRegs.empty() )
        {
            // If there are no more free registers, keep it as inactive, but add a load into a temporary register.
            // The calling method will save it again after the using instruction has been added, as it is still marked
            // as inactive.
            registerToLoadInto = FIRST_VAR_TEMP_REG + operandIndex;
        }
        else
        {
            bool isLhs = operandIndex == 0;
            registerToLoadInto = AllocateRegisterAndMakeActive( operand, isLhs );
        }
        // Load into allocated register
        Instruction loadInstr = std::make_tuple( "", Opcode::LD, registerToLoadInto, memAddrTempReg, 0u );
        m_assemblyInstructions.push_back( loadInstr );
        return registerToLoadInto;
    }


    // Else if inactive and NOT in memory, this is a new variable. Either allocate a register and set as active, or
    // trigger a spill and update statuses as appropriate/add any necessary saves.

    // Expect the operand to be a target, otherwise it doesn't make sense to be a new variable.
    if ( 0u != operandIndex )
    {
        LOG_ERROR_AND_THROW( "Unexpected operand index " + std::to_string( operandIndex )
                             + " for new variable '" + operand + "'", std::runtime_error );
    }

    if ( m_availableRegs.empty() )
    {
        // If there are no available registers, spill the live interval with the latest end point (as the active list is
        // sorted, this is just the latest out of the current var, and the last element in active).
        uint8_t endPointOfOperand = m_liveIntervals[operand].second;
        uint8_t endPointOfLastActiveVar = m_currentActiveVars.rbegin()->second.second;

        if ( endPointOfOperand >= endPointOfLastActiveVar )
        {
            // To 'spill' this variable, we just need to not mark it as active, and assign it to a temporary register
            // for the sake of this access instance. We also need to assign a memory address for it.
            uint8_t allocatedMemAddr = GetNextMemoryLocation();
            m_memoryLocations[operand] = allocatedMemAddr;

            // Use the target temporary register, as this is the target operand.
            return FIRST_VAR_TEMP_REG;
        }
        else
        {
            // To spill the last active var, we need to mark it as inactive, and give its register to our current var.
            // If the active var has been written to, it needs to be saved first.
            auto lastActiveElement = m_currentActiveVars.rbegin();
            std::string activeVarId = lastActiveElement->first;
            ActiveVarInfo activeVarInfo = lastActiveElement->second;

            bool isLastActiveWrittenTo = activeVarInfo.second;
            if ( isLastActiveWrittenTo )
            {
                SaveActiveVar( activeVarId );
            }
            // We don't need to worry about allocating it a memory location, because if it hasn't been written to, then
            // it has been declared in a previous block, and thus already has a memory location.

            // Replace the active var with our operand.
            m_currentActiveVars.erase( activeVarId );
            // For our new entry, as this is a LHS operand, mark it has having been written to.
            activeVarInfo.second = true;
            m_currentActiveVars.insert( { operand, activeVarInfo } );
        }
    }
    else
    {
        // If there are available registers, allocate one. Don't need any pre-loads as this is a new variable.
        constexpr bool isLhs{ true };
        uint8_t allocatedReg = AllocateRegisterAndMakeActive( operand, isLhs );
        return allocatedReg;
    }
}

/**
 * \brief  Allocates an available register to the given variable, removing it from available, and adding a mapping
 *         to the collection of active vars. Throws if there are no available registers.
 *
 * \param[in]  identifier  The variable being allocated a register.
 * \param[in]  isLhs       Whether the variable is on the LHS of an instruction, i.e. it is being written to.
 *
 * \return  The allocated register number.
 */
uint8_t
AssemblyGenerator::AllocateRegisterAndMakeActive(
    const std::string& identifier,
    bool isLhs
)
{
    if ( m_availableRegs.empty() )
    {
        LOG_ERROR_AND_THROW( "Cannot allocate register - no available ones left.", std::runtime_error );
    }

    auto extractedElement = m_availableRegs.extract( m_availableRegs.begin() );
    uint8_t allocatedReg = extractedElement.value();

    // If the operand is the LHS (target), this means it is written to.
    const bool isWrittenTo{ isLhs };
    AddToActive( identifier, allocatedReg, isWrittenTo );
    return allocatedReg;
}

/**
 * \brief  Adds the identifier and its allocated register to the map of active variables, sorted in ascending live
 *         interval end points.
 *
 * \param[in]  identifier         The variable being set as active.
 * \param[in]  allocatedRegister  The register allocated to the variable.
 * \param[in]  isWrittenTo        Whether the variable has been written to. This determines if it needs to be saved at
 *                                the end of the block.
 *
 * \return  The equivalent assembly opcode.
 */
void
AssemblyGenerator::AddToActive(
    const std::string& identifier,
    uint8_t allocatedRegister,
    bool isWrittenTo
)
{
    ActiveVarInfo varInfo = std::make_pair( allocatedRegister, isWrittenTo );

    if ( m_liveIntervals.end() == m_liveIntervals.find( identifier ) )
    {
        LOG_ERROR_AND_THROW( "No live interval could be found for '" + identifier + "'", std::runtime_error );
    }
    size_t endPointOfVar = m_liveIntervals[identifier].second;

    bool inserted{ false };
    for ( auto it = m_currentActiveVars.begin(); it != m_currentActiveVars.end(); ++it )
    {
        std::string currentId = it->first;
        if ( m_liveIntervals.end() == m_liveIntervals.find( currentId ) )
        {
            LOG_ERROR_AND_THROW( "No live interval could be found for '" + currentId + "'", std::runtime_error );
        }
        size_t currentEndPoint = m_liveIntervals[currentId].second;

        // Insert the variable before the entry with the higher end point.
        if ( endPointOfVar <= currentEndPoint )
        {
            m_currentActiveVars.insert( it, { identifier, varInfo } );
            inserted = true;
            break;
        }
    }
    if ( !inserted )
    {
        m_currentActiveVars.insert( m_currentActiveVars.end(), {identifier, varInfo});
    }
}