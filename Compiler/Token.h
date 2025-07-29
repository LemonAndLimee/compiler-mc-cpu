/**
 * Contains declaration of lexical Token class.
 */

#pragma once

#include "TokenTypes.h"
#include <stdint.h>
#include <memory>
#include <deque>

using namespace TokenTypes;

/**
 * Token class.
 */
class Token
{
public:
    using Ptr = std::shared_ptr<Token>;

    Token( TokenType type );
    Token( TokenType type, TokenValue::Ptr value );

    std::string ToString();

    static std::string ConvertTokensToString( const std::deque< Token::Ptr >& tokens );
    static std::string ConvertTokensToString( const std::deque< Token::Ptr >& tokens, size_t numTokens );

    TokenType m_type;

    // Contains token value - e.g. if type is identifier, this would contain the variable name.
    // If the token represents a constant literal number, this would contain the number.
    TokenValue::Ptr m_value;

    bool
    operator==( const Token& comparisonToken ) const
    {
        return comparisonToken.m_type == m_type && *comparisonToken.m_value.get() == *m_value.get();
    }
};

using Tokens = std::deque< Token::Ptr >;