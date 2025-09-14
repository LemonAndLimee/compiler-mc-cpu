#pragma once

#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

#include "TacInstructionFactory.h"

MOCK_BASE_CLASS( TacInstructionFactoryMock, TacInstructionFactory )
{
    using Ptr = std::shared_ptr< TacInstructionFactoryMock >;

    MOCK_METHOD( GetNewTempVar, 1, std::string( std::string ) );
    MOCK_METHOD( GetNewLabel, 1, std::string( std::string ) );

    MOCK_METHOD( GetNextInstructionLabel, 0, std::string( void ) );
    MOCK_METHOD( SetNextInstructionLabel, 1, void( const std::string& ) );

    MOCK_METHOD( AddInstruction, 4, void( std::string, Opcode, Operand, Operand ) );
    MOCK_METHOD( AddSingleOperandInstruction, 3, void( std::string, Opcode, Operand ) );
    MOCK_METHOD( AddNoOperandsInstruction, 2, void( std::string, Opcode ) );
    MOCK_METHOD( AddAssignmentInstruction, 2, void( std::string, Operand ) );
};