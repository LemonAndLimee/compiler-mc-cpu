/**
 * Contains declaration of class responsible for converting TAC into assembly code.
 */

#pragma once

#include <map>

#include "ThreeAddrInstruction.h"

namespace Assembly
{
    enum Opcode
    {
        INVALID=0,
        ADD=1,
        SUB,
        NOT,
        AND,
        OR,
        LS,
        RS,
        LD,
        LDI,
        STR,
        BRE,
        BRLT
    };

    using InstructionTarget = std::variant< uint8_t, std::string >;
    // Label, opcode, target, operand1, operand2
    using Instruction = std::tuple< std::string, Opcode, InstructionTarget, uint8_t, uint8_t >;
    using Instructions = std::vector< Instruction >;

    // There are 15 available registers in the target architecture - addressing is 4 bits, and address 0 is reserved as
    // a null value.
    constexpr size_t TOTAL_NUM_REGS = 15u;
    constexpr size_t FIRST_REG = 1u;
    // An instruction needs a max. of 3 registers, so reserve 3 registers for loading spilled variables. Reserve one
    // more for storing memory addresses while loading these 3 spilled variables (so as not to overwrite a fetched
    // variable while fetching another).
    constexpr size_t NUM_TEMP_REGS = 4u;
    // Number of available, non-reserved registers.
    constexpr size_t NUM_AVAILABLE_REGS = TOTAL_NUM_REGS - NUM_TEMP_REGS;
    // The offset of the address of the first available general-purpose register.
    constexpr size_t AVAILABLE_REG_OFFSET = FIRST_REG + NUM_TEMP_REGS;

    constexpr size_t MEM_ADDR_TEMP_REG = FIRST_REG;
    constexpr size_t FIRST_VAR_TEMP_REG = MEM_ADDR_TEMP_REG + 1u;

    // Memory addresses should also start at 1, as 0 is considered invalid.
    constexpr size_t MEM_ADDR_OFFSET = 1u;

    class AssemblyGenerator
    {
    public:
        using Ptr = std::shared_ptr< AssemblyGenerator >;
        using TacInstructions = std::vector< TAC::ThreeAddrInstruction::Ptr >;

        AssemblyGenerator( const TacInstructions& tacInstructions );

        void CalculateBasicBlocks();
        void CalculateLiveIntervals();

        Instructions GenerateAssemblyInstructions();

    protected:
        using LiveInterval = std::pair< size_t, size_t >;

        using InstrStringArgs = std::tuple< std::string, std::string, std::string >;
        InstrStringArgs GetVarsFromInstruction( TAC::ThreeAddrInstruction::Ptr instruction );
        void RecordVarUse( const std::string& identifier, size_t indexOfUse );

        void GenerateAssemblyForBasicBlock( size_t blockStart, size_t blockEnd );

        // Stores the register number and whether a variable has been edited.
        using ActiveVarInfo = std::pair< uint8_t, bool >;
        // Stores mapping of active vars to information about them.
        using ActiveVars = std::map< std::string, ActiveVarInfo >;
        using AvailableRegs = std::set< uint8_t >;

        void SaveActiveVar( const std::string& identifier );
        void SaveRegister( uint8_t registerToSave, uint8_t memoryAddress );
        std::pair< uint8_t, uint8_t > SplitImmediateOperand( uint8_t immediateValue );
        void AddLoadImmediate( const std::string& label, uint8_t targetRegister, uint8_t immediateValue );
        void AddStoreInstruction( uint8_t registerToStore, uint8_t registerHoldingTarget );
        uint8_t GetNextMemoryLocation();

        void ExpireOldIntervals( size_t currentInstrIndex );

        void GenerateAssemblyForInstr( TAC::ThreeAddrInstruction::Ptr instruction );
        Opcode GetAssemblyOpcode( TAC::ThreeAddrInstruction::Ptr instruction );

        uint8_t GetOperandRegister( const std::string& operand, size_t operandIndex, std::string& labelOfParentInstr );
        uint8_t AllocateRegisterAndMakeActive( const std::string& identifier, bool isLhs );
        void AddToActive( const std::string& identifier, uint8_t allocatedRegister, bool isWrittenTo );

        // The TAC instructions this class is responsible for converting. All indexes used in this class refer to the
        // index of instructions in this vector, as it is const.
        const TacInstructions& m_tacInstructions;

        // Collection of assembly instructions as they are generated.
        Instructions m_assemblyInstructions;

        // A collection of indexes of the start of basic blocks in the given program. If the program only consists of
        // one block, it will contain {0}.
        std::vector< size_t > m_basicBlockStarts;
        // For each variable, store its live interval, i.e. the start and end index of when it is referred to.
        std::map< std::string, LiveInterval > m_liveIntervals;
        // Mapping between variable and its memory location, if it is either spilled or saved between blocks.
        std::unordered_map< std::string, uint8_t > m_memoryLocations;

        // The variables that are active for the current basic block.
        // Stored in the format: identifier, register number, is edited?
        ActiveVars m_currentActiveVars;
        // The registers that are available for the current basic block.
        AvailableRegs m_availableRegs;
    };

} // namespace Assembly