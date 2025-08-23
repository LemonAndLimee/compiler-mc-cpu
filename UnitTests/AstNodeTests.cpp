#include <boost/test/unit_test.hpp>
#include "AstNode.h"

class AstNodeTestsFixture
{
public:
    AstNodeTestsFixture() = default;

    /**
     * \brief  Creates fake AST node with an arbitrary terminal node label, and no children, to be used in test
     *         cases.
     *
     * \return  Created AST node.
     */
    AstNode::Ptr
    CreateFakeAstNode()
    {
        GrammarSymbols::Symbol fakeLabel { T::PLUS };
        AstNode::Children fakeChildren;
        return std::make_shared< AstNode >( fakeLabel, fakeChildren );
    }

    /**
     * \brief  Checks AST node is storing a token, not children. Checks stored token is pointing to the
     *         same instance passed to this method.
     */
    void
    CheckNodeIsStoringToken( AstNode::Ptr node, Token::Ptr token )
    {
        BOOST_CHECK_EQUAL( true, node->IsStorageInUse() );
        BOOST_CHECK( node->IsStoringToken() );
        BOOST_CHECK_EQUAL( token.get(), node->GetToken().get() );
    }

    void
    CheckNodeIsStoringChildren( AstNode::Ptr node, size_t expectedNumChildren )
    {
        BOOST_CHECK_EQUAL( true, node->IsStorageInUse() );
        BOOST_CHECK( !node->IsStoringToken() );
        BOOST_CHECK_EQUAL( expectedNumChildren, node->GetChildren().size() );
    }
};

BOOST_FIXTURE_TEST_SUITE( AstNodeTests, AstNodeTestsFixture )

BOOST_AUTO_TEST_SUITE( GetNodeFromRuleElementsTests )

/**
 * Tests that when GetNodeFromRuleElements is called with an empty elements container, it throws an exception.
 */
BOOST_AUTO_TEST_CASE( EmptyElements )
{
    GrammarSymbols::NT nonTerminalArg { Block };
    AstNode::Elements elements{};
    BOOST_CHECK_THROW( AstNode::GetNodeFromRuleElements( elements, nonTerminalArg ), std::runtime_error );
}

/**
 * Tests that when GetNodeFromRuleElements is called with a single element that is a node label terminal, it returns
 * an AST node with no children, and this token as the node label.
 */
BOOST_AUTO_TEST_CASE( SingleNodeLabelTypeTerminal )
{
    TokenType nodeLabelTokenType = TokenType::IF;

    AstNode::Elements elements { std::make_shared< Token >( nodeLabelTokenType ) };
    GrammarSymbols::NT nonTerminalArg { Block };

    AstNode::Ptr returnedNode = AstNode::GetNodeFromRuleElements( elements, nonTerminalArg );
    BOOST_REQUIRE( nullptr != returnedNode );

    BOOST_CHECK_EQUAL( false, returnedNode->IsStorageInUse() );
    // Check the node label is a terminal symbol, and is equal to the type of the token passed to the method.
    BOOST_CHECK_EQUAL( SymbolType::Terminal, GetSymbolType( returnedNode->m_nodeLabel ) );
    BOOST_CHECK_EQUAL( nodeLabelTokenType, returnedNode->m_nodeLabel );
}

/**
 * Tests that when GetNodeFromRuleElements is called with only elements that are a skip-for-AST type terminal, it
 * skips and therefore has no elements, throwing an exception.
 */
BOOST_AUTO_TEST_CASE( OnlySkipTypeTerminals )
{
    // Create elements vector only consisting of skippable tokens.
    AstNode::Elements elements
    {
        std::make_shared< Token >( TokenType::PAREN_OPEN ),
        std::make_shared< Token >( TokenType::BRACE_CLOSE ),
        std::make_shared< Token >( TokenType::SEMICOLON )
    };

    GrammarSymbols::NT nonTerminalArg { Block };

    BOOST_CHECK_THROW( AstNode::GetNodeFromRuleElements( elements, nonTerminalArg ), std::runtime_error );
}

/**
 * Tests that when GetNodeFromRuleElements is called with a single terminal that is not a node label type,
 * or a skip type, it returns an AST node with this as its stored token, and the node label as the token's type.
 */
BOOST_AUTO_TEST_CASE( SingleNonNodeLabelTerminal )
{
    Token::Ptr token = std::make_shared< Token >( TokenType::IDENTIFIER, "variableName" );

    AstNode::Elements elements { token };
    GrammarSymbols::NT nonTerminalArg { Block };

    AstNode::Ptr returnedNode = AstNode::GetNodeFromRuleElements( elements, nonTerminalArg );
    BOOST_REQUIRE( nullptr != returnedNode );

    // Check node has our token as its stored token
    CheckNodeIsStoringToken( returnedNode, token );

    // Check the node label is a terminal symbol, and is equal to the token's type.
    BOOST_CHECK_EQUAL( SymbolType::Terminal, GetSymbolType( returnedNode->m_nodeLabel ) );
    BOOST_CHECK_EQUAL( token->m_type, returnedNode->m_nodeLabel );
}

/**
 * Tests that when GetNodeFromRuleElements is called with a single element that is another AST node, it returns this
 * node instead of creating one.
 */
BOOST_AUTO_TEST_CASE( SingleNonTerminal )
{
    // Create fake AST node to pass as an element
    AstNode::Ptr fakeAstNode = CreateFakeAstNode();

    AstNode::Elements elements { fakeAstNode };
    GrammarSymbols::NT nonTerminalArg { Block };
    AstNode::Ptr returnedNode = AstNode::GetNodeFromRuleElements( elements, nonTerminalArg );

    BOOST_REQUIRE( nullptr != returnedNode );

    // Check returned node is the same as the node we passed in elements
    BOOST_CHECK_EQUAL( fakeAstNode.get(), returnedNode.get() );
}

/**
 * Tests that when GetNodeFromRuleElements is called with multiple elements, of which one is a node label type, it
 * returns an AST node with this as the node label, and the given elements as children, with the node label token
 * removed.
 */
BOOST_AUTO_TEST_CASE( MultipleChildren_SingleTerminalNodeLabel )
{
    AstNode::Ptr fakeAstNode1 = CreateFakeAstNode();
    AstNode::Ptr fakeAstNode2 = CreateFakeAstNode();
    Token::Ptr nodeLabelToken = std::make_shared< Token >( TokenType::WHILE );
    Token::Ptr skipToken = std::make_shared< Token >( TokenType::BRACE_CLOSE );
    Token::Ptr regularToken = std::make_shared< Token >( TokenType::IDENTIFIER, "variableName" );

    AstNode::Elements elements
    {
        fakeAstNode1,
        fakeAstNode2,
        nodeLabelToken,
        skipToken,
        regularToken
    };

    GrammarSymbols::NT nonTerminalArg { Block };
    AstNode::Ptr returnedNode = AstNode::GetNodeFromRuleElements( elements, nonTerminalArg );
    BOOST_REQUIRE( nullptr != returnedNode );

    // Expect node label to be the node label token type
    BOOST_CHECK_EQUAL( nodeLabelToken->m_type, returnedNode->m_nodeLabel );

    // Expect the children to contain the created AST nodes + a wrapper AST node around the regular token
    constexpr size_t expectedChildrenSize{ 3u };
    CheckNodeIsStoringChildren( returnedNode, expectedChildrenSize );
    AstNode::Children returnedChildren = returnedNode->GetChildren();

    BOOST_CHECK_EQUAL( fakeAstNode1.get(), returnedChildren[0].get() );
    BOOST_CHECK_EQUAL( fakeAstNode2.get(), returnedChildren[1].get() );

    AstNode::Ptr expectedTokenWrapperChild = returnedChildren[2];
    CheckNodeIsStoringToken( expectedTokenWrapperChild, regularToken );
}

/**
 * Tests that when GetNodeFromRuleElements is called with multiple children, of which more than one are node label
 * types, it throws an exception.
 */
BOOST_AUTO_TEST_CASE( MultipleChildren_TwoNodeLabelTypes_Throws )
{
    AstNode::Ptr fakeAstNode1 = CreateFakeAstNode();
    AstNode::Ptr fakeAstNode2 = CreateFakeAstNode();
    Token::Ptr nodeLabelToken = std::make_shared< Token >( TokenType::WHILE );
    Token::Ptr skipToken = std::make_shared< Token >( TokenType::BRACE_CLOSE );
    Token::Ptr regularToken = std::make_shared< Token >( TokenType::IDENTIFIER, "variableName" );
    Token::Ptr nodeLabelToken2 = std::make_shared< Token >( TokenType::OR );

    AstNode::Elements elements
    {
        fakeAstNode1,
        fakeAstNode2,
        nodeLabelToken,
        skipToken,
        regularToken,
        nodeLabelToken2
    };

    GrammarSymbols::NT nonTerminalArg { Block };
    BOOST_CHECK_THROW( AstNode::GetNodeFromRuleElements( elements, nonTerminalArg ), std::runtime_error );
}

/**
 * Tests that when GetNodeFromRuleElements is called with multiple children, of which none are node label types, it
 * returns an AST node with all the given children, and the node label set to the provided NT argument.
 */
BOOST_AUTO_TEST_CASE( MultipleChildren_NoNodeLabel )
{
    AstNode::Ptr fakeAstNode1 = CreateFakeAstNode();
    AstNode::Ptr fakeAstNode2 = CreateFakeAstNode();
    Token::Ptr skipToken = std::make_shared< Token >( TokenType::BRACE_CLOSE );
    Token::Ptr regularToken = std::make_shared< Token >( TokenType::IDENTIFIER, "variableName" );

    AstNode::Elements elements
    {
        fakeAstNode1,
        fakeAstNode2,
        skipToken,
        regularToken
    };

    GrammarSymbols::NT nonTerminalArg { Block };
    AstNode::Ptr returnedNode = AstNode::GetNodeFromRuleElements( elements, nonTerminalArg );
    BOOST_REQUIRE( nullptr != returnedNode );

    // Expect node label to be the non-terminal symbol argument since it can't find a node label element.
    BOOST_CHECK_EQUAL( nonTerminalArg, returnedNode->m_nodeLabel );

    // Expect the children to contain the created AST nodes + a wrapper AST node around the regular token
    constexpr size_t expectedChildrenSize{ 3u };
    CheckNodeIsStoringChildren( returnedNode, expectedChildrenSize );
    AstNode::Children returnedChildren = returnedNode->GetChildren();

    BOOST_CHECK_EQUAL( fakeAstNode1.get(), returnedChildren[0].get() );
    BOOST_CHECK_EQUAL( fakeAstNode2.get(), returnedChildren[1].get() );

    AstNode::Ptr expectedTokenWrapperChild = returnedChildren[2];
    CheckNodeIsStoringToken( expectedTokenWrapperChild, regularToken );
}

BOOST_AUTO_TEST_SUITE_END() // GetNodeFromRuleElementsTests

/**
 * Tests that method GetChildren() will throw an error if the node is storing a token.
 */
BOOST_AUTO_TEST_CASE( GetChildren_IsStoringToken )
{
    // Create node that is storing a token
    constexpr TokenType tokenType{ T::AND };
    Token::Ptr storedToken = std::make_shared< Token >( tokenType );
    AstNode::Ptr node = std::make_shared< AstNode >( tokenType, storedToken );

    BOOST_CHECK_THROW( node->GetChildren(), std::invalid_argument );
}

/**
 * Tests that method GetChildren() will throw an error if the node is storing an empty children vector.
 */
BOOST_AUTO_TEST_CASE( GetChildren_StorageNotInUse )
{
    // Create node that is storing an empty children vector.
    constexpr GrammarSymbols::Symbol nodeLabel{ NT::Block };
    AstNode::Children children{};
    AstNode::Ptr node = std::make_shared< AstNode >( nodeLabel, children );

    BOOST_CHECK_THROW( node->GetChildren(), std::runtime_error );
}

/**
 * Tests that method GetChildren() will successfully return the stored children if they exist.
 */
BOOST_AUTO_TEST_CASE( GetChildren_Success )
{
    // Create node that is storing children.
    constexpr GrammarSymbols::Symbol nodeLabel{ NT::Block };
    AstNode::Ptr fakeChild = CreateFakeAstNode();
    AstNode::Children children{ fakeChild };
    AstNode::Ptr node = std::make_shared< AstNode >( nodeLabel, children );

    AstNode::Children returnedChildren = node->GetChildren();
    BOOST_REQUIRE_EQUAL( 1u, returnedChildren.size() );
    BOOST_CHECK_EQUAL( fakeChild.get(), returnedChildren[0].get() );
}

/**
 * Tests that method GetToken() will throw an error if the node is storing children.
 */
BOOST_AUTO_TEST_CASE( GetToken_IsStoringChildren )
{
    // Create node that is storing children.
    constexpr GrammarSymbols::Symbol nodeLabel{ NT::Block };
    AstNode::Ptr fakeChild = CreateFakeAstNode();
    AstNode::Children children{ fakeChild };
    AstNode::Ptr node = std::make_shared< AstNode >( nodeLabel, children );

    BOOST_CHECK_THROW( node->GetToken(), std::invalid_argument );
}

/**
 * Tests that method GetToken() will throw an error if the node is storing an nullptr token.
 */
BOOST_AUTO_TEST_CASE( GetToken_StorageNotInUse )
{
    // Create node that is storing a nullptr token
    constexpr TokenType tokenType{ T::AND };
    AstNode::Ptr node = std::make_shared< AstNode >( tokenType, nullptr );

    BOOST_CHECK_THROW( node->GetToken(), std::runtime_error );
}

/**
 * Tests that method GetToken() will successfully return the stored token if it exists.
 */
BOOST_AUTO_TEST_CASE( GetToken_Success )
{
    // Create node that is storing a token
    constexpr TokenType tokenType{ T::AND };
    Token::Ptr storedToken = std::make_shared< Token >( tokenType );
    AstNode::Ptr node = std::make_shared< AstNode >( tokenType, storedToken );

    Token::Ptr returnedToken = node->GetToken();
    BOOST_CHECK_EQUAL( storedToken.get(), returnedToken.get() );
}

BOOST_AUTO_TEST_SUITE_END() // AstNodeTests