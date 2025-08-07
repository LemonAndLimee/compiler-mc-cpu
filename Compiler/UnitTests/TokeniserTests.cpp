#include <boost/test/unit_test.hpp>
#include "Tokeniser.h"

class TokeniserTestsFixture
{
public:
    TokeniserTestsFixture() = default;

    void
    CheckTokensAgainstExpected( Tokens& expectedTokens, Tokens& receivedTokens )
    {
        BOOST_REQUIRE_EQUAL( expectedTokens.size(), receivedTokens.size() );

        for ( size_t index = 0u; index < expectedTokens.size(); ++index )
        {
            Token::Ptr expectedToken = expectedTokens[index];
            Token::Ptr receivedToken = receivedTokens[index];

            BOOST_CHECK( *expectedToken.get() == *receivedToken.get() );
        }
    }
};

BOOST_FIXTURE_TEST_SUITE( TokeniserTests, TokeniserTestsFixture )

/**
 * Tests that when ConvertStringToTokens() is called with an empty string, it returns an empty collection of tokens.
 */
BOOST_AUTO_TEST_CASE( ConvertEmptyString )
{
    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( "" );

    Tokens emptyTokens{};
    CheckTokensAgainstExpected( emptyTokens, outputTokens );
}

BOOST_AUTO_TEST_SUITE( ConvertSingleLineTests )

/**
 * Tests that when ConvertStringToTokens() is called on a comment line, it returns an empty collection of tokens.
 */
BOOST_AUTO_TEST_CASE( ConvertCommentLine )
{
    std::string stringToConvert = "// I am a commented out line";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens emptyTokens{};
    CheckTokensAgainstExpected( emptyTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line made up of only whitespace, it returns an empty
 * collection of tokens.
 */
BOOST_AUTO_TEST_CASE( ConvertWhitespaceLine )
{
    std::string stringToConvert = "     \t  ";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens emptyTokens{};
    CheckTokensAgainstExpected( emptyTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line made up of a single, exact-match token, it returns a
 * container containing the expected token.
 */
BOOST_AUTO_TEST_CASE( ConvertExactMatchSingleToken )
{
    std::string stringToConvert = "for";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Token::Ptr expectedToken = std::make_shared<Token>( FOR, std::make_shared<TokenValue>() );
    Tokens expectedTokens{ expectedToken };
    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line made up of a single, pattern-match token, it returns a
 * container containing the expected token.
 */
BOOST_AUTO_TEST_CASE( ConvertPatternMatchSingleToken )
{
    std::string stringToConvert = "variableName";
    TokenType expectedTokenType{ IDENTIFIER };

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Token::Ptr expectedToken = std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "variableName") );
    Tokens expectedTokens{ expectedToken };
    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line made up of multiple tokens, it returns a container
 * containing the expected tokens.
 */
BOOST_AUTO_TEST_CASE( ConvertMultipleTokensLine )
{
    std::string stringToConvert = "byte myNumber = (3+4)*2;";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens expectedTokens = {
        std::make_shared<Token>( DATA_TYPE, std::make_shared<TokenValue>( DataType::DT_BYTE ) ),
        std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "myNumber" ) ),
        std::make_shared<Token>( ASSIGN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( PAREN_OPEN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 3u ) ),
        std::make_shared<Token>( PLUS, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 4u ) ),
        std::make_shared<Token>( PAREN_CLOSE, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( MULTIPLY, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 2u ) ),
        std::make_shared<Token>( SEMICOLON, std::make_shared<TokenValue>() )
    };

    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line that doesn't match any valid tokens, it throws an error.
 */
BOOST_AUTO_TEST_CASE( NoMatchLine )
{
    std::string stringToConvert = "1hello"; // Invalid token, as an identifer cannot begin with a number.

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    BOOST_CHECK_THROW( tokeniser->ConvertStringToTokens( stringToConvert ), std::invalid_argument );
}

/**
 * Tests that when ConvertStringToTokens() is called on a line that contains a non-matching string, it throws an error.
 */
BOOST_AUTO_TEST_CASE( PartialNoMatchLine )
{
    std::string stringToConvert = "valid 1notvalid valid";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    BOOST_CHECK_THROW( tokeniser->ConvertStringToTokens( stringToConvert ), std::invalid_argument );
}

BOOST_AUTO_TEST_SUITE_END() // ConvertSingleLineTests

BOOST_AUTO_TEST_SUITE( ConvertMultipleLinesTests )

/**
 * Tests that when ConvertStringToTokens() is called multiple matching lines, it returns a container containing
 * the expected tokens.
 */
    BOOST_AUTO_TEST_CASE( ConvertMultipleMatchingLines )
{
    std::string stringToConvert = "byte myNumber = (3+4)*2;\nbyte myNumber = (3+4)*2;\nbyte myNumber = (3+4)*2;";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens expectedLineTokens = {
        std::make_shared<Token>( DATA_TYPE, std::make_shared<TokenValue>( DataType::DT_BYTE ) ),
        std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "myNumber" ) ),
        std::make_shared<Token>( ASSIGN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( PAREN_OPEN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 3u ) ),
        std::make_shared<Token>( PLUS, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 4u ) ),
        std::make_shared<Token>( PAREN_CLOSE, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( MULTIPLY, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 2u ) ),
        std::make_shared<Token>( SEMICOLON, std::make_shared<TokenValue>() )
    };

    Tokens expectedTokens;
    expectedTokens.resize( expectedLineTokens.size() * 3 );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() + expectedLineTokens.size() );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() + ( expectedLineTokens.size() * 2 ) );

    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called multiple lines, with the first commented, the tokens
 * returned reflect the second line.
 */
BOOST_AUTO_TEST_CASE( ConvertMultipleLines_FirstCommented )
{
    std::string stringToConvert = "//commented line\nbyte myNumber = (3+4)*2;";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens expectedTokens = {
        std::make_shared<Token>( DATA_TYPE, std::make_shared<TokenValue>( DataType::DT_BYTE ) ),
        std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "myNumber" ) ),
        std::make_shared<Token>( ASSIGN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( PAREN_OPEN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 3u ) ),
        std::make_shared<Token>( PLUS, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 4u ) ),
        std::make_shared<Token>( PAREN_CLOSE, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( MULTIPLY, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 2u ) ),
        std::make_shared<Token>( SEMICOLON, std::make_shared<TokenValue>() )
    };

    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called multiple lines, with the middle commented, the tokens
 * returned reflect the surrounding lines.
 */
BOOST_AUTO_TEST_CASE( ConvertMultipleLines_MiddleCommented )
{
    std::string stringToConvert = "byte myNumber = (3+4)*2;\n//commented line\nbyte myNumber = (3+4)*2;";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens expectedLineTokens = {
        std::make_shared<Token>( DATA_TYPE, std::make_shared<TokenValue>( DataType::DT_BYTE ) ),
        std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "myNumber" ) ),
        std::make_shared<Token>( ASSIGN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( PAREN_OPEN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 3u ) ),
        std::make_shared<Token>( PLUS, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 4u ) ),
        std::make_shared<Token>( PAREN_CLOSE, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( MULTIPLY, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 2u ) ),
        std::make_shared<Token>( SEMICOLON, std::make_shared<TokenValue>() )
    };

    Tokens expectedTokens;
    expectedTokens.resize( expectedLineTokens.size() * 2 );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() + expectedLineTokens.size() );

    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called multiple lines, with a whitespace line, the tokens
 * returned reflect the surrounding lines.
 */
BOOST_AUTO_TEST_CASE( ConvertMultipleLines_OneWhitespace )
{
    std::string stringToConvert = "byte myNumber = (3+4)*2;\n  \t  \nbyte myNumber = (3+4)*2;";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    Tokens outputTokens = tokeniser->ConvertStringToTokens( stringToConvert );

    Tokens expectedLineTokens = {
        std::make_shared<Token>( DATA_TYPE, std::make_shared<TokenValue>( DataType::DT_BYTE ) ),
        std::make_shared<Token>( IDENTIFIER, std::make_shared<TokenValue>( "myNumber" ) ),
        std::make_shared<Token>( ASSIGN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( PAREN_OPEN, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 3u ) ),
        std::make_shared<Token>( PLUS, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 4u ) ),
        std::make_shared<Token>( PAREN_CLOSE, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( MULTIPLY, std::make_shared<TokenValue>() ),
        std::make_shared<Token>( BYTE, std::make_shared<TokenValue>( 2u ) ),
        std::make_shared<Token>( SEMICOLON, std::make_shared<TokenValue>() )
    };

    Tokens expectedTokens;
    expectedTokens.resize( expectedLineTokens.size() * 2 );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() );
    std::copy( expectedLineTokens.begin(), expectedLineTokens.end(), expectedTokens.begin() + expectedLineTokens.size() );

    CheckTokensAgainstExpected( expectedTokens, outputTokens );
}

/**
 * Tests that when ConvertStringToTokens() is called multiple lines, one of which doesn't match, an exception is
 * thrown.
 */
BOOST_AUTO_TEST_CASE( ConvertMultipleLines_OneNonMatch )
{
    std::string stringToConvert = "byte myNumber = (3+4)*2;\n 1invalid";

    Tokeniser::Ptr tokeniser = std::make_shared<Tokeniser>();
    BOOST_CHECK_THROW( tokeniser->ConvertStringToTokens( stringToConvert ), std::invalid_argument );
}

BOOST_AUTO_TEST_SUITE_END() // ConvertMultipleLinesTests

BOOST_AUTO_TEST_SUITE_END() // TokeniserTests