#include <boost/test/unit_test.hpp>

#include "ThreeAddrInstruction.h"

using namespace TAC;

BOOST_AUTO_TEST_SUITE( ThreeAddrInstructionTests )

BOOST_AUTO_TEST_SUITE( EqualityOperator )

BOOST_AUTO_TEST_CASE( EqualInstructions )
{
    std::string result{ "result" };
    Opcode operation{ Opcode::LS };
    Operand operand1{ 5 };
    Operand operand2{ "id" };
    std::string label = "label1";

    ThreeAddrInstruction::Ptr instr1
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, operand2, label );
    ThreeAddrInstruction::Ptr instr2
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, operand2, label );

    BOOST_CHECK( *instr1.get() == *instr2.get() );
}

BOOST_AUTO_TEST_CASE( UnequalInstructions_NonVariantMembers )
{
    std::string result{ "result" };
    Opcode operation{ Opcode::LS };
    Operand operand1{ 2 };
    Operand operand2{ "id" };
    std::string label = "";

    ThreeAddrInstruction::Ptr comparisonInstr
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, operand2, label );

    ThreeAddrInstruction::Ptr differentResult
        = std::make_shared< ThreeAddrInstruction >( "differentResult", operation, operand1, operand2, label );
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *differentResult.get() );

    ThreeAddrInstruction::Ptr differentOpcode
        = std::make_shared< ThreeAddrInstruction >( result, Opcode::ADD, operand1, operand2, label );
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *differentOpcode.get() );

    ThreeAddrInstruction::Ptr differentLabel
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, operand2, "differentLabel");
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *differentLabel.get() );
}

BOOST_AUTO_TEST_CASE( UnequalInstructions_VariantMembers )
{
    std::string result{ "result" };
    Opcode operation{ Opcode::LS };
    Operand operand1{ 2 };
    Operand operand2{ "id" };
    std::string label = "";

    ThreeAddrInstruction::Ptr comparisonInstr
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, operand2, label );

    Operand differentLiteralOperand{ 10 };
    Operand differentIdOperand{ "hello!" };

    // Same operand type, different value
    ThreeAddrInstruction::Ptr sameTypeLiteral
        = std::make_shared< ThreeAddrInstruction >( result, operation, differentLiteralOperand, operand2, label );
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *sameTypeLiteral.get() );

    ThreeAddrInstruction::Ptr sameTypeId
        = std::make_shared< ThreeAddrInstruction >( result, operation, operand1, differentIdOperand, label );
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *sameTypeId.get() );

    // Different operand type
    ThreeAddrInstruction::Ptr differentType
        = std::make_shared< ThreeAddrInstruction >( result, operation, differentIdOperand, operand2, label );
    BOOST_CHECK_EQUAL( false, *comparisonInstr.get() == *differentType.get() );
}

BOOST_AUTO_TEST_SUITE_END() // EqualityOperator

BOOST_AUTO_TEST_SUITE_END() // ThreeAddrInstructionTests