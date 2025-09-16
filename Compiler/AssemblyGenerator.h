/**
 * Contains declaration of class responsible for converting TAC into assembly code.
 */

#pragma once

#include "ThreeAddrInstruction.h"

class AssemblyGenerator
{
public:
    using Ptr = std::shared_ptr< AssemblyGenerator >;
    using TacInstructions = std::vector< TAC::ThreeAddrInstruction::Ptr >;

    AssemblyGenerator( const TacInstructions& tacInstructions );

    void CalculateBasicBlocks();
    void CalculateLiveIntervals();

protected:
    void RecordVarUse( const std::string& identifier, size_t indexOfUse );

    // A collection of indexes of the start of basic blocks in the given program. If the program only consists of one
    // block, it will contain {0}.
    std::vector< size_t > m_basicBlockStarts;

    using LiveInterval = std::pair< size_t, size_t >;
    // For each variable, store its live interval, i.e. the start and end index of when it is referred to.
    std::unordered_map< std::string, LiveInterval > m_liveIntervals;

    // The TAC instructions this class is responsible for converting. All indexes used in this class refer to the index
    // of instructions in this vector, as it is const.
    const TacInstructions& m_tacInstructions;
};