/**
 * Contains definitions of methods related to grammar symbols and rules.
 */

#include "Grammar.h"

/**
 * Converts symbol into human-readable string form.
 *
 * \return String form of symbol.
 */
std::string
GrammarSymbols::ConvertSymbolToString(
    Symbol symbol
)
{
    if ( const T* pTerminal = std::get_if< T >( &symbol ) )
    {
        return TokenTypes::ConvertTokenTypeToString( *pTerminal );
    }
    else if ( const NT* pNonTerminal = std::get_if< NT >( &symbol ) )
    {
        if ( 0 < g_nonTerminalStringForms.count( *pNonTerminal ) )
        {
            return g_nonTerminalStringForms.find( *pNonTerminal )->second;
        }
        std::string errMsg = "Could not find string form for NT symbol " + std::to_string( *pNonTerminal );
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }
    else
    {
        LOG_ERROR( "Symbol doesn't belong to T or NT sets." );
        throw std::runtime_error( "Symbol doesn't belong to T or NT sets." );
    }
}

/**
 * Converts grammar rule (a set of symbols) into human-readable string form.
 *
 * \return String form of rule.
 */
std::string
GrammarRules::ConvertRuleToString(
    const Rule& rule
)
{
    std::string ruleString;
    for ( Rule::iterator iter = rule.begin(); iter != rule.end(); ++iter )
    {
        ruleString += GrammarSymbols::ConvertSymbolToString( *iter ) + " ";
    }

    ruleString.pop_back();

    return ruleString;
}