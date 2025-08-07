/**
 * Contains definitions of methods related to token types.
 */

#include "TokenTypes.h"

#include <stdexcept>

/**
 * Converts token type into human-readable string form.
 *
 * \return String form of token type.
 */
std::string
TokenTypes::ConvertTokenTypeToString (
    TokenType type
)
{
    // If token type is an exact match to a string, use this
    std::unordered_map< std::string,TokenType >::const_iterator iter;
    for ( iter = g_tokenTypesExactMatches.begin(); iter != g_tokenTypesExactMatches.end(); ++iter )
    {
        if ( type == iter->second )
        {
            return iter->first;
        }
    }

    switch ( type )
    {
    case DATA_TYPE:
        return "dt";
    case BYTE:
        return "byte";
    case IDENTIFIER:
        return "id";
    case INVALID_TOKEN:
        return "invalid";
    default:
        std::string errMsg = "Unknown token type " + std::to_string( type );
        LOG_ERROR( errMsg );
        throw std::invalid_argument( errMsg );
    }
}