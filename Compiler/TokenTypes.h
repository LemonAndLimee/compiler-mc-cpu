#pragma once

#include <unordered_map>
#include <string>

#include "TokenValue.h"

namespace TokenTypes
{

enum TokenType
{
    INVALID_TOKEN, // A non-existent token
    DATA_TYPE,
    ASSIGN,
    BYTE,
    IF,
    ELSE,
    WHILE,
    FOR,
    IDENTIFIER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    EXPONENT,
    EQ,  // ==
    NEQ, // !=
    LEQ, // <=
    GEQ, // >=
    LT,  // <
    GT,  // >
    NOT,
    OR,
    AND,
    LSHIFT,
    RSHIFT,
    PAREN_OPEN,
    PAREN_CLOSE,
    BRACE_OPEN,
    BRACE_CLOSE,
    SEMICOLON
};

// Contains the exact string matches (if they exist) of token types.
const std::unordered_map<TokenType, std::string> g_tokenTypesExactMatches {
    // DATA_TYPE -> Non-exact match
    { ASSIGN, "=" },
    // BYTE -> Non-exact match
    { IF, "if" },
    { ELSE, "else" },
    { WHILE, "while" },
    { FOR, "for" },
    // IDENTIFIER -> Non-exact match
    { PLUS, "+" },
    { MINUS, "-" },
    { MULTIPLY, "*" },
    { DIVIDE, "/" },
    { MOD, "%" },
    { EXPONENT, "^" },
    { EQ, "==" },
    { NEQ, "!=" },
    { LEQ, "<=" },
    { GEQ, ">=" },
    { LT, "<" },
    { GT, ">" },
    { NOT, "!" },
    { OR, "|" },
    { AND, "&" },
    { LSHIFT, "<<" },
    { RSHIFT, ">>" },
    { PAREN_OPEN, "(" },
    { PAREN_CLOSE, ")" },
    { BRACE_OPEN, "{" },
    { BRACE_CLOSE, "}" },
    { SEMICOLON, ";" },
};

// Contains the value types held by the non-exact-match token types.
const std::unordered_map<TokenType, TokenValueType> g_tokenValueTypes {
    { DATA_TYPE, TokenValueType::DTYPE },
    { BYTE, TokenValueType::NUMERIC },
    { IDENTIFIER, TokenValueType::STRING },
};

// Contains the mappings of data type token strings.
const std::unordered_map<const std::string, DataType> g_dataTypeStrings {
    { "byte", DataType::DT_BYTE },
};

}