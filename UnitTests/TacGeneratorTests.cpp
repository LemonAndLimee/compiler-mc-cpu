#include <boost/test/unit_test.hpp>

#include "TacGenerator.h"
#include "TacInstructionFactoryMock.h"

class TacGeneratorTestsFixture
{
public:
    TacGeneratorTestsFixture()
    : m_instructionFactoryMock( std::make_shared< TacInstructionFactoryMock >() )
    {
        m_generator = std::make_shared< TacGenerator >( m_instructionFactoryMock );
    }

    enum ExpectLabel
    {
        LBL_FALSE,
        LBL_TRUE
    };
    enum ExpectResult
    {
        RES_FALSE,
        RES_TRUE
    };

    // Wrapper around the mock expect call for AddInstruction()
    void
    ExpectAddInstruction(
        const std::string& expectedTarget,
        Opcode opcode,
        Operand operand1,
        Operand operand2,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->AddInstruction )
            .once()
            .in( sequence )
            .with( expectedTarget, opcode, operand1, operand2 );
    }
    // Wrapper around the mock expect call for AddSingleOperandInstruction()
    void
    ExpectAddSingleOperandInstruction(
        const std::string& expectedTarget,
        Opcode opcode,
        Operand operand,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->AddSingleOperandInstruction )
            .once()
            .in( sequence )
            .with( expectedTarget, opcode, operand );
    }
    // Wrapper around the mock expect call for AddNoOperandsInstruction()
    void
    ExpectAddNoOperandsInstruction(
        const std::string& expectedTarget,
        Opcode opcode,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->AddNoOperandsInstruction )
            .once()
            .in( sequence )
            .with( expectedTarget, opcode );
    }
    // Wrapper around the mock expect call for AddAssignmentInstruction()
    void
    ExpectAddAssignmentInstruction(
        const std::string& expectedTarget,
        Operand operand,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->AddAssignmentInstruction )
            .once()
            .in( sequence )
            .with( expectedTarget, operand );
    }

    /**
     * \brief  Checks the mock calls for an assignment of a new temp variable
     *
     * \param[in]  idToReturn   Variable ID to mock return.
     * \param[in]  expectedLhs  Expected operand to be calling with.
     * \param[in]  sequence     Turtle mock sequence the call should be in.
     */
    void
    CheckNewTempVarCalls(
        const std::string& idToReturn,
        Operand expectedLhs,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->GetNewTempVar )
            .once()
            .in( sequence )
            .returns( idToReturn );
        MOCK_EXPECT( m_instructionFactoryMock->AddAssignmentInstruction )
            .once()
            .in( sequence )
            .with( idToReturn, expectedLhs );
    }

    /**
     * \brief  Checks the mock calls for getting a new label and setting it as the next instruction label.
     *
     * \param[in]  labelToReturn  Label to mock return.
     * \param[in]  sequence       Turtle mock sequence the call should be in.
     */
    void
    CheckGetAndSetLabelCalls(
        const std::string& labelToReturn,
        mock::sequence sequence
    )
    {
        MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel )
            .once()
            .in( sequence )
            .returns( labelToReturn );
        MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel )
            .once()
            .in( sequence )
            .with( labelToReturn );
    }

    /**
     * \brief  Checks the instructions generated for a comparison operation. This is a shared set of checks as they
     *         all share the same pattern.
     *
     * \param[in]  resultIdToReturn   The mock temp var string to return when a result ID is requested.
     * \param[in]  branchOpcode       The opcode of the expected branch instruction.
     * \param[in]  branchOperand1     The first expected branch operand.
     * \param[in]  branchOperand2     The second expected branch operand.
     * \param[in]  valueIfBranchTrue  The literal value to initialise the result with at first. This value is returned
     *                                if the branch condition is true, i.e. the second assignment is skipped.
     */
    void
    ExpectComparisonInstructions(
        const std::string& resultIdToReturn,
        Opcode branchOpcode,
        Operand branchOperand1,
        Operand branchOperand2,
        Literal valueIfBranchTrue
    )
    {
        mock::sequence sequence;

        uint8_t initialValue{ valueIfBranchTrue };
        CheckNewTempVarCalls( resultIdToReturn, initialValue, sequence );

        const std::string endLabel{ "endLabel" };
        MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( endLabel );
        ExpectAddInstruction( endLabel, branchOpcode, branchOperand1, branchOperand2, sequence );

        uint8_t nonBranchValue{ static_cast< bool >( !valueIfBranchTrue ) };
        ExpectAddAssignmentInstruction( resultIdToReturn, nonBranchValue, sequence );

        MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( endLabel );
    }

protected:

    TacInstructionFactoryMock::Ptr m_instructionFactoryMock;

    // Unit under test
    TacGenerator::Ptr m_generator;

    // Some pre-defined example operands to be used in tests.
    const Operand c_emptyOp{};
    const Operand c_stringOp{ "identifier" };
    const Operand c_stringOp2{ "identifier2" };
    const Operand c_literalOp_Five{ 5u };
    const Operand c_literalOp_Two{ 2u };
    const Operand c_zeroOperand{ 0u };
    const Literal c_trueLiteral{ 1u };
    const Literal c_falseLiteral{ 0u };
};

BOOST_FIXTURE_TEST_SUITE( TacGeneratorTests, TacGeneratorTestsFixture )

BOOST_AUTO_TEST_SUITE( MultiplyTests )

/**
 * Tests that the method for generating TAC for multiplication will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Multiply_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Multiply( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for multiplication will return a numeric value with the operation result
 * if both operands are literals, and does not add any instructions.
 */
BOOST_AUTO_TEST_CASE( Multiply_TwoLiterals )
{
    Operand result = m_generator->Multiply( c_literalOp_Five, c_literalOp_Two );

    Literal literal1{ std::get< Literal >( c_literalOp_Five ) };
    Literal literal2{ std::get< Literal >( c_literalOp_Two ) };
    Literal expectedResult{ static_cast< Literal >( literal1 * literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );
}

/**
 * Tests that the method for generating TAC for multiplication will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 *
 * We expect a shift-and-add algorithm to be used where the LSB of the multiplier is used to determine whether to shift
 * the multiplicand, as it is added to a total.
 */
BOOST_AUTO_TEST_CASE( Multiply_Identifier )
{
    /**
     * Expect the following algorithm:
     *
     * result = 0
     * multiplier = op1
     * multiplicand = op2
     * bitCounter = 8
     *
     * loop: andResult = multiplier && 0xFE
     * BRZ shift andResult
     * result = result + multiplicand
     * shift: multiplicand = << multiplicand
     * multiplier = >> multiplier
     * bitCounter = bitCounter - 1
     * BRGT loop bitCounter 0
     *
     * (return result)
     */

    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };

    mock::sequence sequence;

    // First four instructions should be initialising the temp vars.

    const std::string resultId{ "result" };
    constexpr uint8_t expectedResultInit{ 0u };
    CheckNewTempVarCalls( resultId, expectedResultInit, sequence );

    const std::string multiplierId{ "multiplier" };
    CheckNewTempVarCalls( multiplierId, operand1, sequence );

    std::string multiplicandId{ "multiplicand" };
    CheckNewTempVarCalls( multiplicandId, operand2, sequence );

    const std::string bitCounterId{ "bitCounter" };
    constexpr uint8_t expectedBitCounterInit{ 8u };
    CheckNewTempVarCalls( bitCounterId, expectedBitCounterInit, sequence );

    // Main loop:

    const std::string mainLoopLabel{ "mainLoopLabel" };
    CheckGetAndSetLabelCalls( mainLoopLabel, sequence );
    const std::string andTargetId{ "andTarget" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewTempVar ).once().in( sequence ).returns( andTargetId );
    constexpr uint8_t expectedAndBitmask{ 0xFE }; // Bitmask to get the LSB
    ExpectAddInstruction( andTargetId, Opcode::AND, multiplierId, expectedAndBitmask, sequence );

    // Pre-fetch the label for the shift operation, so we can check its value for the next branch instruction.
    const std::string shiftLabel{ "shiftLabel" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( shiftLabel );

    // Check is branching to the shift instructions.
    ExpectAddSingleOperandInstruction( shiftLabel, Opcode::BRZ, andTargetId, sequence );

    ExpectAddInstruction( resultId, Opcode::ADD, resultId, multiplicandId, sequence );

    // Do expect a label because this is where it branches past the addition line.
    MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( shiftLabel );
    ExpectAddSingleOperandInstruction( multiplicandId, Opcode::LS, multiplicandId, sequence );

    ExpectAddSingleOperandInstruction( multiplierId, Opcode::RS, multiplierId, sequence );

    constexpr uint8_t expectedSubAmount{ 1u };
    ExpectAddInstruction( bitCounterId, Opcode::SUB, bitCounterId, expectedSubAmount, sequence );

    constexpr uint8_t expectedCompValue{ 0u };
    ExpectAddInstruction( mainLoopLabel, Opcode::BRGT, bitCounterId, expectedCompValue, sequence );


    Operand result = m_generator->Multiply( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // MultiplyTests

BOOST_AUTO_TEST_SUITE( DivideTests )

/**
 * Tests that the method for generating TAC for division will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Divide_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Divide( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Divide( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Divide( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for division will throw an error if the quotient (operand 2) is 0.
 */
BOOST_AUTO_TEST_CASE( DivideByZero )
{
    BOOST_CHECK_THROW( m_generator->Divide( c_stringOp, c_zeroOperand ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for division will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions.
 */
BOOST_AUTO_TEST_CASE( Divide_TwoLiterals )
{
    Operand result = m_generator->Divide( c_literalOp_Five, c_literalOp_Two );

    Literal literal1{ std::get< Literal >( c_literalOp_Five ) };
    Literal literal2{ std::get< Literal >( c_literalOp_Two ) };
    Literal expectedResult{ static_cast< Literal >( literal1 / literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );
}

/**
 * Tests that the method for generating TAC for division will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 *
 * We expect a repeated subtraction algorithm to be used.
 */
BOOST_AUTO_TEST_CASE( Divide_Identifier )
{
    /**
     * Expect the following algorithm:
     *
     * result = 0
     * dividend = op1
     * quotient = op2
     *
     * loop: BRGT end quotient dividend
     * result = result + 1
     * dividend = dividend - quotient
     * BRU loop
     * end:
     *
     * (return result)
     */

    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };

    mock::sequence sequence;

    // First 3 instructions should be initialising the temp vars.

    const std::string resultId{ "result" };
    constexpr uint8_t expectedResultInit{ 0u };
    CheckNewTempVarCalls( resultId, expectedResultInit, sequence );

    const std::string dividendId{ "dividend" };
    CheckNewTempVarCalls( dividendId, operand1, sequence );

    const std::string quotientId{ "quotient" };
    CheckNewTempVarCalls( quotientId, operand2, sequence );

    // Main loop:

    // Expect to be branching to the end label. Check this at the end of the program.
    const std::string endLabel{ "endLabel" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( endLabel );
    const std::string mainLoopLabel{ "mainLoopLabel" };
    CheckGetAndSetLabelCalls( mainLoopLabel, sequence );
    ExpectAddInstruction( endLabel, Opcode::BRGT, quotientId, dividendId, sequence );

    const uint8_t increment{ 1u };
    ExpectAddInstruction( resultId, Opcode::ADD, resultId, increment, sequence );

    ExpectAddInstruction( dividendId, Opcode::SUB, dividendId, quotientId, sequence );

    // Check is branching to main loop
    ExpectAddNoOperandsInstruction( mainLoopLabel, Opcode::BRU, sequence );

    MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( endLabel );


    Operand result = m_generator->Divide( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // DivideTests

BOOST_AUTO_TEST_SUITE( ModuloTests )

/**
 * Tests that the method for generating TAC for modulo will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Modulo_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Modulo( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Modulo( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Modulo( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for modulo will throw an error if the quotient (operand 2) is 0.
 */
BOOST_AUTO_TEST_CASE( ModuloByZero )
{
    BOOST_CHECK_THROW( m_generator->Modulo( c_stringOp, c_zeroOperand ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for modulo will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions.
 */
BOOST_AUTO_TEST_CASE( Modulo_TwoLiterals )
{
    Operand result = m_generator->Modulo( c_literalOp_Five, c_literalOp_Two );

    Literal literal1{ std::get< Literal >( c_literalOp_Five ) };
    Literal literal2{ std::get< Literal >( c_literalOp_Two ) };
    Literal expectedResult{ static_cast< Literal >( literal1 % literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );
}

/**
 * Tests that the method for generating TAC for modulo will return an identifier of a temporary storage of the
 * result.
 *
 * We expect a repeated subtraction algorithm to be used.
 */
BOOST_AUTO_TEST_CASE( Modulo_Identifier )
{
    /**
     * Expect the following algorithm:
     *
     * result = 0
     * dividend = op1
     * quotient = op2
     *
     * loop: BRGT end quotient dividend
     * result = result + 1
     * dividend = dividend - quotient
     * BRU loop
     * end:
     *
     * (return dividend)
     */

    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };

    mock::sequence sequence;

    // First 3 instructions should be initialising the temp vars.

    const std::string resultId{ "result" };
    constexpr uint8_t expectedResultInit{ 0u };
    CheckNewTempVarCalls( resultId, expectedResultInit, sequence );

    const std::string dividendId{ "dividend" };
    CheckNewTempVarCalls( dividendId, operand1, sequence );

    const std::string quotientId{ "quotient" };
    CheckNewTempVarCalls( quotientId, operand2, sequence );

    // Main loop:

    // Expect to be branching to the end label. Check this at the end of the program.
    const std::string endLabel{ "endLabel" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( endLabel );
    const std::string mainLoopLabel{ "mainLoopLabel" };
    CheckGetAndSetLabelCalls( mainLoopLabel, sequence );
    ExpectAddInstruction( endLabel, Opcode::BRGT, quotientId, dividendId, sequence );

    const uint8_t increment{ 1u };
    ExpectAddInstruction( resultId, Opcode::ADD, resultId, increment, sequence );

    ExpectAddInstruction( dividendId, Opcode::SUB, dividendId, quotientId, sequence );

    // Check is branching to main loop
    ExpectAddNoOperandsInstruction( mainLoopLabel, Opcode::BRU, sequence );

    MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( endLabel );


    Operand result = m_generator->Modulo( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( dividendId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // ModuloTests

BOOST_AUTO_TEST_SUITE( ComparisonTests )

BOOST_AUTO_TEST_SUITE( EqualsTests )

/**
 * Tests that the method for generating TAC for == will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Equals_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Equals( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Equals( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Equals( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for == will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions.
 */
BOOST_AUTO_TEST_CASE( Equals_TwoLiterals )
{
    // False case
    Operand result = m_generator->Equals( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result ) );

    // True case
    Operand result2 = m_generator->Equals( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result2 ) );
}

/**
 * Tests that the method for generating TAC for == will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( Equals_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_trueLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRE, operand1, operand2, valueIfTrue );

    Operand result = m_generator->Equals( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // EqualsTests

BOOST_AUTO_TEST_SUITE( NotEqualsTests )

/**
 * Tests that the method for generating TAC for != will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( NotEquals_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->NotEquals( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->NotEquals( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->NotEquals( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for != will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( NotEquals_TwoLiterals )
{
    // True case
    Operand result = m_generator->NotEquals( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // False case
    Operand result2 = m_generator->NotEquals( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result2 ) );
}

/**
 * Tests that the method for generating TAC for != will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( NotEquals_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_falseLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRE, operand1, operand2, valueIfTrue );

    Operand result = m_generator->NotEquals( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // NotEqualsTests

BOOST_AUTO_TEST_SUITE( LeqTests )

/**
 * Tests that the method for generating TAC for <= will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Leq_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Leq( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Leq( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Leq( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for <= will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( Leq_TwoLiterals )
{
    // True case - less than
    Operand result = m_generator->Leq( c_literalOp_Two, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // True case - equals
    Operand result2 = m_generator->Leq( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result2 ) );

    // False case - greater than
    Operand result3 = m_generator->Leq( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for <= will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( Leq_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_falseLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRGT, operand1, operand2, valueIfTrue );

    Operand result = m_generator->Leq( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // LeqTests

BOOST_AUTO_TEST_SUITE( GeqTests )

/**
 * Tests that the method for generating TAC for >= will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Geq_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Geq( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Geq( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Geq( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for >= will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( Geq_TwoLiterals )
{
    // True case - greater than
    Operand result = m_generator->Geq( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // True case - equals
    Operand result2 = m_generator->Geq( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result2 ) );

    // False case - less than
    Operand result3 = m_generator->Geq( c_literalOp_Two, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for >= will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( Geq_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_falseLiteral; // False if op1 < op2

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRLT, operand1, operand2, valueIfTrue );

    Operand result = m_generator->Geq( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // GeqTests

BOOST_AUTO_TEST_SUITE( LessThanTests )

/**
 * Tests that the method for generating TAC for < will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( LessThan_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->LessThan( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LessThan( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LessThan( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for < will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( LessThan_TwoLiterals )
{
    // False case - greater than
    Operand result = m_generator->LessThan( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result ) );

    // False case - equals
    Operand result2 = m_generator->LessThan( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result2 ) );

    // True case - less than
    Operand result3 = m_generator->LessThan( c_literalOp_Two, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for < will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( LessThan_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_trueLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRLT, operand1, operand2, valueIfTrue );

    Operand result = m_generator->LessThan( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // LessThanTests

BOOST_AUTO_TEST_SUITE( GreaterThanTests )

/**
 * Tests that the method for generating TAC for > will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( GreaterThan_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->GreaterThan( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->GreaterThan( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->GreaterThan( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for > will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( GreaterThan_TwoLiterals )
{
    // True case - greater than
    Operand result = m_generator->GreaterThan( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // False case - equals
    Operand result2 = m_generator->GreaterThan( c_literalOp_Five, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result2 ) );

    // False case - less than
    Operand result3 = m_generator->GreaterThan( c_literalOp_Two, c_literalOp_Five );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for > will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( GreaterThan_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand1{ c_literalOp_Five };
    const Operand operand2{ c_stringOp };
    const Literal valueIfTrue = c_trueLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRGT, operand1, operand2, valueIfTrue );

    Operand result = m_generator->GreaterThan( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // GreaterThanTests

BOOST_AUTO_TEST_SUITE( LogicalNotTests )

/**
 * Tests that the method for generating TAC for logical NOT will throw an error if the  operand is empty.
 */
BOOST_AUTO_TEST_CASE( LogicalNot_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->LogicalNot( c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for logical NOT will return a numeric value with the operation result
 * ifthe operand is a literal.
 */
BOOST_AUTO_TEST_CASE( LogicalNot_Literal )
{
    // True case
    Operand result = m_generator->LogicalNot( c_zeroOperand );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // False case
    Operand result2 = m_generator->LogicalNot( c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result2 ) );
}

/**
 * Tests that the method for generating TAC for logical NOT will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( LogicalNot_Identifier )
{
    const std::string resultIdToReturn{ "result" };
    const Operand operand{ c_stringOp };
    const Literal valueIfTrue = c_trueLiteral;

    ExpectComparisonInstructions( resultIdToReturn, Opcode::BRGT, operand, c_zeroOperand, valueIfTrue );

    Operand result = m_generator->LogicalNot( operand );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultIdToReturn, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // LogicalNotTests

BOOST_AUTO_TEST_SUITE( LogicalOrTests )

/**
 * Tests that the method for generating TAC for logical OR will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( LogicalOr_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->LogicalOr( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LogicalOr( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LogicalOr( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for logical OR will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( LogicalOr_TwoLiterals )
{
    // True case - both >0
    Operand result = m_generator->LogicalOr( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // True case - one operand is >0, one ==0
    Operand result2 = m_generator->LogicalOr( c_zeroOperand, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result2 ) );

    // False case - both zero
    Operand result3 = m_generator->LogicalOr( c_zeroOperand, c_zeroOperand );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for logical OR will return a true value if one operand is a literal
 * representing a >0 value, and it will return the other operand if the literal is 0.
 */
BOOST_AUTO_TEST_CASE( LogicalOr_OneLiteral )
{
    // True cases - the literal is >0

    Operand trueResult1 = m_generator->LogicalOr( c_literalOp_Five, c_stringOp );
    BOOST_REQUIRE( std::holds_alternative< Literal >( trueResult1 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( trueResult1 ) );

    Operand trueResult2 = m_generator->LogicalOr( c_stringOp, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( trueResult2 ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( trueResult2 ) );


    // Cases where the literal is 0

    Operand op2Return = m_generator->LogicalOr( c_zeroOperand, c_stringOp );
    // Expect it to return operand 2 in this case.
    BOOST_REQUIRE( std::holds_alternative< std::string >( op2Return ) );
    BOOST_CHECK_EQUAL( std::get< std::string >( c_stringOp ), std::get< std::string >( op2Return ) );

    Operand op1Return = m_generator->LogicalOr( c_stringOp, c_zeroOperand );
    // Expect it to return operand 2 in this case.
    BOOST_REQUIRE( std::holds_alternative< std::string >( op1Return ) );
    BOOST_CHECK_EQUAL( std::get< std::string >( c_stringOp ), std::get< std::string >( op1Return ) );
}

/**
 * Tests that the method for generating TAC for logical OR will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( LogicalOr_TwoIdentifiers )
{
    const Opcode expectedBranchOpcode{ BRGT };
    const Operand operand1{ c_stringOp };
    const Operand operand2{ c_stringOp2 };
    const Literal valueIfBranchTrue{ c_trueLiteral };

    mock::sequence sequence;


    const std::string resultId{ "result" };
    uint8_t initialValue{ valueIfBranchTrue };
    CheckNewTempVarCalls( resultId, initialValue, sequence );

    const std::string endLabel{ "endLabel" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( endLabel );

    ExpectAddInstruction( endLabel, expectedBranchOpcode, operand1, c_zeroOperand, sequence );
    ExpectAddInstruction( endLabel, expectedBranchOpcode, operand2, c_zeroOperand, sequence );

    uint8_t nonBranchValue{ static_cast< bool >( !valueIfBranchTrue ) };
    ExpectAddAssignmentInstruction( resultId, nonBranchValue, sequence );

    MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( endLabel );


    Operand result = m_generator->LogicalOr( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // LogicalOrTests

BOOST_AUTO_TEST_SUITE( LogicalAndTests )

/**
 * Tests that the method for generating TAC for logical AND will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( LogicalAnd_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->LogicalAnd( c_emptyOp, c_stringOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LogicalAnd( c_stringOp, c_emptyOp ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->LogicalAnd( c_emptyOp, c_emptyOp ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for logical AND will return a numeric value with the operation result
 * if both operands are literals.
 */
BOOST_AUTO_TEST_CASE( LogicalAnd_TwoLiterals )
{
    // True case - both >0
    Operand result = m_generator->LogicalAnd( c_literalOp_Five, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( c_trueLiteral, std::get< Literal >( result ) );

    // False case - one operand is >0, one ==0
    Operand result2 = m_generator->LogicalAnd( c_zeroOperand, c_literalOp_Two );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result2 ) );

    // False case - both zero
    Operand result3 = m_generator->LogicalAnd( c_zeroOperand, c_zeroOperand );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result3 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( result3 ) );
}

/**
 * Tests that the method for generating TAC for logical AND will return a literal "false" value if one of the
 * operands is a literal storing the value "false". This is because regardless of the other value, it will resolve
 * to false.
 * If one operand is "true", it should return the other operand.
 */
BOOST_AUTO_TEST_CASE( LogicalAnd_OneLiteral )
{
    // False cases - the literal is 0

    Operand falseResult1 = m_generator->LogicalAnd( c_zeroOperand, c_stringOp );
    BOOST_REQUIRE( std::holds_alternative< Literal >( falseResult1 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( falseResult1 ) );

    Operand falseResult2 = m_generator->LogicalAnd( c_stringOp, c_zeroOperand );
    BOOST_REQUIRE( std::holds_alternative< Literal >( falseResult2 ) );
    BOOST_CHECK_EQUAL( c_falseLiteral, std::get< Literal >( falseResult2 ) );


    // Cases where the literal is >0

    Operand op2Return = m_generator->LogicalAnd( c_literalOp_Two, c_stringOp );
    // Expect it to return operand 2 in this case.
    BOOST_REQUIRE( std::holds_alternative< std::string >( op2Return ) );
    BOOST_CHECK_EQUAL( std::get< std::string >( c_stringOp ), std::get< std::string >( op2Return ) );

    Operand op1Return = m_generator->LogicalAnd( c_stringOp, c_literalOp_Two );
    // Expect it to return operand 1 in this case.
    BOOST_REQUIRE( std::holds_alternative< std::string >( op1Return ) );
    BOOST_CHECK_EQUAL( std::get< std::string >( c_stringOp ), std::get< std::string >( op1Return ) );
}

/**
 * Tests that the method for generating TAC for logical AND will return an identifier of a temporary storage of the
 * result.
 */
BOOST_AUTO_TEST_CASE( LogicalAnd_TwoIdentifiers )
{
    const Opcode expectedBranchOpcode{ BRGT };
    const Operand operand1{ c_stringOp };
    const Operand operand2{ c_stringOp2 };
    const Literal valueIfBranchTrue{ c_falseLiteral };

    mock::sequence sequence;


    const std::string resultId{ "result" };
    uint8_t initialValue{ valueIfBranchTrue };
    CheckNewTempVarCalls( resultId, initialValue, sequence );

    const std::string endLabel{ "endLabel" };
    MOCK_EXPECT( m_instructionFactoryMock->GetNewLabel ).once().in( sequence ).returns( endLabel );

    ExpectAddInstruction( endLabel, expectedBranchOpcode, operand1, c_zeroOperand, sequence );
    ExpectAddInstruction( endLabel, expectedBranchOpcode, operand2, c_zeroOperand, sequence );

    uint8_t nonBranchValue{ static_cast< bool >( !valueIfBranchTrue ) };
    ExpectAddAssignmentInstruction( resultId, nonBranchValue, sequence );

    MOCK_EXPECT( m_instructionFactoryMock->SetNextInstructionLabel ).once().in( sequence ).with( endLabel );


    Operand result = m_generator->LogicalAnd( operand1, operand2 );

    // Check the returned operand is pointing to the result id string
    BOOST_REQUIRE( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // LogicalAndTests

BOOST_AUTO_TEST_SUITE_END() // ComparisonTests

BOOST_AUTO_TEST_SUITE_END() // TacGeneratorTests