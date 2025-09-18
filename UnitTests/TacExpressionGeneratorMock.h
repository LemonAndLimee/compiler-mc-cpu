#pragma once

#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

#include "TacExpressionGenerator.h"

MOCK_BASE_CLASS( TacExpressionGeneratorMock, ITacExpressionGenerator )
{
    using Ptr = std::shared_ptr< TacExpressionGeneratorMock >;

    MOCK_METHOD( Multiply, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( Divide, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( Modulo, 2, Operand( Operand, Operand ) );

    MOCK_METHOD( Equals, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( NotEquals, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( Leq, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( Geq, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( LessThan, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( GreaterThan, 2, Operand( Operand, Operand ) );

    MOCK_METHOD( LogicalNot, 1, Operand( Operand ) );
    MOCK_METHOD( LogicalOr, 2, Operand( Operand, Operand ) );
    MOCK_METHOD( LogicalAnd, 2, Operand( Operand, Operand ) );
};