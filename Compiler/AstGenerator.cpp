/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
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
    const TokensVector& tokens,
    GrammarSymbols::NT startingNt,
    bool allowLeftoverTokens
)
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

        // Make an editable copy of the input tokens vector
        TokensVector tokensCopy = tokens;

        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokensCopy, currentRule, children, currentTokenIndex ) )
        {
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens

        // If there are leftover tokens and this is not allowed, consider this a non-match for this rule and continue
        // looping through further rules.
        if ( !allowLeftoverTokens )
        {
            if ( !tokensCopy.empty() )
            {
                // TODO: add warning
                continue;
            }
        }
        
        if ( children.empty() )
        {
            throw std::runtime_error( "Rule match found but no child nodes or tokens created." );
        }

        // Construct an AST node from children
        return CreateNodeFromChildren( children );
    }

    // If the loop is exited and no rule match has been found
    // TODO: make string form of symbol names and rules for better error messages
    throw std::runtime_error( "No matching rule could be found." );
}

/**
 * \brief  Tries to resolve a given rule (collection of symbols) from the given list of tokens.
 *
 * \param[in,out]  tokens      String of tokens representing the code to be converted. If a token matches a symbol
 *                             in the rule, it is popped from the start of the vector.
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
    TokensVector& tokens,
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
    throw std::runtime_error( "Not implemented" );
}