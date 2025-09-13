/**
 * Contains declaration of factory class for Three-Address-Code instructions.
 */

#pragma once

#include "ThreeAddrInstruction.h"

using namespace TAC;

class TacInstructionFactory
{
public:
    using Ptr = std::shared_ptr< TacInstructionFactory >;

    TacInstructionFactory();

    std::string GetNewTempVar( std::string hrfName = "temp" );
    std::string GetNewLabel( std::string hrfName = "label" );
    void SetNextLabel( const std::string& label );

private:
    // Counter of the number of temporary variables currently in use.
    size_t m_tempVarsInUse;
    // Counter of the number of branch labels currently in use.
    size_t m_labelsInUse;
    // Customisable next label to return.
    std::string m_nextLabel;
};