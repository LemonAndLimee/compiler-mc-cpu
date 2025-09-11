/**
 * Contains definitions of methods related to grammar symbols and rules.
 */

#include "Grammar.h"
#include "Logger.h"
#include "TokenTypes.h"

#include <string>
#include <stdexcept>

/**
 * Determines whether symbol is terminal or non-terminal.
 *
 * \param[in]  symbol  The symbol being checked.
 *
 * \return Type of the symbol.
 */
GrammarSymbols::SymbolType
GrammarSymbols::GetSymbolType(
    GrammarSymbols::Symbol symbol
)
{
    unsigned maskedSymbolType = symbol & SymbolType::BITMASK;
    if ( SymbolType::Terminal == maskedSymbolType )
    {
        return SymbolType::Terminal;
    }
    else if ( SymbolType::NonTerminal == maskedSymbolType )
    {
        return SymbolType::NonTerminal;
    }
    else
    {
        LOG_ERROR_AND_THROW( "Unknown symbol (" + std::to_string( symbol ) + ") type: "
                             + std::to_string( maskedSymbolType ), std::invalid_argument );
    }
}

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
    SymbolType symbolType = GetSymbolType( symbol );
    if ( SymbolType::NonTerminal == symbolType )
    {
        return g_nonTerminalStringForms.find( static_cast< NT >( symbol ) )->second;
    }
    else if ( SymbolType::Terminal == symbolType )
    {
        return TokenTypes::ConvertTokenTypeToString( static_cast< T >( symbol ) );
    }
    else
    {
        return "UNKNOWN (" + std::to_string( symbol ) + ")";
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

    for ( size_t i = 0; i < rule.size(); ++i )
    {
        ruleString += GrammarSymbols::ConvertSymbolToString( rule[i] ) + " ";
    }
    ruleString.pop_back();

    return ruleString;
}