#include <boost/test/unit_test.hpp>
#include "AstGenerator.h"

class AstGeneratorTestsFixture
{
public:
    AstGeneratorTestsFixture() = default;

    /**
     * \brief  Checks AST node is wrapper node around token - checks the node label is
     *         the token type, and checks it is storing the given token.
     */
    void
    CheckNodeIsTokenWrapper( AstNode::Ptr node, Token::Ptr token )
    {
        BOOST_CHECK_EQUAL( token->m_type, node->m_nodeLabel );
        BOOST_REQUIRE( node->IsStoringToken() );
        BOOST_REQUIRE( node->IsStorageInUse() );
        Token::Ptr nodeToken = node->GetToken();
        BOOST_CHECK( *token.get() == *nodeToken.get() );
    }
};

BOOST_FIXTURE_TEST_SUITE( AstGeneratorTests, AstGeneratorTestsFixture )

/**
 * Tests that if GenerateAst is called with an empty tokens container, it throws an error.
 */
BOOST_AUTO_TEST_CASE( EmptyTokens )
{
    Tokens tokens{};
    constexpr GrammarSymbols::NT startingNt { NT::Block };
    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );
    BOOST_CHECK_THROW( astGenerator->GenerateAst(), std::invalid_argument );
}

/**
 * Tests that if GenerateAst is called with a starting NT that is unrecognised/doesn't map to any rules, it throws
 * an error.
 */
BOOST_AUTO_TEST_CASE( StartingNtHasNoRules )
{
    // Arbitrary token to avoid the empty tokens error
    Tokens tokens{ std::make_shared< Token >( TokenType::AND ) };
    // Out of range value
    constexpr GrammarSymbols::NT startingNt { static_cast< NT >( SymbolType::NonTerminal + 1000u ) };
    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );
    BOOST_CHECK_THROW( astGenerator->GenerateAst(), std::invalid_argument );
}

/**
 * Tests that when GenerateAst is called with tokens that don't match any of the rules associated with the starting
 * non-terminal, the method returns nullptr.
 */
BOOST_AUTO_TEST_CASE( NoMatchesForAnyRules )
{
    // Variable can either resolve to data type + id, or just id
    constexpr GrammarSymbols::NT startingNt { Variable };
    // Arbitrary token - doesn't match above rules
    Tokens tokens{ std::make_shared< Token >( TokenType::AND ) };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    BOOST_CHECK_EQUAL( nullptr, astGenerator->GenerateAst() );
}

/**
 * Tests that when GenerateAst is called with a rule match of a single terminal symbol, it returns a successful
 * match in the form of an AST node.
 */
BOOST_AUTO_TEST_CASE( MatchesRule_SingleTerminal )
{
    // Variable can either resolve to data type + id, or just id
    constexpr GrammarSymbols::NT startingNt { Variable };
    // A single ID token should match one of the rules for Variable
    const std::string tokenString = "hello";
    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString );
    Tokens tokens{ idToken };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check the returned node is a wrapper around the given token
    CheckNodeIsTokenWrapper( returnedNode, idToken );
}

/**
 * Tests that when GenerateAst is called with a high-level rule of which one of its descendants match a single
 * terminal symbol, it has the same behaviour as simply being called from the parent rule symbol.
 */
BOOST_AUTO_TEST_CASE( HighLevelRule_SingleTerminalMatch )
{
    // Use high level rule - it has to traverse through various descendants to find a match.
    constexpr GrammarSymbols::NT startingNt { Logical };
    // A single ID token should match one of the rules for Variable, or Factor
    const std::string tokenString = "hello";
    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString );
    Tokens tokens{ idToken };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check the returned node is a wrapper around the given token
    CheckNodeIsTokenWrapper( returnedNode, idToken );
}

/**
 * Tests that when GenerateAst is called with a NT that has a rule with a single non-terminal symbol,
 * it returns the node representing that non-terminal rule instead.
 */
BOOST_AUTO_TEST_CASE( MatchesRule_SingleNonTerminal )
{
    // One of the rules for Term is "Factor"
    // "Factor" then expands to "ID", which can be resolved by a single ID token.
    constexpr GrammarSymbols::NT startingNt { Term };

    // A single ID token should match one of the rules for Factor
    const std::string tokenString = "hello";
    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString );
    Tokens tokens{ idToken };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Expect the wrapper around the ID token to be returned, as there are no other symbols/child symbols in the rule.
    CheckNodeIsTokenWrapper( returnedNode, idToken );
}

/**
 * Tests that when GenerateAst is called with a NT that has a rule with multiple symbols,
 * it returns the node with an appropriate label, and children containing the sub-trees belonging to
 * the non-terminals, as well as wrapper nodes around any tokens.
 */
BOOST_AUTO_TEST_CASE( MatchesRule_MultipleMixedSymbols )
{
    // One of the rules for Term is "Factor"
    // "Factor" then expands to "ID", which can be resolved by a single ID token.
    constexpr GrammarSymbols::NT startingNt { Term };

    // A single ID token should match one of the rules for Factor
    const std::string tokenString1 = "hello";
    Token::Ptr idToken1 = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString1 );
    const std::string tokenString2 = "hello2";
    Token::Ptr idToken2 = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString2 );

    Token::Ptr expToken = std::make_shared< Token >( TokenType::MULTIPLY );

    // The set of tokens should satisfy the rule "Factor MULTIPLY Factor"
    Tokens tokens{ idToken1, expToken, idToken2 };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check the returned node is as expected:
    // - With label MULTIPLY
    // - Storing two child nodes, each containing the ID tokens
    BOOST_CHECK_EQUAL( TokenType::MULTIPLY, returnedNode->m_nodeLabel );

    BOOST_CHECK( returnedNode->IsStorageInUse() );
    BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children children = returnedNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, children.size() );

    // Check first child holds the first ID token
    AstNode::Ptr child1 = children[0];
    CheckNodeIsTokenWrapper( child1, idToken1 );

    // Check second child holds the second ID token
    AstNode::Ptr child2 = children[1];
    CheckNodeIsTokenWrapper( child2, idToken2 );
}

/**
 * Tests that when GenerateAst is called with a NT that has a matching rule, but with leftover excess
 * tokens, it will return nullptr.
 */
BOOST_AUTO_TEST_CASE( MatchesRule_LeftoverTokens_NotAllowed )
{
    // Variable can either resolve to data type + id, or just id
    constexpr GrammarSymbols::NT startingNt { Variable };
    // A single ID token should match one of the rules for Variable
    const std::string tokenString = "hello";
    Token::Ptr idToken = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString );

    Token::Ptr excessToken = std::make_shared< Token >( TokenType::MOD );
    Token::Ptr excessToken1 = std::make_shared< Token >( TokenType::FOR );
    // Tokens contains a match followed by leftover token
    Tokens tokens{ idToken, excessToken, excessToken1 };
    size_t originalTokensSize = tokens.size();

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    // Expect non-successful result
    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_CHECK_EQUAL( nullptr, returnedNode );
}

/**
 * Tests that when GenerateAst is called with a NT that has a matching rule with a non-terminal as its last symbol,
 * but with leftover excess tokens, it will return nullptr. This test is to check that the same restriction is applied
 * to the non-terminal as it creates its own sub-tree.
 */
BOOST_AUTO_TEST_CASE( MatchesRule_LeftoverTokens_NotAllowed_NtLastSymbol )
{
    // One of the rules for Term is "Factor"
    // "Factor" then expands to "ID", which can be resolved by a single ID token.
    constexpr GrammarSymbols::NT startingNt { Term };

    // A single ID token should match one of the rules for Factor
    const std::string tokenString1 = "hello";
    Token::Ptr idToken1 = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString1 );
    const std::string tokenString2 = "hello2";
    Token::Ptr idToken2 = std::make_shared< Token >( TokenType::IDENTIFIER, tokenString2 );

    Token::Ptr expToken = std::make_shared< Token >( TokenType::MULTIPLY );

    // Excess leftover tokens
    Token::Ptr excessToken = std::make_shared< Token >( TokenType::MOD );
    Token::Ptr excessToken1 = std::make_shared< Token >( TokenType::FOR );

    // The set of tokens should satisfy the rule "Factor MULTIPLY Factor", with leftover tokens
    // at the end
    Tokens tokens{ idToken1, expToken, idToken2, excessToken, excessToken1 };
    size_t originalTokensSize = tokens.size();

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    // Expect non-successful result
    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_CHECK_EQUAL( nullptr, returnedNode );
}

/**
 * Tests that GenerateAst can handle a complex set of tokens, representing an AST with multiple sub-trees.
 * In this example use a while loop.
 */
BOOST_AUTO_TEST_CASE( MultipleSubTrees_WhileLoop )
{
    // Use tokens to represent the following:
    // while ( 1 ) { byte varName = 0; };
    Token::Ptr whileToken = std::make_shared< Token >( TokenType::WHILE );
    Token::Ptr parenOpenToken = std::make_shared< Token >( TokenType::PAREN_OPEN );
    Token::Ptr oneToken = std::make_shared< Token >( TokenType::BYTE, 1u );
    Token::Ptr parenCloseToken = std::make_shared< Token >( TokenType::PAREN_CLOSE);
    Token::Ptr braceOpenToken = std::make_shared< Token >( TokenType::BRACE_OPEN);
    Token::Ptr byteToken = std::make_shared< Token >( TokenType::DATA_TYPE, DataType::DT_BYTE );
    Token::Ptr varToken = std::make_shared< Token >( TokenType::IDENTIFIER, "varName" );
    Token::Ptr assignToken = std::make_shared< Token >( TokenType::ASSIGN );
    Token::Ptr zeroToken = std::make_shared< Token >( TokenType::BYTE, 0u );
    Token::Ptr semiColonToken1 = std::make_shared< Token >( TokenType::SEMICOLON );
    Token::Ptr braceCloseToken = std::make_shared< Token >( TokenType::BRACE_CLOSE);
    Token::Ptr semiColonToken2 = std::make_shared< Token >( TokenType::SEMICOLON );

    Tokens tokens = { whileToken, parenOpenToken, oneToken, parenCloseToken, braceOpenToken, byteToken,
                      varToken, assignToken, zeroToken, semiColonToken1, braceCloseToken, semiColonToken2 };

    constexpr GrammarSymbols::NT startingNt{ NT::Block };
    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check node label is WHILE
    BOOST_CHECK_EQUAL( TokenType::WHILE, returnedNode->m_nodeLabel );
    // Check is storing 2 children
    BOOST_CHECK( returnedNode->IsStorageInUse() );
    BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children children = returnedNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, children.size() );

    // Expect first child to be storing a BYTE token
    AstNode::Ptr child1 = children[0];
    CheckNodeIsTokenWrapper( child1, oneToken );

    // Expect second child to have label ASSIGN, and holding 2 child nodes
    AstNode::Ptr assignNode = children[1];
    BOOST_CHECK_EQUAL( TokenType::ASSIGN, assignNode->m_nodeLabel );
    BOOST_CHECK( assignNode->IsStorageInUse() );
    BOOST_CHECK( !assignNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children assignChildren = assignNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, assignChildren.size() );

    // Expect first child of the assign node to be a Variable node, holding 2 wrapper nodes around the
    // data type and the identifier
    AstNode::Ptr variableNode = assignChildren[0];
    BOOST_CHECK_EQUAL( NT::Variable, variableNode->m_nodeLabel );
    BOOST_CHECK( variableNode->IsStorageInUse() );
    BOOST_CHECK( !variableNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children variableChildren = variableNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, variableChildren.size() );

    AstNode::Ptr variableChild1 = variableChildren[0];
    CheckNodeIsTokenWrapper( variableChild1, byteToken );
    AstNode::Ptr variableChild2 = variableChildren[1];
    CheckNodeIsTokenWrapper( variableChild2, varToken );

    // Expect second child of the assign node to be a wrapper node around the byte value of 0
    AstNode::Ptr byteNode = assignChildren[1];
    CheckNodeIsTokenWrapper( byteNode, zeroToken );
}

/**
 * Tests that GenerateAst can handle a complex set of tokens, representing an AST with multiple sub-trees.
 * In this example use an if-else statement.
 */
BOOST_AUTO_TEST_CASE( MultipleSubTrees_IfElse )
{
    // Use tokens to represent the following:
    // if ( 1 ) { varName = 0; } else { varName = 0; };
    Token::Ptr ifToken = std::make_shared< Token >( TokenType::IF );

    Token::Ptr parenOpenToken = std::make_shared< Token >( TokenType::PAREN_OPEN );
    Token::Ptr oneToken = std::make_shared< Token >( TokenType::BYTE, 1u );
    Token::Ptr parenCloseToken = std::make_shared< Token >( TokenType::PAREN_CLOSE);

    Token::Ptr braceOpenToken = std::make_shared< Token >( TokenType::BRACE_OPEN);
    Token::Ptr varToken = std::make_shared< Token >( TokenType::IDENTIFIER, "varName" );
    Token::Ptr assignToken = std::make_shared< Token >( TokenType::ASSIGN );
    Token::Ptr zeroToken = std::make_shared< Token >( TokenType::BYTE, 0u );
    Token::Ptr semiColonToken = std::make_shared< Token >( TokenType::SEMICOLON );
    Token::Ptr braceCloseToken = std::make_shared< Token >( TokenType::BRACE_CLOSE);

    Token::Ptr elseToken = std::make_shared< Token >( TokenType::ELSE );

    Token::Ptr braceOpenToken2 = std::make_shared< Token >( TokenType::BRACE_OPEN );
    Token::Ptr varToken2 = std::make_shared< Token >( TokenType::IDENTIFIER, "varName" );
    Token::Ptr assignToken2 = std::make_shared< Token >( TokenType::ASSIGN );
    Token::Ptr zeroToken2 = std::make_shared< Token >( TokenType::BYTE, 0u );
    Token::Ptr semiColonToken2 = std::make_shared< Token >( TokenType::SEMICOLON );
    Token::Ptr braceCloseToken2 = std::make_shared< Token >( TokenType::BRACE_CLOSE );

    Token::Ptr semiColonToken3 = std::make_shared< Token >( TokenType::SEMICOLON );

    Tokens tokens = {
        ifToken,
        parenOpenToken,
        oneToken,
        parenCloseToken,
        braceOpenToken,
        varToken,
        assignToken,
        zeroToken,
        semiColonToken,
        braceCloseToken,
        elseToken,
        braceOpenToken2,
        varToken2,
        assignToken2,
        zeroToken2,
        semiColonToken2,
        braceCloseToken2,
        semiColonToken3
    };

    constexpr GrammarSymbols::NT startingNt{ NT::Block };
    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check node label is IF
    BOOST_CHECK_EQUAL( TokenType::IF, returnedNode->m_nodeLabel );
    // Check is storing 3 children
    BOOST_CHECK( returnedNode->IsStorageInUse() );
    BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children children = returnedNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 3u, children.size() );

    // Expect first child to be storing a BYTE token
    AstNode::Ptr child1 = children[0];
    CheckNodeIsTokenWrapper( child1, oneToken );

    // Expect second child to have label ASSIGN, and holding 2 child nodes
    {
        AstNode::Ptr assignNode = children[1];
        BOOST_CHECK_EQUAL( TokenType::ASSIGN, assignNode->m_nodeLabel );
        BOOST_CHECK( assignNode->IsStorageInUse() );
        BOOST_CHECK( !assignNode->IsStoringToken() ); // Expect it to be storing children
        AstNode::Children assignChildren = assignNode->GetChildren();
        BOOST_REQUIRE_EQUAL( 2u, assignChildren.size() );

        // Expect first child of the assign node to be an ID node, holding 2 wrapper nodes around the
        // data type and the identifier
        AstNode::Ptr idNode = assignChildren[0];
        CheckNodeIsTokenWrapper( idNode, varToken );

        // Expect second child of the assign node to be a wrapper node around the byte value of 0
        AstNode::Ptr byteNode = assignChildren[1];
        CheckNodeIsTokenWrapper( byteNode, zeroToken );
    }

    // Expect third child to be an ELSE node, with an assign child node
    {
        AstNode::Ptr elseNode = children[2];
        BOOST_CHECK_EQUAL( TokenType::ELSE, elseNode->m_nodeLabel );
        BOOST_CHECK( elseNode->IsStorageInUse() );
        BOOST_CHECK( !elseNode->IsStoringToken() ); // Expect it to be storing children
        AstNode::Children elseChildren = elseNode->GetChildren();
        BOOST_REQUIRE_EQUAL( 1u, elseChildren.size() );

        {
            AstNode::Ptr assignNode = elseChildren[0];
            BOOST_CHECK_EQUAL( TokenType::ASSIGN, assignNode->m_nodeLabel );
            BOOST_CHECK( assignNode->IsStorageInUse() );
            BOOST_CHECK( !assignNode->IsStoringToken() ); // Expect it to be storing children
            AstNode::Children assignChildren = assignNode->GetChildren();
            BOOST_REQUIRE_EQUAL( 2u, assignChildren.size() );

            // Expect first child of the assign node to be an ID node, holding 2 wrapper nodes around the
            // data type and the identifier
            AstNode::Ptr idNode = assignChildren[0];
            CheckNodeIsTokenWrapper( idNode, varToken );

            // Expect second child of the assign node to be a wrapper node around the byte value of 0
            AstNode::Ptr byteNode = assignChildren[1];
            CheckNodeIsTokenWrapper( byteNode, zeroToken );
        }
    }
}

/**
 * Tests that operators are parsed correctly regarding order, and parentheses. The current expected behaviour is as
 * follows:
 *
 * - If more than one operator is used, order of operations should be used, and no parentheses should be required.
 *   - A + B * C should result in a plus node, holding A and a multiply node as its children.
 * - If parentheses are used, these should take priority over the order of operations.
 *   - (A + B) * C should result in a multiply node, holding a plus node and C as its children.
 * - The current unresolved behaviour in the grammar is if two operators at the same level (e.g. + and -, == and ==).
 *   - Typical behaviour would be to read from left to right but the grammar currently doesn't support this. Therefore
 *     parentheses should be used to define order of operations - without parentheses present in this scenario, the
 *     parsing operation should fail.
 */
BOOST_AUTO_TEST_SUITE( OrderOfOperationsTests )

/**
 * Tests that when GenerateAst is called on an expression containing multiple operators without any parentheses, it
 * resolves the order of operations correctly when creating sub-trees.
 */
BOOST_AUTO_TEST_CASE( MultipleOperators_NoParentheses )
{
    // Create tokens for a Logical expression
    Token::Ptr byte1 = std::make_shared< Token >( TokenType::BYTE, 1 );
    Token::Ptr plus = std::make_shared< Token >( TokenType::PLUS );
    Token::Ptr byte2 = std::make_shared< Token >( TokenType::BYTE, 2 );
    Token::Ptr multiply = std::make_shared< Token >( TokenType::MULTIPLY );
    Token::Ptr byte3 = std::make_shared< Token >( TokenType::BYTE, 3 );

    Tokens tokens{
        byte1,
        plus,
        byte2,
        multiply,
        byte3,
    };

    constexpr GrammarSymbols::NT startingNt { Logical };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check the returned node is as expected:
    // - With label PLUS
    // - Storing two child nodes, one for the byte 1, and another for the multiply sub-tree
    BOOST_CHECK_EQUAL( TokenType::PLUS, returnedNode->m_nodeLabel );

    BOOST_CHECK( returnedNode->IsStorageInUse() );
    BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children children = returnedNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, children.size() );

    // Check first child holds the first byte token
    AstNode::Ptr child1 = children[0];
    CheckNodeIsTokenWrapper( child1, byte1 );

    // Check second child holds a subtree for the multiply operator
    AstNode::Ptr child2 = children[1];
    BOOST_CHECK_EQUAL( TokenType::MULTIPLY, child2->m_nodeLabel );
    BOOST_CHECK( child2->IsStorageInUse() );
    BOOST_CHECK( !child2->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children multiplyChildren = child2->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, multiplyChildren.size() );

    AstNode::Ptr multiplyChild1 = multiplyChildren[0];
    CheckNodeIsTokenWrapper( multiplyChild1, byte2 );
    AstNode::Ptr multiplyChild2 = multiplyChildren[1];
    CheckNodeIsTokenWrapper( multiplyChild2, byte3 );
}

/**
 * Tests that when GenerateAst is called on an expression containing multiple operators and parentheses that define
 * a certain order of operations, the created sub-trees reflect this.
 */
BOOST_AUTO_TEST_CASE( MultipleOperators_ParenthesesSetNewOrder )
{
    // Create tokens for a Logical expression
    Token::Ptr parenOpen = std::make_shared< Token >( TokenType::PAREN_OPEN );
    Token::Ptr byte1 = std::make_shared< Token >( TokenType::BYTE, 1 );
    Token::Ptr plus = std::make_shared< Token >( TokenType::PLUS );
    Token::Ptr byte2 = std::make_shared< Token >( TokenType::BYTE, 2 );
    Token::Ptr parenClose = std::make_shared< Token >( TokenType::PAREN_CLOSE );
    Token::Ptr multiply = std::make_shared< Token >( TokenType::MULTIPLY );
    Token::Ptr byte3 = std::make_shared< Token >( TokenType::BYTE, 3 );

    Tokens tokens{
        parenOpen,
        byte1,
        plus,
        byte2,
        parenClose,
        multiply,
        byte3,
    };

    constexpr GrammarSymbols::NT startingNt { Logical };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_REQUIRE_NE( nullptr, returnedNode );

    // Check the returned node is as expected:
    // - With label MULTIPLY
    // - Storing two child nodes, one for the plus node, and another for byte 3
    BOOST_CHECK_EQUAL( TokenType::MULTIPLY, returnedNode->m_nodeLabel );

    BOOST_CHECK( returnedNode->IsStorageInUse() );
    BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children children = returnedNode->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, children.size() );

    // Check first child holds a subtree for the plus operator
    AstNode::Ptr child1 = children[0];
    BOOST_CHECK_EQUAL( TokenType::PLUS, child1->m_nodeLabel );
    BOOST_CHECK( child1->IsStorageInUse() );
    BOOST_CHECK( !child1->IsStoringToken() ); // Expect it to be storing children
    AstNode::Children plusChildren = child1->GetChildren();
    BOOST_REQUIRE_EQUAL( 2u, plusChildren.size() );

    AstNode::Ptr plusChild1 = plusChildren[0];
    CheckNodeIsTokenWrapper( plusChild1, byte1 );
    AstNode::Ptr plusChild2 = plusChildren[1];
    CheckNodeIsTokenWrapper( plusChild2, byte2 );

    // Check second child holds the third byte token
    AstNode::Ptr child2 = children[1];
    CheckNodeIsTokenWrapper( child2, byte3 );
}

/**
 * Tests that when GenerateAst is called on an expression containing multiple operators at the same level, but no
 * parentheses, it fails and returns an error.
 */
BOOST_AUTO_TEST_CASE( MultipleOperatorsAtSameLevel_NoParentheses )
{
    // Create tokens for a Logical expression
    Token::Ptr byte1 = std::make_shared< Token >( TokenType::BYTE, 1 );
    Token::Ptr plus = std::make_shared< Token >( TokenType::PLUS );
    Token::Ptr byte2 = std::make_shared< Token >( TokenType::BYTE, 2 );
    Token::Ptr minus = std::make_shared< Token >( TokenType::MINUS );
    Token::Ptr byte3 = std::make_shared< Token >( TokenType::BYTE, 3 );

    Tokens tokens{
        byte1,
        plus,
        byte2,
        minus,
        byte3,
    };

    constexpr GrammarSymbols::NT startingNt { Logical };

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

    AstNode::Ptr returnedNode = astGenerator->GenerateAst();
    BOOST_CHECK_EQUAL( nullptr, returnedNode );
}

/**
 * Tests that when GenerateAst is called on an expression containing multiple operators at the same level with
 * parentheses, the created sub-trees reflect the specified order.
 */
 BOOST_AUTO_TEST_CASE( MultipleOperatorsAtSameLevel_Parentheses )
 {
     // Create tokens for a Logical expression
     Token::Ptr parenOpen = std::make_shared< Token >( TokenType::PAREN_OPEN );
     Token::Ptr byte1 = std::make_shared< Token >( TokenType::BYTE, 1 );
     Token::Ptr plus = std::make_shared< Token >( TokenType::PLUS );
     Token::Ptr byte2 = std::make_shared< Token >( TokenType::BYTE, 2 );
     Token::Ptr parenClose = std::make_shared< Token >( TokenType::PAREN_CLOSE );
     Token::Ptr minus = std::make_shared< Token >( TokenType::MINUS );
     Token::Ptr byte3 = std::make_shared< Token >( TokenType::BYTE, 3 );

     Tokens tokens{
         parenOpen,
         byte1,
         plus,
         byte2,
         parenClose,
         minus,
         byte3,
     };

     constexpr GrammarSymbols::NT startingNt { Logical };

     AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >( tokens, startingNt );

     AstNode::Ptr returnedNode = astGenerator->GenerateAst();
     BOOST_REQUIRE_NE( nullptr, returnedNode );

     // Check the returned node is as expected:
     // - With label MINUS
     // - Storing two child nodes, one for the plus node, and another for byte 3
     BOOST_CHECK_EQUAL( TokenType::MINUS, returnedNode->m_nodeLabel );

     BOOST_CHECK( returnedNode->IsStorageInUse() );
     BOOST_CHECK( !returnedNode->IsStoringToken() ); // Expect it to be storing children
     AstNode::Children children = returnedNode->GetChildren();
     BOOST_REQUIRE_EQUAL( 2u, children.size() );

     // Check first child holds a subtree for the plus operator
     AstNode::Ptr child1 = children[0];
     BOOST_CHECK_EQUAL( TokenType::PLUS, child1->m_nodeLabel );
     BOOST_CHECK( child1->IsStorageInUse() );
     BOOST_CHECK( !child1->IsStoringToken() ); // Expect it to be storing children
     AstNode::Children plusChildren = child1->GetChildren();
     BOOST_REQUIRE_EQUAL( 2u, plusChildren.size() );

     AstNode::Ptr plusChild1 = plusChildren[0];
     CheckNodeIsTokenWrapper( plusChild1, byte1 );
     AstNode::Ptr plusChild2 = plusChildren[1];
     CheckNodeIsTokenWrapper( plusChild2, byte2 );

     // Check second child holds the third byte token
     AstNode::Ptr child2 = children[1];
     CheckNodeIsTokenWrapper( child2, byte3 );
 }

BOOST_AUTO_TEST_SUITE_END() // OrderOfOperationsTests

BOOST_AUTO_TEST_SUITE_END() // AstGeneratorTests