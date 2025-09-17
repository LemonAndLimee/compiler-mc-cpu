#include <boost/test/unit_test.hpp>

#include "AssemblyGenerator.h"

using namespace Assembly;

// Derived class of the assembly generator, with extended member access for testing.
class AssemblyGenerator_Test : public AssemblyGenerator
{
public:
    using Ptr = std::shared_ptr< AssemblyGenerator_Test >;
    using AssemblyGenerator::AssemblyGenerator;
    using AssemblyGenerator::m_basicBlockStarts;
    using AssemblyGenerator::LiveInterval;
    using AssemblyGenerator::m_liveIntervals;
};

BOOST_AUTO_TEST_SUITE( AssemblyGeneratorTests )

/**
 * Tests that method for calculating basic block boundaries will populate its collection with a single zero element if
 * there are no branching instructions, i.e. there is only one block.
 */
BOOST_AUTO_TEST_CASE( CalculateBasicBlocks_NoBranches )
{
    // Simulate a few simple operations, with no branching or labels.
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", 5u ),
        std::make_shared< TAC::ThreeAddrInstruction >( "var2", TAC::Opcode::ADD, "var1", "var1" ),
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", TAC::Opcode::LS, "var2", "" )
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_basicBlockStarts.size() );
    generator->CalculateBasicBlocks();

    BOOST_REQUIRE_EQUAL( 1u, generator->m_basicBlockStarts.size() );
    BOOST_CHECK_EQUAL( 0u, generator->m_basicBlockStarts[0] );
}

/**
 * Tests that method for calculating basic block boundaries will populate its collection with a single zero element if
 * there are no branching instructions except for one at the very end.
 */
BOOST_AUTO_TEST_CASE( CalculateBasicBlocks_OneBranchAtEnd )
{
    // Simulate a few simple operations, with 1 branching instruction as the last instruction.
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", 5u ),
        std::make_shared< TAC::ThreeAddrInstruction >( "var2", TAC::Opcode::ADD, "var1", "var1" ),
        std::make_shared< TAC::ThreeAddrInstruction >( "branchTarget", TAC::Opcode::BRE, "var2", "" )
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_basicBlockStarts.size() );
    generator->CalculateBasicBlocks();

    BOOST_REQUIRE_EQUAL( 1u, generator->m_basicBlockStarts.size() );
    BOOST_CHECK_EQUAL( 0u, generator->m_basicBlockStarts[0] );
}

/**
 * Tests that method for calculating basic block boundaries will successfully populate with the start of the blocks,
 * if there are multiple. Check it considers both branches and labels as block boundaries, and that it correctly
 * handles consecutive branch instructions.
 */
BOOST_AUTO_TEST_CASE( CalculateBasicBlocks_MultipleBlocks )
{
    // Simulate a a program with multiple blocks, as well as consecutive block boundaries.
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", 5u ),
        std::make_shared< TAC::ThreeAddrInstruction >( "var2", TAC::Opcode::ADD, "var1", "var1" ),
        std::make_shared< TAC::ThreeAddrInstruction >( "branchTarget", TAC::Opcode::BRE, "var2", "" ),
        // expect a block boundary here, so the next block starts at index 3
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", 5u ),
        std::make_shared< TAC::ThreeAddrInstruction >( "var2", TAC::Opcode::ADD, "var1", "var1", "label"),
        // expect a block boundary due to using a label - next block at index 5
        std::make_shared< TAC::ThreeAddrInstruction >( "branchTarget", TAC::Opcode::BRLT, "var1", "var2" ),
        // expect another immediate new block, at index 6
        std::make_shared< TAC::ThreeAddrInstruction >( "var1", 5u ),
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_basicBlockStarts.size() );
    generator->CalculateBasicBlocks();

    const std::vector< size_t > expectedBlockBoundaries{ 0, 3, 5, 6 };
    BOOST_CHECK_EQUAL_COLLECTIONS( expectedBlockBoundaries.begin(), expectedBlockBoundaries.end(),
                                   generator->m_basicBlockStarts.begin(), generator->m_basicBlockStarts.end() );
}

/**
 * Tests that method for calculating live intervals will ignore empty string identifiers, and not add any entries.
 */
BOOST_AUTO_TEST_CASE( CalculateLiveIntervals_EmptyString )
{
    // Use fake instruction with empty string identifier (this wouldn't be a valid lhs value but this is for the sake
    // of testing the live interval method only.
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( "", 5u )
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
    generator->CalculateLiveIntervals();
    // Check no entry has been added
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
}

/**
 * Tests that method for calculating live intervals will create an entry for a new variable, with the start and end
 * index being the same, if it is only used once.
 */
BOOST_AUTO_TEST_CASE( CalculateLiveIntervals_OneReference )
{
    const std::string id{ "var1"};
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( id, 5u )
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
    generator->CalculateLiveIntervals();

    BOOST_REQUIRE_EQUAL( 1u, generator->m_liveIntervals.size() );
    AssemblyGenerator_Test::LiveInterval liveInterval = generator->m_liveIntervals[id];
    constexpr size_t expectedStartIndex{ 0u };
    constexpr size_t expectedEndIndex{ expectedStartIndex };
    BOOST_CHECK_EQUAL( expectedStartIndex, liveInterval.first );
    BOOST_CHECK_EQUAL( expectedEndIndex, liveInterval.second );
}

/**
 * Tests that method for calculating live intervals will ignore a branch target, i.e. a label, as this is not a var.
 */
BOOST_AUTO_TEST_CASE( CalculateLiveIntervals_DoesntAddBranchTarget )
{
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( "branchTarget", TAC::Opcode::BRE, "", "" )
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
    generator->CalculateLiveIntervals();
    // Check no entry has been added
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
}

/**
 * Tests that method for calculating live intervals will correctly create and update an entry for a variable, with
 * the end index being its most recent access.
 */
BOOST_AUTO_TEST_CASE( CalculateLiveIntervals_MultipleReferences )
{
    const std::string varA{ "a" }; // Expect range of 0-2
    const std::string varB{ "b" }; // Expect range of 1-3
    const std::string varC{ "c" }; // Expect range of 2-2

    // Some assignment/operation instructions with variables of varying live intervals.
    AssemblyGenerator::TacInstructions instructions{
        std::make_shared< TAC::ThreeAddrInstruction >( varA, 1u ),
        std::make_shared< TAC::ThreeAddrInstruction >( varB, 2u ),
        std::make_shared< TAC::ThreeAddrInstruction >( varC, TAC::Opcode::ADD, varA, varB ),
        std::make_shared< TAC::ThreeAddrInstruction >( varB, 3u ),
    };

    AssemblyGenerator_Test::Ptr generator = std::make_shared< AssemblyGenerator_Test >( instructions );
    BOOST_CHECK_EQUAL( 0u, generator->m_liveIntervals.size() );
    generator->CalculateLiveIntervals();

    BOOST_REQUIRE_EQUAL( 3u, generator->m_liveIntervals.size() );

    AssemblyGenerator_Test::LiveInterval liveIntervalA = generator->m_liveIntervals[varA];
    BOOST_CHECK_EQUAL( 0u, liveIntervalA.first );
    BOOST_CHECK_EQUAL( 2u, liveIntervalA.second );

    AssemblyGenerator_Test::LiveInterval liveIntervalB = generator->m_liveIntervals[varB];
    BOOST_CHECK_EQUAL( 1u, liveIntervalB.first );
    BOOST_CHECK_EQUAL( 3u, liveIntervalB.second );

    AssemblyGenerator_Test::LiveInterval liveIntervalC = generator->m_liveIntervals[varC];
    BOOST_CHECK_EQUAL( 2u, liveIntervalC.first );
    BOOST_CHECK_EQUAL( 2u, liveIntervalC.second );
}

BOOST_AUTO_TEST_SUITE_END() // AssemblyGeneratorTests