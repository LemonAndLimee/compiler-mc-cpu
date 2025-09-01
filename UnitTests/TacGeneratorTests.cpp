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
     * \param[in]  ExpectResult    Whether to expect a result string (the value itself is not checked).
     */
    void
    CheckInstrAttributes(
        Opcode expectedOpcode,
        Operand expectedOperand1,
        Operand expectedOperand2,
        ExpectLabel expectLabel,
        ExpectResult ExpectResult
    )
    {
        BOOST_CHECK_EQUAL( expectedOpcode, m_currentInstruction->m_operation );

        if ( std::holds_alternative< std::monostate >( expectedOperand1 ) )
        {
            BOOST_CHECK( std::holds_alternative< std::monostate >( m_currentInstruction->m_operand1 ) );
        }
        else
        {
            BOOST_CHECK( expectedOperand1 == m_currentInstruction->m_operand1 );
        }

        if ( std::holds_alternative< std::monostate >( expectedOperand2 ) )
        {
            BOOST_CHECK( std::holds_alternative< std::monostate >( m_currentInstruction->m_operand2 ) );
        }
        else
        {
            BOOST_CHECK( expectedOperand2 == m_currentInstruction->m_operand2 );
        }

        bool labelIsEmpty = m_currentInstruction->m_label == "";
        bool labelCheck = ( ExpectLabel::LBL_TRUE == expectLabel ) ? !labelIsEmpty : labelIsEmpty;
        BOOST_CHECK( labelCheck );

        bool resultIsEmpty = m_currentInstruction->m_result == "";
        bool resultCheck = ( ExpectResult::RES_TRUE == ExpectResult ) ? !resultIsEmpty : resultIsEmpty;
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
        std::string result = m_currentInstruction->m_result;
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
        std::string label = m_currentInstruction->m_label;
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

protected:
    // Unit under test
    TacGenerator::Ptr m_generator;

    // Current instruction being considered by the test - stored here to reduce code clutter.
    ThreeAddrInstruction::Ptr m_currentInstruction;
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
    Operand invalidOperand{};
    Operand invalidOperand2{};
    Operand validOperand{ "identifier" };

    Instructions instructions{};

    BOOST_CHECK_THROW( m_generator->Multiply( invalidOperand, validOperand, instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( validOperand, invalidOperand, instructions ), std::invalid_argument );
    BOOST_CHECK_THROW( m_generator->Multiply( invalidOperand, invalidOperand2, instructions ), std::invalid_argument );
}

/**
 * Tests that the method for generating TAC for multiplication will return a numeric value with the operation result
 * if both operands are literals, and does not add any instructions to the given container.
 */
BOOST_AUTO_TEST_CASE( Multiply_Success_TwoLiterals )
{
    constexpr uint8_t byte1{ 2u };
    constexpr uint8_t byte2{ 5u };

    Operand operand1{ byte1 };
    Operand operand2{ byte2 };

    Instructions instructions{};

    BOOST_CHECK_EQUAL( 0u, instructions.size() );
    Operand result = m_generator->Multiply( operand1, operand2, instructions );

    constexpr uint8_t expectedResult{ byte1 * byte2 };
    BOOST_REQUIRE( std::holds_alternative< Literal >( result ) );
    BOOST_CHECK_EQUAL( expectedResult, std::get< Literal >( result ) );

    BOOST_CHECK_EQUAL( 0u, instructions.size() );
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
    constexpr uint8_t byte{ 2u };
    const std::string id{ "identifier"};

    Operand operand1{ byte };
    Operand operand2{ id };

    Instructions instructions{ nullptr }; // Initialise with an element to test that previous contents are not removed.
    size_t initialSize{ instructions.size() };

    BOOST_CHECK_EQUAL( 1u, initialSize );
    Operand result = m_generator->Multiply( operand1, operand2, instructions );

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
    BOOST_REQUIRE_EQUAL( expectedNumInstructionsAdded + initialSize, instructions.size() );

    m_currentInstruction = instructions[0];
    BOOST_CHECK_EQUAL( nullptr, m_currentInstruction );

    // First four instructions should be initialising the temp vars.

    m_currentInstruction = instructions[1];
    constexpr uint8_t expectedResultInit{ 0u };
    CheckInstrAttributes( Opcode::UNUSED, expectedResultInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string resultId = GetResultIdAndCheckValid();

    m_currentInstruction = instructions[2];
    CheckInstrAttributes( Opcode::UNUSED, operand1, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string multiplierId = GetResultIdAndCheckValid();

    m_currentInstruction = instructions[3];
    CheckInstrAttributes( Opcode::UNUSED, operand2, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string multiplicandId = GetResultIdAndCheckValid();

    m_currentInstruction = instructions[4];
    constexpr uint8_t expectedBitCounterInit{ 8u };
    CheckInstrAttributes( Opcode::UNUSED, expectedBitCounterInit, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    std::string bitCounterId = GetResultIdAndCheckValid();

    // Main loop:

    m_currentInstruction = instructions[5];
    constexpr uint8_t expectedAndBitmask{ 0xFE }; // Bitmask to get the LSB
    CheckInstrAttributes( Opcode::AND, multiplierId, expectedAndBitmask, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    std::string andResultId = GetResultIdAndCheckValid();
    std::string mainLoopLabel = GetLabelAndCheckValid();

    // Pre-fetch the label for the shift operation, so we can check its value for the next branch instruction.
    m_currentInstruction = instructions[8];
    std::string shiftLabel = GetLabelAndCheckValid();

    m_currentInstruction = instructions[6];
    CheckInstrAttributes( Opcode::BRZ, andResultId, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( shiftLabel, GetResultIdAndCheckValid() ); // Check is branching to the shift instructions.

    m_currentInstruction = instructions[7];
    CheckInstrAttributes( Opcode::ADD, resultId, multiplicandId, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    BOOST_CHECK_EQUAL( resultId, GetResultIdAndCheckValid() ); // Check is returning the operation into "result".

    m_currentInstruction = instructions[8];
    // Do expect a label because this is where it branches past the addition line.
    CheckInstrAttributes( Opcode::LS, multiplicandId, {}, ExpectLabel::LBL_TRUE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "multiplicand".
    BOOST_CHECK_EQUAL( multiplicandId, GetResultIdAndCheckValid() );

    m_currentInstruction = instructions[9];
    CheckInstrAttributes( Opcode::RS, multiplierId, {}, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "multiplier".
    BOOST_CHECK_EQUAL( multiplierId, GetResultIdAndCheckValid() );

    m_currentInstruction = instructions[10];
    constexpr uint8_t expectedSubAmount{ 1u };
    CheckInstrAttributes( Opcode::SUB, bitCounterId, expectedSubAmount, ExpectLabel::LBL_FALSE, ExpectResult::RES_TRUE );
    // Check is returning the operation into "bit counter".
    BOOST_CHECK_EQUAL( bitCounterId, GetResultIdAndCheckValid() );

    m_currentInstruction = instructions[11];
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

BOOST_AUTO_TEST_SUITE_END() // TacGeneratorTests