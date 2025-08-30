#include <boost/test/unit_test.hpp>

#include "TacGenerator.h"

class TacGeneratorTestsFixture
{
public:
    TacGeneratorTestsFixture()
    : m_generator( std::make_shared< TacGenerator >() )
    {
    }

    /**
     * \brief  Asserts that two collections of instructions are equal.
     *
     * \param[in]  lhs  The first collection of instructions.
     * \param[in]  rhs  The second collection of instructions, being compared.
     */
    void
    CheckInstructionsEqual(
        const Instructions& lhs,
        const Instructions& rhs
    )
    {
        BOOST_REQUIRE_EQUAL( lhs.size(), rhs.size() );

        for ( size_t index = 0; index < lhs.size(); ++index )
        {
            BOOST_CHECK( *lhs[index].get() == *rhs[index].get() );
        }
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

BOOST_AUTO_TEST_SUITE_END() // TacGeneratorTests