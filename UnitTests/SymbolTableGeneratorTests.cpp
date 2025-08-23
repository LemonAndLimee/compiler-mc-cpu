#include <boost/test/unit_test.hpp>
#include "SymbolTableGenerator.h"

/**
 * \brief  Holds utility code for test cased related to symbol table generation.
 */
class SymbolTableGeneratorTestsFixture
{
public:
    SymbolTableGeneratorTestsFixture()
    : m_generator( std::make_shared< SymbolTableGenerator >() )
    { }

    /**
     * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from a value specified
     *         by a token.
     *
     * \param[in]  varName     The name of the new variable.
     * \param[in]  isNewVar    Whether the LHS variable is new, i.e. needs declaring.
     * \param[in]  valueToken  Token representing value to assign to the variable. Can be literal or identifier.
     *
     * \return  Assignment AST node. Root of the created subtree.
     */
    AstNode::Ptr
    CreateAssignStatementSubtree(
        const std::string& varName,
        bool isNewVar,
        Token::Ptr valueToken
    )
    {
        // LHS
        AstNode::Ptr lhsNode;

        Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, varName );
        AstNode::Ptr idNode = std::make_shared< AstNode >( TokenType::IDENTIFIER, idToken );

        // If is new var, nest it inside a variable subtree.
        if ( isNewVar )
        {
            Token::Ptr dataTypeToken = std::make_shared< Token >( TokenType::DATA_TYPE, DataType::DT_BYTE );
            AstNode::Ptr dataTypeNode = std::make_shared< AstNode >( TokenType::DATA_TYPE, dataTypeToken );

            AstNode::Children varNodeChildren{ dataTypeNode, idNode };
            AstNode::Ptr variableNode = std::make_shared< AstNode >( NT::Variable, varNodeChildren );

            lhsNode = variableNode;
        }
        else
        {
            lhsNode = idNode;
        }

        // RHS
        AstNode::Ptr valueNode = std::make_shared< AstNode >( valueToken->m_type, valueToken );

        // Construct parent node
        AstNode::Children children{ lhsNode, valueNode };
        AstNode::Ptr assignNode = std::make_shared< AstNode >( TokenType::ASSIGN, children );

        return assignNode;
    }

    /**
     * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from a byte value.
     *         Delegates to CreateAssignStatementSubtree().
     */
    AstNode::Ptr
    CreateAssignNodeFromByteValue(
        const std::string& varName,
        bool isNewVar,
        uint8_t value
    )
    {
        Token::Ptr valueToken = std::make_shared< Token >( TokenType::BYTE, value );
        return CreateAssignStatementSubtree( varName, isNewVar, valueToken );
    }
    /**
     * \brief  Constructs AST subtree representing an assignment statement, of a byte variable from another variable.
     *         Delegates to CreateAssignStatementSubtree().
     */
    AstNode::Ptr
    CreateAssignNodeFromVar(
        const std::string& varName,
        bool isNewVar,
        const std::string& valueVarName )
    {
        Token::Ptr valueToken = std::make_shared< Token >( TokenType::IDENTIFIER, valueVarName );
        return CreateAssignStatementSubtree( varName, isNewVar, valueToken );
    }

    /**
     * \brief  Calls the unit under test to generate symbol table(s), and validates that one was created.
     *
     * \param[in]  scopeNode           The AST node for which to generate the table.
     * \param[in]  expectedNumEntries  The expected number of entries to validate in the created table.
     *
     * \returns  The generated table.
     */
    SymbolTable::Ptr
    GenerateTableAndValidate(
        AstNode::Ptr scopeNode,
        size_t expectedNumEntries
    )
    {
        BOOST_CHECK_EQUAL( nullptr, scopeNode->m_symbolTable );
        m_generator->GenerateSymbolTableForAst( scopeNode );
        BOOST_REQUIRE_NE( nullptr, scopeNode->m_symbolTable );
        BOOST_CHECK_EQUAL( expectedNumEntries, scopeNode->m_symbolTable->GetNumEntries() );
        return scopeNode->m_symbolTable;
    }

    /**
     * \brief  Creates scoped block AST node from child nodes vector. Utility function to avoid having to declare
     *         vector separately due to the constructor taking a reference.
     *
     * \return  Created AST scoped block node.
     */
    AstNode::Ptr
    CreateScopedNodeFromChildren(
        AstNode::Children childNodes
    )
    {
        return std::make_shared< AstNode >( NT::Scoped_block, childNodes );
    }

    /**
     * \brief  Checks for symbol table entry, and if exists checks its fields match. Verifies it holds data type "byte".
     *
     * \param[in]  table       The symbol table to search for the entry (will also search parent tables).
     * \param[in]  symbolName  The symbol name corresponding to the entry being checked.
     * \param[in]  readFrom    The expected value of the "read from" flag in the entry.
     * \param[in]  writtenTo   The expected value of the "written to" flag in the entry.
     */
    void
    CheckForByteEntry(
        SymbolTable::Ptr table,
        const std::string& symbolName,
        bool readFrom,
        bool writtenTo
    )
    {
        SymbolTableEntry::Ptr fetchedEntry = table->GetEntryIfExists( symbolName );
        BOOST_REQUIRE_NE( nullptr, fetchedEntry );

        BOOST_CHECK_EQUAL( DataType::DT_BYTE, fetchedEntry->dataType );
        BOOST_CHECK_EQUAL( readFrom, fetchedEntry->isReadFrom );
        BOOST_CHECK_EQUAL( writtenTo, fetchedEntry->isWrittenTo );
    }

protected:
    SymbolTableGenerator::Ptr m_generator;
};

BOOST_FIXTURE_TEST_SUITE( SymbolTableGeneratorTests, SymbolTableGeneratorTestsFixture )

BOOST_AUTO_TEST_SUITE( ValidationTests )

/**
 * Tests that if a call is made to generate a symbol table for a nullptr AST node, an invalid argument error is raised.
 */
BOOST_AUTO_TEST_CASE( NullptrAstNode )
{
    BOOST_CHECK_THROW( m_generator->GenerateSymbolTableForAst( nullptr ), std::invalid_argument );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST node that already has a table, a runtime error is
 * raised.
 */
BOOST_AUTO_TEST_CASE( AstNodeAlreadyHasTable )
{
    AstNode::Ptr node = std::make_shared< AstNode >( 0u, nullptr ); // Filler values to satisfy constructor
    SymbolTable::Ptr existingTable = std::make_shared< SymbolTable >( nullptr );
    node->m_symbolTable = existingTable;

    BOOST_CHECK_THROW( m_generator->GenerateSymbolTableForAst( node ), std::runtime_error );
}

BOOST_AUTO_TEST_SUITE_END() // ValidationTests

BOOST_AUTO_TEST_SUITE( SingleScopeTests )

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains no symbols, an empty table class is
 * still assigned to the AST member variable.
 */
BOOST_AUTO_TEST_CASE( AstHasNoSymbols )
{
    // Fake child nodes of the AST. Use fake tokens as the generator method should only check node types.
    // With this in mind, we can also fake NT subtree nodes by using the "Token" constructor, for convenience.

    Token::Ptr fakeToken = std::make_shared< Token >( TokenType::INVALID_TOKEN );

    AstNode::Children children{ std::make_shared< AstNode >( T::BYTE, fakeToken ),
                                std::make_shared< AstNode >( T::AND, fakeToken ),
                                std::make_shared< AstNode >( NT::For_init, fakeToken ) };
    AstNode::Ptr scopeNode = std::make_shared< AstNode >( NT::Block, children );

    constexpr size_t expectedNumEntries{ 0u };
    SymbolTable::Ptr table = GenerateTableAndValidate( scopeNode, expectedNumEntries );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains a read from a symbol that has no
 * entry, a.k.a has not been declared, an error is raised.
 */
BOOST_AUTO_TEST_CASE( UndeclaredIdentifier )
{
    // Create assignment statement that reads from undeclared variable.
    std::string varName1 = "foo";
    std::string varName2 = "bar";
    constexpr bool isNewVariable{ true };
    AstNode::Ptr assignNode = CreateAssignNodeFromVar( varName2, isNewVariable, varName1 );

    BOOST_CHECK_THROW( m_generator->GenerateSymbolTableForAst( assignNode ), std::runtime_error );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains a single unreferenced identifier,
 * the resulting table has the corresponding entry with the "read from" and "written to" flags set to false. This means
 * the entry can be removed on the next pass, and an "unreferenced variable" warning generated.
 */
BOOST_AUTO_TEST_CASE( SingleIdentifier_Unreferenced )
{
    std::string varName = "foo";
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    // Use single assign statement as the scope
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    constexpr size_t expectedNumEntries{ 1u };
    SymbolTable::Ptr table = GenerateTableAndValidate( assignNode, expectedNumEntries );

    constexpr bool expectReadFrom{ false };
    constexpr bool expectWrittenTo{ false };
    CheckForByteEntry( table, varName, expectReadFrom, expectWrittenTo );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains an identifier that is read
 * from (i.e. is not followed by an assign token), the resulting table has the corresponding entry with the "read from"
 * flag set to true.
 */
BOOST_AUTO_TEST_CASE( Identifier_ReadFrom )
{
    // Create first declaration statement.
    std::string varName = "foo";
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    AstNode::Ptr assignNode1 = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    // Create second assign statement which reads from the first.
    std::string varName2 = "bar";
    constexpr bool isNewVariable2{ true };
    AstNode::Ptr assignNode2 = CreateAssignNodeFromVar( varName2, isNewVariable2, varName );

    // Create node to hold both assignment statements.
    AstNode::Children children{ assignNode1, assignNode2 };
    AstNode::Ptr scopeNode = std::make_shared< AstNode >( NT::Block, children );

    // Expect two entries as 2 vars were declared.
    constexpr size_t expectedNumEntries{ 2u };
    SymbolTable::Ptr table = GenerateTableAndValidate( scopeNode, expectedNumEntries );

    // Expect first variable to have been read from.
    constexpr bool expectReadFrom{ true };
    constexpr bool expectWrittenTo{ false };
    CheckForByteEntry( table, varName, expectReadFrom, expectWrittenTo );

    // Expect second variable to be unreferenced.
    constexpr bool expectReadFrom2{ false };
    constexpr bool expectWrittenTo2{ false };
    CheckForByteEntry( table, varName2, expectReadFrom2, expectWrittenTo2 );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains a single identifier that is written
 * to (i.e. is followed by an assign token), the resulting table has the corresponding entry with the "written to" flag
 * set to true.
 */
BOOST_AUTO_TEST_CASE( SingleIdentifier_WrittenTo )
{
    // Create first declaration statement.
    std::string varName = "foo";
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    AstNode::Ptr assignNode1 = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    // Create second assign statement which writes to the first.
    constexpr uint8_t numValue2{ 3u };
    constexpr bool isNewVariable2{ false };
    AstNode::Ptr assignNode2 = CreateAssignNodeFromByteValue( varName, isNewVariable2, numValue2 );

    // Create node to hold both assignment statements.
    AstNode::Children children{ assignNode1, assignNode2 };
    AstNode::Ptr scopeNode = std::make_shared< AstNode >( NT::Block, children );

    constexpr size_t expectedNumEntries{ 1u };
    SymbolTable::Ptr table = GenerateTableAndValidate( scopeNode, expectedNumEntries );

    // Expect variable to have been written from but not read to.
    constexpr bool expectReadFrom{ false };
    constexpr bool expectWrittenTo{ true };
    CheckForByteEntry( table, varName, expectReadFrom, expectWrittenTo );
}

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains an identifier which is read from
 * and written to, an entry is created with the appropriate flags set to true.
 */
BOOST_AUTO_TEST_CASE( Identifier_ReadFromAndWrittenTo )
{
    // Create first declaration statement.
    std::string varName1 = "foo";
    constexpr uint8_t numValue1{ 5u };
    constexpr bool isNewVariable1{ true };
    AstNode::Ptr assignNode1 = CreateAssignNodeFromByteValue( varName1, isNewVariable1, numValue1 );

    // Create second assign statement which writes to the first.
    constexpr uint8_t numValue2{ 3u };
    constexpr bool isNewVariable2{ false };
    AstNode::Ptr assignNode2 = CreateAssignNodeFromByteValue( varName1, isNewVariable2, numValue2 );

    // Create third assign statement which reads from the first.
    std::string varName2 = "bar";
    constexpr bool isNewVariable3{ true };
    AstNode::Ptr assignNode3 = CreateAssignNodeFromVar( varName2, isNewVariable3, varName1 );

    // Create node to hold all assignment statements.
    AstNode::Children children{ assignNode1, assignNode2, assignNode3 };
    AstNode::Ptr scopeNode = std::make_shared< AstNode >( NT::Block, children );

    constexpr size_t expectedNumEntries{ 2u };
    SymbolTable::Ptr table = GenerateTableAndValidate( scopeNode, expectedNumEntries );

    constexpr bool expectReadFrom{ true };
    constexpr bool expectWrittenTo{ true };
    CheckForByteEntry( table, varName1, expectReadFrom, expectWrittenTo );

    constexpr bool expectReadFrom2{ false };
    constexpr bool expectWrittenTo2{ false };
    CheckForByteEntry( table, varName2, expectReadFrom2, expectWrittenTo2 );
}

BOOST_AUTO_TEST_SUITE_END() // SingleScopeTests

BOOST_AUTO_TEST_SUITE( MultiScopeTests )

/**
 * Tests that if a call is made to generate a symbol table for an AST that contains a scope-defining subtree, a new
 * child symbol table is assigned to that node.
 */
BOOST_AUTO_TEST_CASE( NestedScopeCreatesNewTable )
{
    // Create fake child scope-defining node to test a new table is created.
    constexpr TokenType scopeDefiningTokenType{ TokenType::WHILE };
    Token::Ptr fakeToken = std::make_shared< Token >( TokenType::INVALID_TOKEN );
    AstNode::Ptr fakeTokenWrapper = std::make_shared< AstNode >( TokenType::AND, fakeToken );
    AstNode::Children scopeChildren{ fakeTokenWrapper };
    AstNode::Ptr childNode = std::make_shared< AstNode >( scopeDefiningTokenType, scopeChildren );

    AstNode::Children children{ childNode };
    AstNode::Ptr parentNode = std::make_shared< AstNode >( NT::Block, children );

    constexpr size_t expectedNumEntries{ 0u };
    SymbolTable::Ptr parentTable = GenerateTableAndValidate( parentNode, expectedNumEntries );

    SymbolTable::Ptr childTable = childNode->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, childTable );
    BOOST_CHECK_EQUAL( 0u, childTable->GetNumEntries() );
}

/**
 * Tests that when generating symbol tables across more than one scope, if a variable is read from/written to in a
 * nested scope, the symbol table entry is still edited accordingly.
 */
BOOST_AUTO_TEST_CASE( VarReferencesInNestedScope )
{
    // Declaration statement for the parent scope
    std::string varName = "foo";
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    AstNode::Ptr declarationNode = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    // Assign to the var name, this statement goes in the child scope
    constexpr uint8_t numValue2{ 5u };
    constexpr bool isNewVariable2{ false };
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, isNewVariable2, numValue2 );

    constexpr TokenType scopeDefiningTokenType{ TokenType::IF };
    AstNode::Children childScopeChildren{ assignNode };
    AstNode::Ptr childScope = std::make_shared< AstNode >( scopeDefiningTokenType, childScopeChildren );

    AstNode::Children parentScopeChildren{ declarationNode, childScope };
    AstNode::Ptr parentScope = std::make_shared< AstNode >( NT::Block, parentScopeChildren );

    constexpr size_t expectedNumParentEntries{ 1u };
    SymbolTable::Ptr parentTable = GenerateTableAndValidate( parentScope, expectedNumParentEntries );

    constexpr bool expectReadFrom{ false };
    constexpr bool expectWrittenTo{ true };
    CheckForByteEntry( parentTable, varName, expectReadFrom, expectWrittenTo );

    SymbolTable::Ptr childTable = childScope->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, childTable );
    BOOST_CHECK_EQUAL( 0u, childTable->GetNumEntries() );
}

/**
 * Tests that when generating symbol tables for two sibling scopes, two separate entries are made for the same
 * identifier name, and references are not considered cross-scopes.
 */
BOOST_AUTO_TEST_CASE( SiblingScopesSameIdentifier )
{
    std::string varName = "foo";

    constexpr TokenType scopeDefiningTokenType{ TokenType::FOR };

    // Scope 1:
    // Declare identifier and then write to it.
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    AstNode::Ptr declarationNode = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    constexpr uint8_t numValue2{ 3u };
    constexpr bool isNewVariable2{ false };
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, isNewVariable2, numValue2 );

    AstNode::Children scope1Children{ declarationNode, assignNode };
    AstNode::Ptr scope1 = std::make_shared< AstNode >( scopeDefiningTokenType, scope1Children );

    // Scope 2:
    // Declare identifier, and do not reference it.
    constexpr uint8_t numValue3{ 1u };
    constexpr bool isNewVariable3{ true };
    AstNode::Ptr declarationNode2 = CreateAssignNodeFromByteValue( varName, isNewVariable3, numValue3 );

    AstNode::Children scope2Children{ declarationNode2 };
    AstNode::Ptr scope2 = std::make_shared< AstNode >( scopeDefiningTokenType, scope2Children );

    // Parent scope containing both sibling scopes
    AstNode::Children parentChildren{ scope1, scope2 };
    AstNode::Ptr parentScope = std::make_shared< AstNode >( NT::Block, parentChildren );

    constexpr size_t expectedNumParentEntries{ 0u };
    SymbolTable::Ptr parentTable = GenerateTableAndValidate( parentScope, expectedNumParentEntries );

    // Scope 1 table
    SymbolTable::Ptr table1 = scope1->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, table1 );
    BOOST_CHECK_EQUAL( 1u, table1->GetNumEntries() );
    constexpr bool expectReadFrom{ false };
    constexpr bool expectWrittenTo{ true };
    CheckForByteEntry( table1, varName, expectReadFrom, expectWrittenTo );

    // Scope 2 table
    SymbolTable::Ptr table2 = scope2->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, table2 );
    BOOST_CHECK_EQUAL( 1u, table2->GetNumEntries() );
    constexpr bool expectReadFrom2{ false };
    constexpr bool expectWrittenTo2{ false };
    CheckForByteEntry( table2, varName, expectReadFrom2, expectWrittenTo2 );
}

/**
 * Tests that when generating symbol tables for a doubly-nested scope (i.e. grandchild scope), a table is created and
 * populated correctly for each AST node.
 */
BOOST_AUTO_TEST_CASE( GrandchildNestedScope )
{
    std::string varName = "foo";

    // In the parent scope: declare identifier
    constexpr uint8_t numValue{ 5u };
    constexpr bool isNewVariable{ true };
    AstNode::Ptr declarationNode = CreateAssignNodeFromByteValue( varName, isNewVariable, numValue );

    // In the grandchild scope, write to identifier
    constexpr uint8_t numValue2{ 3u };
    constexpr bool isNewVariable2{ false };
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, isNewVariable2, numValue2 );

    // Populate scopes
    AstNode::Children grandchildScopeChildren{ assignNode };
    constexpr TokenType scopeDefiningTokenType1{ TokenType::FOR };
    AstNode::Ptr grandchildScope = std::make_shared< AstNode >( scopeDefiningTokenType1, grandchildScopeChildren );

    AstNode::Children childScopeChildren{ grandchildScope };
    constexpr TokenType scopeDefiningTokenType2{ TokenType::WHILE };
    AstNode::Ptr childScope = std::make_shared< AstNode >( scopeDefiningTokenType2, childScopeChildren );

    AstNode::Children parentScopeChildren{ declarationNode, childScope };
    constexpr TokenType scopeDefiningTokenType3{ TokenType::IF };
    AstNode::Ptr parentScope = std::make_shared< AstNode >( scopeDefiningTokenType3, parentScopeChildren );

    // Expect the entry to belong to the parent table as it was declared in that scope.
    constexpr size_t expectedNumParentEntries{ 1u };
    SymbolTable::Ptr parentTable = GenerateTableAndValidate( parentScope, expectedNumParentEntries );

    // Validate parent table
    constexpr bool expectReadFrom{ false };
    constexpr bool expectWrittenTo{ true };
    CheckForByteEntry( parentTable, varName, expectReadFrom, expectWrittenTo );

    // Validate child table
    SymbolTable::Ptr childTable = childScope->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, childTable );
    BOOST_CHECK_EQUAL( 0u, childTable->GetNumEntries() );

    // Validate grandchild table
    SymbolTable::Ptr grandchildTable = grandchildScope->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, grandchildTable );
    BOOST_CHECK_EQUAL( 0u, grandchildTable->GetNumEntries() );
}

BOOST_AUTO_TEST_SUITE_END() // MultiScopeTests

BOOST_AUTO_TEST_SUITE( RealExamples )

/**
 * Tests that symbol tables can be correctly generated for a small example program starting with symbol "Block", and
 * containing a while loop with a scope.
 */
BOOST_AUTO_TEST_CASE( StartingSymbolBlock_WhileLoop )
{
    /**
     * Create program with a while loop (condition = 1), with an inside-scope var declaration, and an outside-scope
     * var declaration.
     */

    // Create first declaration statement.
    std::string insideScopeVar = "insideScope";
    constexpr uint8_t insideScopeValue{ 5u };
    constexpr bool insideScope_IsNewVariable{ true };
    AstNode::Ptr insideScopeAssign = CreateAssignNodeFromByteValue( insideScopeVar,
                                                                    insideScopeValue,
                                                                    insideScope_IsNewVariable );

    Token::Ptr conditionByteToken = std::make_shared< Token >( TokenType::BYTE, 1u );
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( TokenType::BYTE, conditionByteToken );

    AstNode::Children whileChildren = { conditionNode, insideScopeAssign };
    AstNode::Ptr whileNode = std::make_shared< AstNode >( TokenType::WHILE, whileChildren );

    // Create second, outside-scope declaration statement.
    std::string outsideScopeVar = "outsideScope";
    constexpr uint8_t outsideScopeValue{ 10u };
    constexpr bool outsideScope_IsNewVariable{ true };
    AstNode::Ptr outsideScopeAssign = CreateAssignNodeFromByteValue( outsideScopeVar,
                                                                     outsideScopeValue,
                                                                     outsideScope_IsNewVariable );

    AstNode::Children blockChildren{ whileNode, outsideScopeAssign};
    AstNode::Ptr blockNode = std::make_shared< AstNode >( NT::Block, blockChildren );

    constexpr size_t expectedBlockTableSize{ 1u };
    SymbolTable::Ptr blockSt = GenerateTableAndValidate( blockNode, expectedBlockTableSize );
    CheckForByteEntry( blockSt, outsideScopeVar, false, false );

    SymbolTable::Ptr whileSt = whileNode->m_symbolTable;
    BOOST_REQUIRE_NE( nullptr, whileSt );
    BOOST_REQUIRE_EQUAL( 1u, whileSt->GetNumEntries() );
    CheckForByteEntry( whileSt, insideScopeVar, false, false );
}

BOOST_AUTO_TEST_SUITE_END() // RealExamples

BOOST_AUTO_TEST_SUITE_END() // SymbolTableGeneratorTests