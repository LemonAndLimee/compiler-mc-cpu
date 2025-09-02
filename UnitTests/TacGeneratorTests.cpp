#include <boost/test/unit_test.hpp>

#include "TacGenerator.h"

class TacGeneratorTestsFixture
{
public:
    TacGeneratorTestsFixture()
    : m_generator( std::make_shared< TacGenerator >() )
    {
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

    /**
     * \brief  Checks the current instruction against the expected values.
     *
     * \param[in]  expectedOpcode    The expected opcode value to check.
     * \param[in]  expectedOperand1  First expected operand value to check.
     * \param[in]  expectedOperand2  Second expected operand value to check.
     * \param[in]  expectLabel       Whether to expect a label value (the value itself is not checked).
     * \param[in]  expectResult      Whether to expect a result string (the value itself is not checked).
     */
    void
    CheckInstrAttributes(
        Opcode expectedOpcode,
        Operand expectedOperand1,
        Operand expectedOperand2,
        ExpectLabel expectLabel,
        ExpectResult expectResult
    )
    {
        ThreeAddrInstruction::Ptr currentInstruction = m_instructions[m_currInstrIndex];

        BOOST_CHECK_EQUAL( expectedOpcode, currentInstruction->m_operation);

        if ( std::holds_alternative< std::monostate >( expectedOperand1 ) )
        {
            BOOST_CHECK( std::holds_alternative< std::monostate >( currentInstruction->m_operand1 ) );
        }
        else
        {
            BOOST_CHECK( expectedOperand1 == currentInstruction->m_operand1 );
        }

        if ( std::holds_alternative< std::monostate >( expectedOperand2 ) )
        {
            BOOST_CHECK( std::holds_alternative< std::monostate >( currentInstruction->m_operand2 ) );
        }
        else
        {
            BOOST_CHECK( expectedOperand2 == currentInstruction->m_operand2 );
        }

        bool labelIsEmpty = currentInstruction->m_label == "";
        bool labelCheck = ( ExpectLabel::LBL_TRUE == expectLabel ) ? !labelIsEmpty : labelIsEmpty;
        BOOST_CHECK( labelCheck );

        bool resultIsEmpty = currentInstruction->m_result == "";
        bool resultCheck = ( ExpectResult::RES_TRUE == expectResult ) ? !resultIsEmpty : resultIsEmpty;
        BOOST_CHECK( resultCheck );
    }

    /**
     * \brief  Checks result string is non-empty, and returns it.
     *
     * \returns  The string of the id storing the result.
     */
    std::string
    GetResultIdAndCheckValid()
    {
        std::string result = m_instructions[m_currInstrIndex]->m_result;
        BOOST_CHECK_NE( "", result );
        return result;
    }

    /**
     * \brief  Checks label string is non-empty, and returns it.
     *
     * \returns  The string of the label associated with the given instruction.
     */
    std::string
    GetLabelAndCheckValid()
    {
        std::string label = m_instructions[m_currInstrIndex]->m_label;
        BOOST_CHECK_NE( "", label );
        return label;
    }

    /**
     * \brief  Checks the given vector of ID/label strings are all unique to each other.
     *
     * \param[in]  strings  Vector of strings.
     */
    void
    CheckStringsAreUnique(
        const std::vector< std::string >& strings
    )
    {
        std::set< std::string > s( strings.begin(), strings.end() );
        BOOST_CHECK_EQUAL( s.size(), strings.size() );
    }

    // Unit under test
    TacGenerator::Ptr m_generator;

    // Some pre-defined example operands to be used in tests. Two copies of each type so separate instances of both
    // types can be passed to a method if needed.
    Operand m_emptyOp{};
    Operand m_emptyOp2{};
    Operand m_stringOp{ "identifier" };
    Operand m_stringOp2{ "identifier2" };
    Operand m_literalOp{ 5u };
    Operand m_literalOp2{ 2u };

    // Instructions vector to be passed to generate methods.
    Instructions m_instructions;
    // Index of current instruction being considered by the test - stored here to reduce code clutter.
    size_t m_currInstrIndex;
};

BOOST_FIXTURE_TEST_SUITE( TacGeneratorTests, TacGeneratorTestsFixture )

/**
 * Tests that the method for getting a new temp variable will start at 0, and increment 1 at a time.
 */
BOOST_AUTO_TEST_CASE( GetNewTempVar )
{
    BOOST_CHECK_EQUAL( "0temp", m_generator->GetNewTempVar() );
    const std::string tempName = "testName";
    BOOST_CHECK_EQUAL( "1" + tempName, m_generator->GetNewTempVar( tempName ) );
    BOOST_CHECK_EQUAL( "2temp", m_generator->GetNewTempVar() );
}

/**
 * Tests that the method for getting a new branch label will start at 0, and increment 1 at a time.
 */
BOOST_AUTO_TEST_CASE( GetNewLabel )
{
    BOOST_CHECK_EQUAL( "label0", m_generator->GetNewLabel() );
    BOOST_CHECK_EQUAL( "label1", m_generator->GetNewLabel() );
    const std::string tempLabel = "testLabel";
    BOOST_CHECK_EQUAL( tempLabel + "2", m_generator->GetNewLabel( tempLabel ) );
}

/**
 * Tests that the method for getting a new branch label will return the pre-set result if a value has already been
 * specified. Test it will not get used a second time.
 */
BOOST_AUTO_TEST_CASE( GetNewLabel_ReusePrevious )
{
    BOOST_CHECK_EQUAL( "label0", m_generator->GetNewLabel() );
    std::string nextLabel = "next";
    m_generator->SetNextLabel( nextLabel );
    BOOST_CHECK_EQUAL( nextLabel, m_generator->GetNewLabel() );
    BOOST_CHECK_EQUAL( "label1", m_generator->GetNewLabel());
}

BOOST_AUTO_TEST_SUITE( MultiplyTests )

/**
 * Tests that the method for generating TAC for multiplication will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Multiply_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Multiply( m_emptyOp, m_stringOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( m_stringOp, m_emptyOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( m_emptyOp, m_emptyOp2, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for multiplication will return a numeric value with the operation result
 * if both operands are literals, and does not add any instructions to the given container.
 */
BOOST_AUTO_TEST_CASE( Multiply_Success_TwoLiterals )
{
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
    Operand result = m_generator->Multiply( m_literalOp, m_literalOp2, m_instructions );

    Literal literal1{ std::get< Literal >( m_literalOp ) };
    Literal literal2{ std::get< Literal >( m_literalOp2 ) };
    Literal expectedResult{ static_cast< Literal >( literal1 * literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );

    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
}

/**
 * Tests that the method for generating TAC for multiplication will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 *
 * We expect a shift-and-add algorithm to be used where the LSB of the multiplier is used to determine whether to shift
 * the multiplicand, as it is added to a total.
 */
BOOST_AUTO_TEST_CASE( Multiply_Success_Identifier )
{
    m_instructions.push_back( nullptr ); // Initialise with an element to test that previous contents are not removed.
    size_t initialSize{ m_instructions.size() };

    BOOST_CHECK_EQUAL( 1u, initialSize );
    Operand result = m_generator->Multiply( m_literalOp, m_stringOp, m_instructions );

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

    constexpr size_t expectedNumInstructionsAdded{ 11u };
    BOOST_REQUIRE_EQUAL( expectedNumInstructionsAdded + initialSize, m_instructions.size() );

    m_currInstrIndex = 0u;
    BOOST_CHECK_EQUAL( nullptr, m_instructions[m_currInstrIndex] );

    // First four instructions should be initialising the temp vars.

    m_currInstrIndex = 1u;
    constexpr uint8_t expectedResultInit{ 0u };
    CheckInstrAttributes( Opcode::UNUSED, expectedResultInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string resultId = GetResultIdAndCheckValid();

    m_currInstrIndex = 2u;
    CheckInstrAttributes( Opcode::UNUSED, m_literalOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string multiplierId = GetResultIdAndCheckValid();

    m_currInstrIndex = 3u;
    CheckInstrAttributes( Opcode::UNUSED, m_stringOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string multiplicandId = GetResultIdAndCheckValid();

    m_currInstrIndex = 4u;
    constexpr uint8_t expectedBitCounterInit{ 8u };
    CheckInstrAttributes( Opcode::UNUSED, expectedBitCounterInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string bitCounterId = GetResultIdAndCheckValid();

    // Main loop:

    m_currInstrIndex = 5u;
    constexpr uint8_t expectedAndBitmask{ 0xFE }; // Bitmask to get the LSB
    CheckInstrAttributes( Opcode::AND, multiplierId, expectedAndBitmask, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    std::string andResultId = GetResultIdAndCheckValid();
    std::string mainLoopLabel = GetLabelAndCheckValid();

    // Pre-fetch the label for the shift operation, so we can check its value for the next branch instruction.
    m_currInstrIndex = 8u;
    std::string shiftLabel = GetLabelAndCheckValid();

    m_currInstrIndex = 6u;
    CheckInstrAttributes( Opcode::BRZ, andResultId, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( shiftLabel, GetResultIdAndCheckValid() ); // Check is branching to the shift instructions.

    m_currInstrIndex = 7u;
    CheckInstrAttributes( Opcode::ADD, resultId, multiplicandId, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( resultId, GetResultIdAndCheckValid() ); // Check is returning the operation into "result".

    m_currInstrIndex = 8u;
    // Do expect a label because this is where it branches past the addition line.
    CheckInstrAttributes( Opcode::LS, multiplicandId, {}, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "multiplicand".
    BOOST_CHECK_EQUAL( multiplicandId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 9u;
    CheckInstrAttributes( Opcode::RS, multiplierId, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "multiplier".
    BOOST_CHECK_EQUAL( multiplierId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 10u;
    constexpr uint8_t expectedSubAmount{ 1u };
    CheckInstrAttributes( Opcode::SUB, bitCounterId, expectedSubAmount, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "bit counter".
    BOOST_CHECK_EQUAL( bitCounterId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 11u;
    constexpr uint8_t expectedCompValue{ 0u };
    CheckInstrAttributes( Opcode::BRGT, bitCounterId, expectedCompValue, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( mainLoopLabel, GetResultIdAndCheckValid() ); // Check is branching back to start of main loop.

    // Check all ID and label values are unique (i.e. they are being used correctly and not duplicating one another).
    std::vector< std::string > ids{ resultId, multiplierId, multiplicandId, bitCounterId, andResultId };
    CheckStringsAreUnique( ids );
    std::vector< std::string > labels{ mainLoopLabel, shiftLabel };
    CheckStringsAreUnique( labels );

    // Check the returned operand is pointing to the result id string
    BOOST_CHECK( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // MultiplyTests

BOOST_AUTO_TEST_SUITE( DivideTests )

/**
 * Tests that the method for generating TAC for division will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Divide_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Divide( m_emptyOp, m_stringOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Divide( m_stringOp, m_emptyOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Divide( m_emptyOp, m_emptyOp2, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for division will throw an error if the quotient (operand 2) is 0.
 */
BOOST_AUTO_TEST_CASE( DivideByZero )
{
    Operand zeroOperand{ 0u };

    BOOST_CHECK_THROW( m_generator->Divide( m_stringOp, zeroOperand, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for division will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions to the given container.
 */
BOOST_AUTO_TEST_CASE( Divide_Success_TwoLiterals )
{
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
    Operand result = m_generator->Divide( m_literalOp, m_literalOp2, m_instructions );

    Literal literal1{ std::get< Literal >( m_literalOp ) };
    Literal literal2{ std::get< Literal >( m_literalOp2 ) };
    Literal expectedResult{ static_cast< Literal >( literal1 / literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );

    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
}

/**
 * Tests that the method for generating TAC for division will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 *
 * We expect a repeated subtraction algorithm to be used.
 */
BOOST_AUTO_TEST_CASE( Divide_Success_Identifier )
{
    m_instructions.push_back( nullptr ); // Initialise with an element to test that previous contents are not removed.
    size_t initialSize{ m_instructions.size() };

    BOOST_CHECK_EQUAL( 1u, initialSize );
    Operand result = m_generator->Divide( m_literalOp, m_stringOp, m_instructions );

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

    constexpr size_t expectedNumInstructionsAdded{ 7u };
    BOOST_REQUIRE_EQUAL( expectedNumInstructionsAdded + initialSize, m_instructions.size() );

    m_currInstrIndex = 0u;
    BOOST_CHECK_EQUAL( nullptr, m_instructions[m_currInstrIndex] );

    // First 3 instructions should be initialising the temp vars.

    m_currInstrIndex = 1u;
    constexpr uint8_t expectedResultInit{ 0u };
    CheckInstrAttributes( Opcode::UNUSED, expectedResultInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string resultId = GetResultIdAndCheckValid();

    m_currInstrIndex = 2u;
    CheckInstrAttributes( Opcode::UNUSED, m_literalOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string dividendId = GetResultIdAndCheckValid();

    m_currInstrIndex = 3u;
    CheckInstrAttributes( Opcode::UNUSED, m_stringOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string quotientId = GetResultIdAndCheckValid();

    // Main loop:

    m_currInstrIndex = 4u;
    CheckInstrAttributes( Opcode::BRGT, quotientId, dividendId, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    // Expect to be branching to the end label. Check this at the end of the program.
    std::string endLabel = GetResultIdAndCheckValid();
    std::string mainLoopLabel = GetLabelAndCheckValid();

    m_currInstrIndex = 5u;
    CheckInstrAttributes( Opcode::ADD, resultId, 1u, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( resultId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 6u;
    CheckInstrAttributes( Opcode::SUB, dividendId, quotientId, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( dividendId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 7u;
    CheckInstrAttributes( Opcode::BRU, {}, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( mainLoopLabel, GetResultIdAndCheckValid() ); // Check is branching to main loop

    // Check that the end label is returned next time a label is requested.
    BOOST_CHECK_EQUAL( endLabel, m_generator->GetNewLabel() );

    // Check all ID and label values are unique (i.e. they are being used correctly and not duplicating one another).
    std::vector< std::string > ids{ resultId, dividendId, quotientId };
    CheckStringsAreUnique( ids );
    std::vector< std::string > labels{ mainLoopLabel, endLabel };
    CheckStringsAreUnique( labels );

    // Check the returned operand is pointing to the result id string
    BOOST_CHECK( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // DivideTests

BOOST_AUTO_TEST_SUITE( ModuloTests )

/**
 * Tests that the method for generating TAC for modulo will throw an error if either operand is empty.
 */
BOOST_AUTO_TEST_CASE( Modulo_InvalidOperands )
{
    BOOST_CHECK_THROW( m_generator->Modulo( m_emptyOp, m_stringOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Modulo( m_stringOp, m_emptyOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Modulo( m_emptyOp, m_emptyOp2, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for modulo will throw an error if the quotient (operand 2) is 0.
 */
BOOST_AUTO_TEST_CASE( ModuloByZero )
{
    Operand zeroOperand{ 0u };
    BOOST_CHECK_THROW( m_generator->Modulo( m_stringOp, zeroOperand, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for modulo will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions to the given container.
 */
BOOST_AUTO_TEST_CASE( Modulo_Success_TwoLiterals )
{
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
    Operand result = m_generator->Modulo( m_literalOp, m_literalOp2, m_instructions );

    Literal literal1{ std::get< Literal >( m_literalOp ) };
    Literal literal2{ std::get< Literal >( m_literalOp2 ) };
    Literal expectedResult{ static_cast< Literal >( literal1 % literal2 ) };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );

    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
}

/**
 * Tests that the method for generating TAC for modulo will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 *
 * We expect a repeated subtraction algorithm to be used.
 */
BOOST_AUTO_TEST_CASE( Modulo_Success_Identifier )
{
    m_instructions.push_back( nullptr ); // Initialise with an element to test that previous contents are not removed.
    size_t initialSize{ m_instructions.size() };

    BOOST_CHECK_EQUAL( 1u, initialSize );
    Operand result = m_generator->Modulo( m_literalOp, m_stringOp, m_instructions );

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

    constexpr size_t expectedNumInstructionsAdded{ 7u };
    BOOST_REQUIRE_EQUAL( expectedNumInstructionsAdded + initialSize, m_instructions.size() );

    m_currInstrIndex = 0u;
    BOOST_CHECK_EQUAL( nullptr, m_instructions[m_currInstrIndex] );

    // First 3 instructions should be initialising the temp vars.

    m_currInstrIndex = 1u;
    constexpr uint8_t expectedResultInit{ 0u };
    CheckInstrAttributes( Opcode::UNUSED, expectedResultInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string resultId = GetResultIdAndCheckValid();

    m_currInstrIndex = 2u;
    CheckInstrAttributes( Opcode::UNUSED, m_literalOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string dividendId = GetResultIdAndCheckValid();

    m_currInstrIndex = 3u;
    CheckInstrAttributes( Opcode::UNUSED, m_stringOp, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string quotientId = GetResultIdAndCheckValid();

    // Main loop:

    m_currInstrIndex = 4u;
    CheckInstrAttributes( Opcode::BRGT, quotientId, dividendId, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    // Expect to be branching to the end label. Check this at the end of the program.
    std::string endLabel = GetResultIdAndCheckValid();
    std::string mainLoopLabel = GetLabelAndCheckValid();

    m_currInstrIndex = 5u;
    CheckInstrAttributes( Opcode::ADD, resultId, 1u, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( resultId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 6u;
    CheckInstrAttributes( Opcode::SUB, dividendId, quotientId, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( dividendId, GetResultIdAndCheckValid() );

    m_currInstrIndex = 7u;
    CheckInstrAttributes( Opcode::BRU, {}, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( mainLoopLabel, GetResultIdAndCheckValid() ); // Check is branching to main loop

    // Check that the end label is returned next time a label is requested.
    BOOST_CHECK_EQUAL( endLabel, m_generator->GetNewLabel() );

    // Check all ID and label values are unique (i.e. they are being used correctly and not duplicating one another).
    std::vector< std::string > ids{ resultId, dividendId, quotientId };
    CheckStringsAreUnique( ids );
    std::vector< std::string > labels{ mainLoopLabel, endLabel };
    CheckStringsAreUnique( labels );

    // Check the returned operand is pointing to the result id string
    BOOST_CHECK( std::holds_alternative< std::string >( result ) );
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
    BOOST_CHECK_THROW( m_generator->Equals( m_emptyOp, m_stringOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Equals( m_stringOp, m_emptyOp, m_instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Equals( m_emptyOp, m_emptyOp2, m_instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for == will return a numeric value with the operation result
 * if both operands are literals, and it does not add any instructions to the given container.
 */
BOOST_AUTO_TEST_CASE( Equals_Success_TwoLiterals )
{
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );

    // False case
    Operand result = m_generator->Equals( m_literalOp, m_literalOp2, m_instructions );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    constexpr Literal falseValue{ false };
    BOOST_CHECK_EQUAL( falseValue, std::get< Literal >( result ) );
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );

    // True case
    Operand result2 = m_generator->Equals( m_literalOp, m_literalOp, m_instructions );
    BOOST_REQUIRE( std::holds_alternative< Literal >( result2 ) );
    constexpr Literal trueValue{ true };
    BOOST_CHECK_EQUAL( trueValue, std::get< Literal >( result2 ) );
    BOOST_CHECK_EQUAL( 0u, m_instructions.size() );
}

/**
 * Tests that the method for generating TAC for == will return an identifier of a temporary storage of the
 * result, and that the necessary pre-instructions are appended to the given container.
 */
BOOST_AUTO_TEST_CASE( Equals_Success_Identifier )
{
    m_instructions.push_back( nullptr ); // Initialise with an element to test that previous contents are not removed.
    size_t initialSize{ m_instructions.size() };

    BOOST_CHECK_EQUAL( 1u, initialSize );
    Operand result = m_generator->Equals( m_literalOp, m_stringOp, m_instructions );


    constexpr size_t expectedNumInstructionsAdded{ 3u };
    BOOST_REQUIRE_EQUAL( expectedNumInstructionsAdded + initialSize, m_instructions.size() );

    m_currInstrIndex = 0u;
    BOOST_CHECK_EQUAL( nullptr, m_instructions[m_currInstrIndex] );


    m_currInstrIndex = 1u;
    constexpr uint8_t initialValue{ 1u };
    CheckInstrAttributes( Opcode::UNUSED, initialValue, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string resultId = GetResultIdAndCheckValid();

    m_currInstrIndex = 2u;
    CheckInstrAttributes( Opcode::BRE, m_literalOp, m_stringOp, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string endLabel = GetResultIdAndCheckValid();

    m_currInstrIndex = 3u;
    constexpr uint8_t nonBranchValue{ 0u };
    CheckInstrAttributes( Opcode::UNUSED, nonBranchValue, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( resultId, GetResultIdAndCheckValid() );


    // Check that the end label is returned next time a label is requested.
    BOOST_CHECK_EQUAL( endLabel, m_generator->GetNewLabel() );

    // Check the returned operand is pointing to the result id string
    BOOST_CHECK( std::holds_alternative< std::string >( result ) );
    BOOST_CHECK_EQUAL( resultId, std::get< std::string >( result ) );
}

BOOST_AUTO_TEST_SUITE_END() // EqualsTests

BOOST_AUTO_TEST_SUITE_END() // ComparisonTests

BOOST_AUTO_TEST_SUITE_END() // TacGeneratorTests