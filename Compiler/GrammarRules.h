/**
 * Contains enumerations of Grammar symbols, as well as Grammar rules representing the target language,
 * stored in collections.
 */

#pragma once

#include "TokenTypes.h"
#include <set>
#include <variant>

using namespace TokenTypes;

namespace GrammarSymbols
{

// Terminal Symbol
using T = TokenTypes::TokenType;
// Non-terminal Symbol. Use lowercase to help make T vs NT more human-readable.
enum NT
{
    Block,
    Section,
    For_loop,
    For_init,
    If_else,
    If_stmt,
    While_loop,
    Statement,
    Comparison,
    Logical,
    Variable,
    Bitwise,
    Shift,
    Negation,
    Expression,
    Term,
    Exp_factor,
    Factor,
};

using Symbol = std::variant< T, NT >;
using Rule = std::set< Symbol >;
using Rules = std::set< Rule >;

const std::unordered_map< NT, Rules > g_nonTerminalRuleSets {
    {
        Block,
        {
            { Section, Block },
            { Section }
        }
    },
    {
        Section,
        {
            { Statement, T::SEMICOLON },
            { For_loop, T::SEMICOLON },
            { If_else, T::SEMICOLON },
            { While_loop, T::SEMICOLON },
        }
    },
    {
        For_loop,
        {
            { T::FOR, For_init, T::BRACE_OPEN, Block, T::BRACE_CLOSE }
        }
    },
    {
        For_init,
        {
            { T::PAREN_OPEN, Statement, T::SEMICOLON, Comparison, T::SEMICOLON, Statement, T::PAREN_CLOSE }
        }
    },
    {
        If_else,
        {
            { If_stmt, T::ELSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE },
            { If_stmt }
        }
    },
    {
        If_stmt,
        {
            { T::IF, T::PAREN_OPEN, Logical, T::PAREN_CLOSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE }
        }
    },
    {
        While_loop,
        {
            { T::WHILE, T::PAREN_OPEN, Logical, T::PAREN_CLOSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE }
        }
    },
    {
        Statement,
        {
            // Use "Variable" here to restrict variable declarations to the LHS of assignment statements.
            { Variable, T::ASSIGN, Logical }
        }
    },
    {
        Variable,
        {
            { T::DATA_TYPE, T::IDENTIFIER },
            { T::IDENTIFIER }
        }
    },
    {
        Logical,
        {
            { Bitwise, T::OR, Bitwise },
            { Bitwise, T::AND, Bitwise },
            { Bitwise }
        }
    },
    {
        Bitwise,
        {
            { Comparison, T::BITWISE_OR, Comparison },
            { Comparison, T::BITWISE_AND, Comparison },
            { Comparison }
        }
    },
    {
        Comparison,
        {
            { Shift, T::EQ, Shift },
            { Shift, T::NEQ, Shift },
            { Shift, T::LEQ, Shift },
            { Shift, T::GEQ, Shift },
            { Shift, T::LT, Shift },
            { Shift, T::GT, Shift },
            { Shift }
        }
    },
    {
        Shift,
        {
            { Negation, T::LSHIFT, Negation },
            { Negation, T::RSHIFT, Negation },
            { Negation }
        }
    },
    {
        Negation,
        {
            { T::NOT, Expression },
            { Expression }
        }
    },
    {
        Expression,
        {
            { Term, T::PLUS, Term },
            { Term, T::MINUS, Term },
            { Term }
        }
    },
    {
        Term,
        {
            { Exp_factor, T::MULTIPLY, Exp_factor },
            { Exp_factor, T::DIVIDE, Exp_factor },
            { Exp_factor, T::MOD, Exp_factor },
            { Exp_factor }
        }
    },
    {
        Exp_factor,
        {
            { Factor, T::EXPONENT, Factor },
            { Factor }
        }
    },
    {
        Factor,
        {
            { T::IDENTIFIER },
            { T::BYTE },
            { T::PAREN_OPEN, Logical, T::PAREN_CLOSE }
        }
    },
};

}