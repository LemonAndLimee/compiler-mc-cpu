/**
 * Definition of lexical Token class.
 */

#include "Token.h"
#include "Logger.h"

#include <vector>

using namespace TokenTypes;

Token::Token( TokenType type )
: Token( type, std::make_shared< TokenValue >() )
{
}

Token::Token( TokenType type, TokenValue::Ptr value )
: m_type( type ),
  m_value( value )
{
}

/**
 * Converts token into human-readable string form.
 *
 * \return String form of token.
 */
std::string
Token::ToString()
{
    std::string outputStr;
    outputStr += TokenTypes::ConvertTokenTypeToString( m_type );

    if ( TokenValueType::UNUSED != m_value->m_valueType )
    {
        outputStr += ":";
        if ( TokenValueType::NUMERIC == m_value->m_valueType )
        {
            outputStr += std::to_string( m_value->m_value.numericValue );
        }
        else if ( TokenValueType::STRING == m_value->m_valueType )
        {
            outputStr +=  m_value->m_value.stringValue;
        }
        else if ( TokenValueType::DTYPE == m_value->m_valueType )
        {
            if ( DataType::DT_BYTE == m_value->m_value.dataTypeValue )
            {
                outputStr += "byte";
            }
            else
            {
                LOG_WARN( "Unknown token value data type." );
            }
        } 
        else
        {
            LOG_WARN( "Unknown token value type." );
        }
    }

    return outputStr;
}

/**
 * Converts vector of tokens into human-readable string form.
 *
 * `param[in]  tokensVector  Vector of tokens.
 *
 * \return String form of tokens.
 */
std::string
Token::ConvertTokensVectorToString(
    const std::vector< Token::Ptr >& tokensVector
)
{
    std::string tokensVectorString;
    for ( size_t i = 0; i < tokensVector.size(); ++i )
    {
        tokensVectorString += tokensVector[i]->ToString() + ", ";
    }

    // Pop off the leftover comma and space
    tokensVectorString.pop_back();
    tokensVectorString.pop_back();

    return tokensVectorString;
}