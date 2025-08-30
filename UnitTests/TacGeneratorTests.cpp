#include <boost/test/unit_test.hpp>

#include "TacGenerator.h"

class TacGeneratorTestsFixture
{
public:
    TacGeneratorTestsFixture()
    : m_generator( std::make_shared< TacGenerator >() )
    {
    }

protected:
    TacGenerator::Ptr m_generator;
};

BOOST_FIXTURE_TEST_SUITE( TacGeneratorTests, TacGeneratorTestsFixture )

/**
 * Tests that the method for getting a new temp variable will start at 0, and increment 1 at a time.
 */
BOOST_AUTO_TEST_CASE( GetNewTempVar )
{
    BOOST_CHECK_EQUAL( "0temp", m_generator->GetNewTempVar() );
    BOOST_CHECK_EQUAL( "1temp", m_generator->GetNewTempVar() );
    BOOST_CHECK_EQUAL( "2temp", m_generator->GetNewTempVar() );
}

BOOST_AUTO_TEST_SUITE_END() // TacGeneratorTests