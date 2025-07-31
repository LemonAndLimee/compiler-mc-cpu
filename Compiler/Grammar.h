/**
 * Contains enumerations of Grammar symbols, as well as Grammar rules representing the target language,
 * stored in collections.
 */

#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>

namespace GrammarSymbols
{
    enum SymbolType {
        Terminal    = 0x0100,
        NonTerminal = 0x0200,
        BITMASK     = 0xFF00
    };
    // Terminal Symbol
    enum T
    {
        INVALID_TOKEN = SymbolType::Terminal, // A non-existent token
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
        BITWISE_OR,
        BITWISE_AND,
        LSHIFT,
        RSHIFT,
        PAREN_OPEN,
        PAREN_CLOSE,
        BRACE_OPEN,
        BRACE_CLOSE,
        SEMICOLON,
    };
    // Non-terminal Symbol. Use lowercase to help make T vs NT more human-readable.
    enum NT
    {
        Block = SymbolType::NonTerminal,
        Section,
        For_loop,
        For_init,
        If_else,
        Else,
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

    using Symbol = unsigned;

    // Token types that represent the relationship-definer of a given rule, and can be assigned to an AST node label.
    // This does not include types such as identifier, data type, etc. as these are tokens which hold one value and
    // therefore do not require their own AST node.
    // Important: a rule may only contain max. 1 node label type symbols.
    const std::unordered_set< T > g_nodeLabelTerminals {
        ASSIGN,
        IF,
        ELSE,
        FOR,
        WHILE,
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        MOD,
        EXPONENT,
        EQ,
        NEQ,
        LEQ,
        GEQ,
        LT,
        GT,
        NOT,
        OR,
        AND,
        BITWISE_OR,
        BITWISE_AND,
        LSHIFT,
        RSHIFT
    };

    // Token types that can be skipped for the AST, e.g. punctuation
    const std::unordered_set< T > g_skipForAstTerminals {
        PAREN_OPEN,
        PAREN_CLOSE,
        BRACE_OPEN,
        BRACE_CLOSE,
        SEMICOLON
    };

    // Human-readable string forms of non-terminal symbols
    const std::unordered_map< NT, std::string > g_nonTerminalStringForms {
        { Block , "Block" },
        { Section , "Section" },
        { For_loop , "For_loop" },
        { For_init , "For_init" },
        { If_else , "If_else" },
        { Else , "Else" },
        { While_loop , "While_loop" },
        { Statement , "Statement" },
        { Comparison , "Comparison" },
        { Logical , "Logical" },
        { Variable , "Variable" },
        { Bitwise , "Bitwise" },
        { Shift , "Shift" },
        { Negation , "Negation" },
        { Expression , "Expression" },
        { Term , "Term" },
        { Exp_factor , "Exp_factor" },
        { Factor , "Factor" }
    };

    SymbolType GetSymbolType( Symbol symbol );
    std::string ConvertSymbolToString( Symbol symbol );
}

namespace GrammarRules
{
    using namespace GrammarSymbols;

    using Rule = std::vector< Symbol >;
    using Rules = std::vector< Rule >;

    std::string ConvertRuleToString( const Rule& rule );

    // Mapping between non-terminal symbols and the rules they can expand to.
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
                { T::IF, T::PAREN_OPEN, Logical, T::PAREN_CLOSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE },
                { T::IF, T::PAREN_OPEN, Logical, T::PAREN_CLOSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE, Else }
            }
        },
        {
            Else,
            {
                { T::ELSE, T::BRACE_OPEN, Block, T::BRACE_CLOSE }
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
                // Enforce the use of parentheses for complex statements e.g. 1 + (2 + 3)
                // TODO: investigate removing this restriction - would have to deal with infinite
                // recursion.
                { T::PAREN_OPEN, Logical, T::PAREN_CLOSE }
            }
        },
    };
}