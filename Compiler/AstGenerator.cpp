/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include "Logger.h"
#include <stdexcept>

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
AstGenerator::GenerateAst(
    const Tokens& tokens,
    GrammarSymbols::NT startingNt,
    bool allowLeftoverTokens
)
{
    std::string startingNtString = GrammarSymbols::ConvertSymbolToString( startingNt );
    if ( 0u == g_nonTerminalRuleSets.count( startingNt ) )
    {
        std::string errMsg = "Starting NT symbol " + startingNtString + " has no associated rules.";
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }

    LOG_INFO( "Generating AST for starting NT " + startingNtString + "." );
    // Try each rule belonging to the starting NT symbol
    Rules rules =  g_nonTerminalRuleSets.find( startingNt )->second;
    for ( Rule currentRule : rules )
    {
        std::string ruleString = GrammarRules::ConvertRuleToString( currentRule );
        LOG_INFO( "Trying rule '" + ruleString + "'" );

        // Children of the node we are building
        std::vector< AstNode::Child > children;
        // Index of the current token we are consuming from the input collection
        size_t currentTokenIndex{ 0u };

        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokens, currentRule, children, currentTokenIndex ) )
        {
            LOG_INFO_LOW_LEVEL( "Rule doesn't match, continuing..." );
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens

        // If there are leftover tokens and this is not allowed, consider this a non-match for this rule and continue
        // looping through further rules.
        if ( !allowLeftoverTokens )
        {
            if ( tokens.count() <= currentTokenIndex )
            {
                LOG_INFO_LOW_LEVEL( "Leftover tokens at the end: rejecting rule '" + ruleString + "'." );
                continue;
            }
        }
        
        if ( children.empty() )
        {
            std::string errMsg = "Rule match found for '" + ruleString + "' but no child nodes or tokens created.";
            LOG_ERROR( errMsg );
            throw std::runtime_error( errMsg );
        }

        // Construct an AST node from children
        return CreateNodeFromChildren( children );
    }

    // If the loop is exited and no rule match has been found
    std::string errMsg = "No matching rule could be found for start symbol " + startingNtString;
    LOG_ERROR( errMsg + " and tokens '" + Token::ConvertTokensToString( tokens ) + "'");
    throw std::runtime_error( errMsg );
}

/**
 * \brief  Tries to resolve a given rule (collection of symbols) from the given list of tokens.
 *
 * \param[in]      tokens      String of tokens representing the code to be converted.
 * \param[in]      rule        The current rule that is being tested. Consists of symbols, either terminal or
 *                             non-terminal.
 * \param[out]     children    Child nodes or tokens belonging to the rule being tested.
 * \param[out]     tokenIndex  The index of the next token after this rule has been resolved. Used by a calling
 *                             method to test a later rule, or determine if there are leftover tokens.  
 *
 * \return  True if the rule could successfully be matched to the tokens, false otherwise.
 */
bool
AstGenerator::TryRule(
    const Tokens& tokens,
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
    LOG_WARN( "Method not implemented, returning false." );
    return false;
}

/**
 * \brief  Creates an AST node instance from a given set of child nodes/tokens. Skips tokens that can be skipped,
 *         e.g. punctuation, and assigns the node label according to the set of children.
 *
 * \param[in]  children  Child nodes or tokens belonging to the node being created.
 *
 * \return  Ptr to the created AST node.
 */
AstNode::Ptr
AstGenerator::CreateNodeFromChildren(
    const std::vector< AstNode::Child >& children
)
{
    LOG_WARN( "Method not implemented, throwing exception..." );
    throw std::runtime_error( "Not implemented" );
}