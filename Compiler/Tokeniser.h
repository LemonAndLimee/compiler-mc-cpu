/**
 * Contains declaration of Tokeniser class.
 */

#pragma once

#include "Token.h"
#include <string>
#include <memory>

// Defines the prefix string that means "everything else on this line is comment"
const std::string g_commentPrefix = "//";

// Class responsible for converting a string into a stream of tokens.
class Tokeniser
{
public:
    using Ptr = std::shared_ptr< Tokeniser >;
    using UPtr = std::unique_ptr< Tokeniser >;
    Tokeniser() = default;

    Tokens ConvertStringToTokens( const std::string& inputString );
protected:
    void ConvertSingleLineAndAppend( const std::string& inputString, Tokens& tokens );

    Token::Ptr GetNextToken( const std::string& inputString, size_t& startIndex );

    TokenType GetTokenType( const std::string& tokenString ) noexcept;

    bool IsWhitespace( const char character );

    Token::Ptr CreateTokenFromString( const TokenType type, const std::string& tokenString );
};