#include <boost/test/unit_test.hpp>
#include "SymbolTable.h"

/**
 * Derived copy of SymbolTable class, with extended access for testing.
 */
class SymbolTable_Test : public SymbolTable
{
public:
    using Ptr = std::shared_ptr< SymbolTable_Test >;
    using SymbolTable::SymbolTable;
    using SymbolTable::m_table;
};

BOOST_AUTO_TEST_SUITE( SymbolTableTests )

/**
 * Tests that GetEntryIfExists will throw an exception if AddEntry is called with a nullptr entry.
 */
BOOST_AUTO_TEST_CASE( AddEntry_NullptrEntry )
{
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( nullptr );
    BOOST_CHECK_THROW( currentTable->AddEntry( entryIdentifier, nullptr ), std::runtime_error );
}

/**
 * Tests that GetEntryIfExists successfully add the given entry to its map member.
 */
BOOST_AUTO_TEST_CASE( AddEntry_Success )
{
    SymbolTableEntry::Ptr entry = std::make_shared< SymbolTableEntry >();
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( nullptr );
    BOOST_CHECK( currentTable->m_table.empty() );
    currentTable->AddEntry( entryIdentifier, entry );

    BOOST_CHECK_EQUAL( 1u, currentTable->m_table.size() );
    SymbolTableEntry::Ptr foundEntry = currentTable->m_table.find( entryIdentifier )->second;
    BOOST_CHECK_EQUAL( entry.get(), foundEntry.get() );
}

/**
 * Tests that GetEntryIfExists will throw an exception if AddEntry is called with an identifier for which an entry
 * already exists.
 */
BOOST_AUTO_TEST_CASE( AddEntry_EntryAlreadyExists )
{
    SymbolTableEntry::Ptr entry = std::make_shared< SymbolTableEntry >();
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( nullptr );
    BOOST_CHECK( currentTable->m_table.empty() );
    currentTable->AddEntry( entryIdentifier, entry );

    BOOST_CHECK_EQUAL( 1u, currentTable->m_table.size() );

    SymbolTableEntry::Ptr entry2 = std::make_shared< SymbolTableEntry >();
    BOOST_CHECK_THROW( currentTable->AddEntry( entryIdentifier, entry2 ), std::runtime_error );
}

/**
 * Tests that GetEntryIfExists will successfully return the corresponding entry if it exists in the current table.
 */
BOOST_AUTO_TEST_CASE( GetEntry_ExistsInCurrentTable )
{
    SymbolTableEntry::Ptr entry = std::make_shared< SymbolTableEntry >();
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( nullptr );
    currentTable->AddEntry( entryIdentifier, entry );

    SymbolTableEntry::Ptr foundEntry = currentTable->GetEntryIfExists( entryIdentifier );
    BOOST_CHECK_EQUAL( foundEntry.get(), entry.get() );
}

/**
 * Tests that GetEntryIfExists will successfully return the corresponding entry if it exists in a parent table.
 */
BOOST_AUTO_TEST_CASE( GetEntry_ExistsInParentTable )
{
    SymbolTableEntry::Ptr entry = std::make_shared< SymbolTableEntry >();
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr parentTable = std::make_shared< SymbolTable_Test >( nullptr );
    parentTable->AddEntry( entryIdentifier, entry );

    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( parentTable );

    SymbolTableEntry::Ptr foundEntry = currentTable->GetEntryIfExists( entryIdentifier );
    BOOST_CHECK_EQUAL( foundEntry.get(), entry.get() );
}

/**
 * Tests that GetEntryIfExists will return nullptr if no corresponding entry exists in the table or any higher tables.
 */
BOOST_AUTO_TEST_CASE( GetEntry_NoMatch )
{
    const std::string entryIdentifier = "idName";

    SymbolTable_Test::Ptr parentTable = std::make_shared< SymbolTable_Test >( nullptr );
    SymbolTable_Test::Ptr currentTable = std::make_shared< SymbolTable_Test >( parentTable );

    SymbolTableEntry::Ptr foundEntry = currentTable->GetEntryIfExists( entryIdentifier );
    BOOST_CHECK_EQUAL( nullptr, foundEntry );
}

BOOST_AUTO_TEST_SUITE_END() // SymbolTableTests