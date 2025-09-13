#include <boost/test/unit_test.hpp>

#include "TacInstructionFactory.h"

class TacInstrFactoryTestsFixture
{
public:
    TacInstrFactoryTestsFixture()
    : m_instructionFactory( std::make_shared< TacInstructionFactory >() )
    {};

    TacInstructionFactory::Ptr m_instructionFactory;
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
 * Tests that the method for getting a new branch label will return the pre-set result if a value has already been
 * specified. Test it will not get used a second time.
 */
BOOST_AUTO_TEST_CASE( GetNewLabel_ReusePrevious )
{
    BOOST_CHECK_EQUAL( "label0", m_instructionFactory->GetNewLabel() );
    std::string nextLabel = "next";
    m_instructionFactory->SetNextLabel( nextLabel );
    BOOST_CHECK_EQUAL( nextLabel, m_instructionFactory->GetNewLabel() );
    BOOST_CHECK_EQUAL( "label1", m_instructionFactory->GetNewLabel());
}

BOOST_AUTO_TEST_SUITE_END() // TacInstructionFactoryTests