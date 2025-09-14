#include <boost/test/unit_test.hpp>

#include "TacInstructionFactory.h"

// Extended access version of the factory class for testing.
class InstructionFactory_Test : public TacInstructionFactory
{
public:
    using Ptr = std::shared_ptr< InstructionFactory_Test >;
    using TacInstructionFactory::TacInstructionFactory;
    using TacInstructionFactory::m_instructions;
};

class TacInstrFactoryTestsFixture
{
public:
    TacInstrFactoryTestsFixture()
    : m_instructionFactory( std::make_shared< InstructionFactory_Test >() )
    {};

    InstructionFactory_Test::Ptr m_instructionFactory;
};

BOOST_FIXTURE_TEST_SUITE( TacInstructionFactoryTests, TacInstrFactoryTestsFixture )

/**
 * Tests that the method for getting a new temp variable will start at 0, and increment 1 at a time.
 */
BOOST_AUTO_TEST_CASE( GetNewTempVar )
{
    BOOST_CHECK_EQUAL( "0temp", m_instructionFactory->GetNewTempVar() );
    const std::string tempName = "testName";
    BOOST_CHECK_EQUAL( "1" + tempName, m_instructionFactory->GetNewTempVar( tempName ) );
    BOOST_CHECK_EQUAL( "2temp", m_instructionFactory->GetNewTempVar() );
}

/**
 * Tests that the method for getting a new branch label will start at 0, and increment 1 at a time.
 */
BOOST_AUTO_TEST_CASE( GetNewLabel )
{
    BOOST_CHECK_EQUAL( "label0", m_instructionFactory->GetNewLabel() );
    BOOST_CHECK_EQUAL( "label1", m_instructionFactory->GetNewLabel() );
    const std::string tempLabel = "testLabel";
    BOOST_CHECK_EQUAL( tempLabel + "2", m_instructionFactory->GetNewLabel( tempLabel ) );
}

/**
 * Tests that the method for getting the next instruction label returns an empty string if one has not been set.
 */
BOOST_AUTO_TEST_CASE( GetNextLabel_DoesNotExist )
{
    BOOST_CHECK_EQUAL( "", m_instructionFactory->GetNextInstructionLabel() );
}

/**
 * Tests that the method for getting the next instruction label returns an empty string if one has not been set.
 */
BOOST_AUTO_TEST_CASE( GetSetNextLabel )
{
    BOOST_CHECK_EQUAL( "", m_instructionFactory->GetNextInstructionLabel() );
    const std::string testLabel = "testLabel";
    m_instructionFactory->SetNextInstructionLabel( testLabel );
    BOOST_CHECK_EQUAL( testLabel, m_instructionFactory->GetNextInstructionLabel() );
}

/**
 * Tests that the method for getting the next instruction label throws an error if one has already been set.
 */
BOOST_AUTO_TEST_CASE( SetNextLabel_AlreadyExists )
{
    BOOST_CHECK_EQUAL( "", m_instructionFactory->GetNextInstructionLabel() );
    const std::string testLabel = "testLabel";
    m_instructionFactory->SetNextInstructionLabel( testLabel );
    BOOST_CHECK_THROW( m_instructionFactory->SetNextInstructionLabel( testLabel ), std::runtime_error );
}

BOOST_AUTO_TEST_SUITE( AddInstructionTests )

/**
 * Tests that the method for adding an instruction successfully creates one with the given values, and adds it to the
 * instructions storage.
 */
BOOST_AUTO_TEST_CASE( AddInstruction )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    const Operand operand1{ "op1" };
    const Operand operand2{ 5u };
    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );
    BOOST_CHECK_EQUAL( opcode, instruction->m_opcode );
    BOOST_CHECK( operand1 == instruction->m_operand1 );
    BOOST_CHECK( operand2 == instruction->m_operand2 );
    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

/**
 * Tests that the method for adding an instruction successfully creates and adds an instruction with a given label if
 * one has already been set.
 */
BOOST_AUTO_TEST_CASE( AddInstruction_PreSetLabel )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string label{ "label" };
    m_instructionFactory->SetNextInstructionLabel( label );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    const Operand operand1{ "op1" };
    const Operand operand2{ 5u };
    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );
    BOOST_CHECK_EQUAL( opcode, instruction->m_opcode );
    BOOST_CHECK( operand1 == instruction->m_operand1 );
    BOOST_CHECK( operand2 == instruction->m_operand2 );
    BOOST_CHECK_EQUAL( label, instruction->m_label );
}

/**
 * Tests that the method for adding a 1-operand instruction successfully creates one with the given values, and adds it
 * to the instructions storage.
 */
BOOST_AUTO_TEST_CASE( AddSingleOperandInstruction )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    const Operand operand{ "op1" };
    m_instructionFactory->AddSingleOperandInstruction( target, opcode, operand );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );
    BOOST_CHECK_EQUAL( opcode, instruction->m_opcode );
    BOOST_CHECK( operand == instruction->m_operand1 );
    BOOST_CHECK( std::holds_alternative< std::monostate >( instruction->m_operand2 ) );
    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

/**
 * Tests that the method for adding an instruction with no operands successfully creates one with the given values, and
 * adds it to the instructions storage.
 */
BOOST_AUTO_TEST_CASE( AddNoOperandsInstruction )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    m_instructionFactory->AddNoOperandsInstruction( target, opcode );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );
    BOOST_CHECK_EQUAL( opcode, instruction->m_opcode );
    BOOST_CHECK( std::holds_alternative< std::monostate >( instruction->m_operand1 ) );
    BOOST_CHECK( std::holds_alternative< std::monostate >( instruction->m_operand2 ) );
    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

/**
 * Tests that the method for adding an assignment instruction successfully creates one with the given values, and adds
 * it to the instructions storage.
 */
BOOST_AUTO_TEST_CASE( AddAssignmentInstruction )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    const Operand operand{ "op1" };
    m_instructionFactory->AddAssignmentInstruction( target, operand );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );
    BOOST_CHECK_EQUAL( Opcode::UNUSED, instruction->m_opcode );
    BOOST_CHECK( operand == instruction->m_operand1 );
    BOOST_CHECK( std::holds_alternative< std::monostate >( instruction->m_operand2 ) );
    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

BOOST_AUTO_TEST_SUITE_END() // AddInstructionTests

/**
 * Tests that the method for getting instructions successfully returns the stored instructions container.
 */
BOOST_AUTO_TEST_CASE( GetInstructions )
{
    // Populate with some instructions
    const std::string target{ "target" };
    const Operand operand{ "op1" };
    m_instructionFactory->AddAssignmentInstruction( target, operand );
    m_instructionFactory->AddAssignmentInstruction( target, operand );
    const std::string target2{ "target2" };
    const Operand operand2{ "op2" };
    m_instructionFactory->AddAssignmentInstruction( target2, operand2 );

    TacInstructionFactory::Instructions instructions = m_instructionFactory->GetInstructions();
    BOOST_CHECK_EQUAL_COLLECTIONS( instructions.begin(),
                                   instructions.end(),
                                   m_instructionFactory->m_instructions.begin(),
                                   m_instructionFactory->m_instructions.end() );
}

/**
 * Tests that the method for getting instructions will add a filler instruction on the end if there is still a label
 * due, before returning.
 */
BOOST_AUTO_TEST_CASE( GetInstructions_AddsInstrOnEnd )
{
    const std::string target{ "target" };
    const Operand operand{ "op1" };
    m_instructionFactory->AddAssignmentInstruction( target, operand );

    const std::string label{ "label" };
    m_instructionFactory->SetNextInstructionLabel( label );

    BOOST_CHECK_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    TacInstructionFactory::Instructions instructions = m_instructionFactory->GetInstructions();
    BOOST_CHECK_EQUAL( 2u, m_instructionFactory->m_instructions.size() );

    BOOST_CHECK_EQUAL_COLLECTIONS( instructions.begin(),
                                   instructions.end(),
                                   m_instructionFactory->m_instructions.begin(),
                                   m_instructionFactory->m_instructions.end() );

    ThreeAddrInstruction::Ptr fillerInstr = instructions[1];
    // Expect assignment to 0, with the correct label.
    BOOST_CHECK_EQUAL( label, fillerInstr->m_label );
    BOOST_CHECK_EQUAL( Opcode::UNUSED, fillerInstr->m_opcode );
    const Operand expectedOp{ 0u };
    BOOST_CHECK( expectedOp == fillerInstr->m_operand1 );
}

BOOST_AUTO_TEST_SUITE_END() // TacInstructionFactoryTests