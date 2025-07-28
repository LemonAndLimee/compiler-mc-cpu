/**
 * Contains collections of data related to token types.
 */

#pragma once

#include <unordered_map>
#include <string>

#include "TokenValue.h"
#include "Logger.h"
#include "Grammar.h"

namespace TokenTypes
{

using namespace GrammarSymbols;
using TokenType = T;

// Contains the exact string matches (if they exist) of token types.
const std::unordered_map<std::string, TokenType> g_tokenTypesExactMatches {
    // DATA_TYPE -> Non-exact match
    { "=", ASSIGN },
    // BYTE -> Non-exact match
    { "if", IF },
    { "else", ELSE },
    { "while", WHILE },
    { "for", FOR },
    // IDENTIFIER -> Non-exact match
    { "+", PLUS },
    { "-", MINUS },
    { "*", MULTIPLY },
    { "/", DIVIDE },
    { "%", MOD },
    { "^", EXPONENT },
    { "==", EQ },
    { "!=", NEQ },
    { "<=", LEQ },
    { ">=", GEQ },
    { "<", LT },
    { ">", GT },
    { "!", NOT },
    { "|", OR },
    { "&", AND },
    { "||", BITWISE_OR },
    { "&&", BITWISE_AND },
    { "<<", LSHIFT },
    { ">>", RSHIFT },
    { "(", PAREN_OPEN },
    { ")", PAREN_CLOSE },
    { "{", BRACE_OPEN },
    { "}", BRACE_CLOSE },
    { ";", SEMICOLON },
};

// Contains the value types held by the non-exact-match token types.
const std::unordered_map<TokenType, TokenValueType> g_tokenValueTypes {
    { DATA_TYPE, TokenValueType::DTYPE },
    { BYTE, TokenValueType::NUMERIC },
    { IDENTIFIER, TokenValueType::STRING },
};

// Contains the mappings of data type token strings.
const std::unordered_map<std::string, DataType> g_dataTypeStrings {
    { "byte", DataType::DT_BYTE },
};

std::string ConvertTokenTypeToString ( TokenType type );

}