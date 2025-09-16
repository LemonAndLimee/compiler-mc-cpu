#include <boost/test/unit_test.hpp>

#include "TacInstructionFactory.h"

// Extended access version of the factory class for testing.
class InstructionFactory_Test : public TacInstructionFactory
{
public:
    using Ptr = std::shared_ptr< InstructionFactory_Test >;
    using TacInstructionFactory::TacInstructionFactory;
    using TacInstructionFactory::m_instructions;
    using TacInstructionFactory::m_nextInstrLabel;
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
    BOOST_CHECK_EQUAL( "0label", m_instructionFactory->GetNewLabel() );
    BOOST_CHECK_EQUAL( "1label", m_instructionFactory->GetNewLabel() );
    const std::string tempLabel = "testLabel";
    BOOST_CHECK_EQUAL( "2" + tempLabel, m_instructionFactory->GetNewLabel( tempLabel ) );
}

/**
 * Tests that the method for getting the next instruction label returns an empty string if one has not been set.
 */
BOOST_AUTO_TEST_CASE( SetNextLabel )
{
    BOOST_CHECK_EQUAL( "", m_instructionFactory->m_nextInstrLabel );
    const std::string testLabel = "testLabel";
    m_instructionFactory->SetNextInstructionLabel( testLabel );
    BOOST_CHECK_EQUAL( testLabel, m_instructionFactory->m_nextInstrLabel );
}

/**
 * Tests that the method for getting the next instruction label throws an error if one has already been set.
 */
BOOST_AUTO_TEST_CASE( SetNextLabel_AlreadyExists )
{
    BOOST_CHECK_EQUAL( "", m_instructionFactory->m_nextInstrLabel );
    const std::string testLabel = "testLabel";
    m_instructionFactory->SetNextInstructionLabel( testLabel );
    BOOST_CHECK_THROW( m_instructionFactory->SetNextInstructionLabel( testLabel ), std::runtime_error );
}

BOOST_AUTO_TEST_SUITE( AddInstructionTests )

/**
 * Tests that the method for adding an instruction successfully creates one with the given values, and adds it to the
 * instructions storage.
 */
BOOST_AUTO_TEST_CASE( AddInstruction_Identifiers )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    const std::string op1String{ "op1" };
    const Operand operand1{ op1String };
    const std::string op2String{ "op2" };
    const Operand operand2{ op2String };

    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];

    BOOST_CHECK_EQUAL( target, instruction->m_target );

    BOOST_REQUIRE( instruction->IsOperation() );
    Operation::Ptr rhsOperation = instruction->GetOperation();
    BOOST_CHECK_EQUAL( opcode, rhsOperation->opcode );
    BOOST_CHECK_EQUAL( op1String, rhsOperation->operand1 );
    BOOST_CHECK_EQUAL( op2String, rhsOperation->operand2 );

    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

/**
 * Tests that the method for adding an instruction will add a prerequisite temporary var assignment if one of the
 * operands is a non-zero literal.
 */
BOOST_AUTO_TEST_CASE( AddInstruction_NonZeroLiteralOperand )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    constexpr Literal literalValue{ 5u };
    const Operand operand1{ literalValue };
    const std::string op2String{ "op2" };
    const Operand operand2{ op2String };

    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    // Expect 2 instructions to have been added - one assignment before the actual instruction.
    BOOST_REQUIRE_EQUAL( 2u, m_instructionFactory->m_instructions.size() );

    ThreeAddrInstruction::Ptr assignmentInstr = m_instructionFactory->m_instructions[0];
    std::string tempVar = assignmentInstr->m_target;
    BOOST_REQUIRE( !assignmentInstr->IsOperation() );
    Operand rhsOperand = std::get< Operand >( assignmentInstr->m_rhs );
    BOOST_REQUIRE( std::holds_alternative< Literal >( rhsOperand ) );
    BOOST_CHECK_EQUAL( literalValue, std::get< Literal >( rhsOperand ) );


    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[1];
    BOOST_CHECK_EQUAL( target, instruction->m_target );

    BOOST_REQUIRE( instruction->IsOperation() );
    Operation::Ptr rhsOperation = instruction->GetOperation();
    BOOST_CHECK_EQUAL( opcode, rhsOperation->opcode );
    BOOST_CHECK_EQUAL( tempVar, rhsOperation->operand1 );
    BOOST_CHECK_EQUAL( op2String, rhsOperation->operand2 );

    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

/**
 * Tests that the method for adding an instruction will swap any zero literal operands for an empty string when
 * creating the instruction.
 */
BOOST_AUTO_TEST_CASE( AddInstruction_ZeroLiteralOperand )
{
    BOOST_CHECK_EQUAL( 0u, m_instructionFactory->m_instructions.size() );

    const std::string target{ "target" };
    constexpr Opcode opcode{ ADD };
    const std::string op1String{ "op1" };
    const Operand operand1{ op1String };
    const Operand operand2{ 0u };

    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];

    BOOST_CHECK_EQUAL( target, instruction->m_target );

    BOOST_REQUIRE( instruction->IsOperation() );
    Operation::Ptr rhsOperation = instruction->GetOperation();
    BOOST_CHECK_EQUAL( opcode, rhsOperation->opcode );
    BOOST_CHECK_EQUAL( op1String, rhsOperation->operand1 );
    BOOST_CHECK( ThreeAddrInstruction::IsOperandEmpty( rhsOperation->operand2 ) );

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
    const std::string op1String{ "op1" };
    const Operand operand1{ op1String };
    const std::string op2String{ "op2" };
    const Operand operand2{ op2String };

    m_instructionFactory->AddInstruction( target, opcode, operand1, operand2 );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];

    BOOST_CHECK_EQUAL( target, instruction->m_target );

    BOOST_REQUIRE( instruction->IsOperation() );
    Operation::Ptr rhsOperation = instruction->GetOperation();
    BOOST_CHECK_EQUAL( opcode, rhsOperation->opcode );
    BOOST_CHECK_EQUAL( op1String, rhsOperation->operand1 );
    BOOST_CHECK_EQUAL( op2String, rhsOperation->operand2 );

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
    const std::string operandString{ "op1" };
    const Operand operand{ operandString };
    m_instructionFactory->AddSingleOperandInstruction( target, opcode, operand );

    BOOST_REQUIRE_EQUAL( 1u, m_instructionFactory->m_instructions.size() );
    ThreeAddrInstruction::Ptr instruction = m_instructionFactory->m_instructions[0];
    BOOST_CHECK_EQUAL( target, instruction->m_target );

    BOOST_CHECK( instruction->IsOperation() );
    Operation::Ptr rhsOperation = instruction->GetOperation();
    BOOST_CHECK_EQUAL( opcode, rhsOperation->opcode );
    BOOST_CHECK_EQUAL( operandString, rhsOperation->operand1 );
    BOOST_CHECK( ThreeAddrInstruction::IsOperandEmpty( rhsOperation->operand2 ) );

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

    BOOST_CHECK( !instruction->IsOperation() );
    Operand rhs = std::get< Operand >( instruction->m_rhs );
    BOOST_CHECK( operand == rhs );

    BOOST_CHECK_EQUAL( "", instruction->m_label );
}

BOOST_AUTO_TEST_SUITE_END() // AddInstructionTests

/**
 * Tests that the method for pointing a branch instruction to the end will throw if given a nullptr instruction.
 */
BOOST_AUTO_TEST_CASE( SetInstructionBranchToEndLabel_NullptrInstr )
{
    BOOST_CHECK_THROW( m_instructionFactory->SetInstructionBranchToNextLabel( nullptr, "" ), std::invalid_argument );
}

/**
 * Tests that the method for pointing a branch instruction to the end will throw if given a non-branch instruction.
 */
BOOST_AUTO_TEST_CASE( SetInstructionBranchToEndLabel_NonBranchInstr )
{
    const std::string target{ "target" };
    constexpr Opcode opcode{ LS }; // Non-branch opcode
    const std::string operand{ "value" };

    ThreeAddrInstruction::Ptr instr = std::make_shared< ThreeAddrInstruction >( target, opcode, operand, operand );
    BOOST_CHECK_THROW( m_instructionFactory->SetInstructionBranchToNextLabel( instr, "" ), std::invalid_argument );
}

/**
 * Tests that the method for pointing a branch instruction to the end will create a new end label first if the 'next
 * instruction label' isn't already set, then assign it to the target of the given instruction.
 */
BOOST_AUTO_TEST_CASE( SetInstructionBranchToEndLabel_CreatesNewEndLabel )
{
    const std::string placeholderTarget{ TacInstructionFactory::PLACEHOLDER };
    constexpr Opcode opcode{ BRE };
    const std::string operand{ "value" }; // Operands don't matter for this test

    ThreeAddrInstruction::Ptr instr
        = std::make_shared< ThreeAddrInstruction >( placeholderTarget, opcode, operand, operand );

    BOOST_REQUIRE_EQUAL( "", m_instructionFactory->m_nextInstrLabel );
    BOOST_REQUIRE_EQUAL( placeholderTarget, instr->m_target );
    m_instructionFactory->SetInstructionBranchToNextLabel( instr, "end" );

    std::string endLabel = m_instructionFactory->m_nextInstrLabel;
    BOOST_CHECK_NE( "", endLabel );
    BOOST_CHECK_EQUAL( endLabel, instr->m_target );
}

/**
 * Tests that the method for pointing a branch instruction to the end will assign the existing end label to the target
 * of the given instruction.
 */
BOOST_AUTO_TEST_CASE( SetInstructionBranchToEndLabel_ExistingEndLabel )
{
    const std::string placeholderTarget{ TacInstructionFactory::PLACEHOLDER };
    constexpr Opcode opcode{ BRE };
    const std::string operand{ "value" }; // Operands don't matter for this test

    ThreeAddrInstruction::Ptr instr
        = std::make_shared< ThreeAddrInstruction >( placeholderTarget, opcode, operand, operand );
    BOOST_REQUIRE_EQUAL( placeholderTarget, instr->m_target );

    const std::string predefinedEndLabel{ "endLabel" };
    m_instructionFactory->SetNextInstructionLabel( predefinedEndLabel );
    BOOST_REQUIRE_EQUAL( predefinedEndLabel, m_instructionFactory->m_nextInstrLabel );

    m_instructionFactory->SetInstructionBranchToNextLabel( instr, "end" );

    // Check the 'next label' has not been changed since it was already set to a value.
    BOOST_CHECK_EQUAL( predefinedEndLabel, m_instructionFactory->m_nextInstrLabel );
    BOOST_CHECK_EQUAL( predefinedEndLabel, instr->m_target );
}

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
    BOOST_REQUIRE( !fillerInstr->IsOperation() );
    Operand rhsOperand = std::get< Operand >( fillerInstr->m_rhs );
    BOOST_REQUIRE( std::holds_alternative< Literal >( rhsOperand ) );
    const Literal expectedValue{ 0u };
    BOOST_CHECK_EQUAL( expectedValue, std::get< Literal >( rhsOperand ) );
}

BOOST_AUTO_TEST_SUITE_END() // TacInstructionFactoryTests