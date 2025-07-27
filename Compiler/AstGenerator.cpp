/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include <stdexcept>

using namespace GrammarRules;

/**
 * \brief Generates an Abstract Syntax tree from a given set of tokens. If the syntax of the tokens
 *        is invalid, it throws an error.
 *
 * \param[in]  tokens               String of tokens representing the code to be converted.
 * \param[in]  startingNt           Non-terminal symbol identifying the rule to start the tree generation from.
 * \param[in]  allowLeftoverTokens  If set to false, will throw error if there are leftover tokens after resolving the
 *                                  rule. Can be used for root node of AST, as well as ending NTs in a rule.
 *
 * \return  Pointer to the root of the generated tree.
 */
AstNode::Ptr
AstGenerator::GenerateAst( TokensVector& tokens, GrammarSymbols::NT startingNt, bool allowLeftoverTokens )
{
    if ( 0u == g_nonTerminalRuleSets.count( startingNt ) )
    {
        throw std::invalid_argument( "Starting NT symbol has no associated rules." );
    }

    // Try each rule belonging to the starting NT symbol
    Rules rules =  g_nonTerminalRuleSets.find( startingNt )->second;
    for ( Rule currentRule : rules )
    {
        // Children of the node we are building
        std::vector< AstNode::Child > children;
        // Index of the current token we are consuming from the input vector
        size_t currentTokenIndex{ 0u };

        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokens, currentRule, children, currentTokenIndex ) )
        {
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens
        // TODO: check for leftover tokens and throw if not allowed
        // Construct an AST node from the children
    }

    throw std::runtime_error( "Not implemented.");
}

/**
 * \brief Tries to resolve a given rule (collection of symbols) from the given list of tokens.
 *
 * \param[in]   tokens      String of tokens representing the code to be converted.
 * \param[in]   rule        The current rule that is being tested. Consists of symbols, either terminal or non-terminal.
 * \param[out]  children    Child nodes or tokens belonging to the rule being tested.
 * \param[out]  tokenIndex  The index of the next token after this rule has been resolved. Used by a calling function
 *                          to test a later rule, or determine if there are leftover tokens.  
 *
 * \return  True if the rule could successfully be matched to the tokens, false otherwise.
 */
bool
AstGenerator::TryRule(
    const TokensVector& tokens,
    const Rule& rule,
    std::vector< AstNode::Child >& children,
    size_t& tokenIndex
)
{
    for ( Symbol symbol : rule )
    {
        // If symbol is terminal
        // If rule symbol matches the token currently being pointed at, increment token ptr and continue
        // If symbol not to be skipped (e.g. {}, (), ;), add the token to children

        // Else if symbol non terminal, call get AST node on that rule name
        // Then add this to children
        // TODO: do we need the get AST node method to update our token index i.e. taking it as out param?
    }
}