/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include "Logger.h"
#include <stdexcept>

/**
 * \brief Generates an Abstract Syntax tree from a given set of tokens. If the syntax of the tokens
 *        is invalid, it returns nullptr. Note: this method will only edit the tokens argument upon
 *        successful AST creation.
 *
 * \param[in,out]  tokens               String of tokens representing the code to be converted.
 * \param[in]      startingNt           Non-terminal symbol identifying the rule to start the tree generation from.
 * \param[in]      allowLeftoverTokens  If set to false, will return nullptr if there are leftover tokens after
 *                                      resolving the rule. Can be used for root node of AST, as well as ending NTs in
 *                                      a rule.
 *
 * \return  Pointer to the root of the generated tree. Nullptr if tokens are invalid.
 */
AstNode::Ptr
AstGenerator::GenerateAst(
    Tokens& tokens,
    GrammarSymbols::NT startingNt,
    bool allowLeftoverTokens
)
{
    std::string startingNtString = GrammarSymbols::ConvertSymbolToString( startingNt );
    LOG_INFO_MEDIUM_LEVEL( "Generating AST for starting symbol " + startingNtString );
    LOG_INFO_LOW_LEVEL( "Tokens: " + Token::ConvertTokensToString( tokens, 3 )
                        + "... Allow leftover tokens=" + std::to_string( allowLeftoverTokens ) );

    if ( 0u == g_nonTerminalRuleSets.count( startingNt ) )
    {
        std::string errMsg = "Starting symbol " + startingNtString + " has no associated rules.";
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }

    // Try each rule belonging to the starting NT symbol
    Rules rules =  g_nonTerminalRuleSets.find( startingNt )->second;
    for ( Rule currentRule : rules )
    {
        // Child elements of the node we are building
        AstNode::Elements elements;
        
        // Make copy of tokens to try this rule with
        Tokens tokensCopy = tokens;

        std::string ruleString = GrammarRules::ConvertRuleToString( currentRule );
        LOG_INFO_MEDIUM_LEVEL( "Inside " + startingNtString + ": trying rule '" + ruleString );
        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokensCopy, currentRule, allowLeftoverTokens, elements ) )
        {
            LOG_INFO_MEDIUM_LEVEL( "Inside " + startingNtString +  ": no match for rule '" + ruleString + "'" );
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens

        // If there are leftover tokens and this is not allowed, consider this a non-match for this rule and continue
        // looping through further rules.
        if ( !allowLeftoverTokens )
        {
            if ( !tokensCopy.empty() )
            {
                LOG_INFO_MEDIUM_LEVEL( "Leftover tokens (" + Token::ConvertTokensToString( tokensCopy, 3 )
                                       + "...) at the end: rejecting rule '" + ruleString );
                continue;
            }
        }
        
        if ( elements.empty() )
        {
            std::string errMsg = "Rule match found for '" + ruleString + "' but no child nodes or tokens created.";
            LOG_ERROR( errMsg );
            throw std::runtime_error( errMsg );
        }

        // Swap out original tokens collection so it has all the correct tokens popped off the front
        tokens.swap( tokensCopy );

        // TODO: if only has 1 child:
        // If child is AstNode, return that instead
        // Else if child is Token, create node with the token

        LOG_INFO_MEDIUM_LEVEL( "Found match for '" + ruleString + "', creating AST node from children..." );
        // Construct an AST node from children
        return AstNode::CreateNodeFromRuleElements( elements, startingNt );
    }

    // If the loop is exited and no rule match has been found
    std::string errMsg = "No matching rule could be found for start symbol " + startingNtString + ": returning nullptr.";
    LOG_INFO_MEDIUM_LEVEL( errMsg );
    return nullptr;
}

/**
 * \brief  Tries to resolve a given rule (collection of symbols) from the given list of tokens. Pops tokens
 *         of the front of the given tokens container as it consumes rule symbols.
 *
 * \param[in,out]  tokens      String of tokens representing the code to be converted.
 * \param[in]      rule        The current rule that is being tested. Consists of symbols, either terminal or
 *                             non-terminal.
 * \param[in]      allowLeftoverTokens  Whether the parent rule allows leftover tokens.
 * \param[out]     elements    Child nodes or tokens belonging to the rule being tested.
 *
 * \return  True if the rule could successfully be matched to the tokens, false otherwise.
 */
bool
AstGenerator::TryRule(
    Tokens& tokens,
    const Rule& rule,
    bool allowLeftoverTokens,
    AstNode::Elements& elements
)
{
    std::string ruleString = GrammarRules::ConvertRuleToString( rule );
    LOG_INFO_MEDIUM_LEVEL( "Trying rule " + ruleString + " with tokens: " + Token::ConvertTokensToString( tokens, 3 )
                           + "..." );
    for ( size_t i = 0; i < rule.size(); ++i )
    {
        Symbol symbol = rule[i];

        std::string symbolString = GrammarSymbols::ConvertSymbolToString( symbol );
        LOG_INFO_MEDIUM_LEVEL( "Trying symbol '" + symbolString + "' in rule '" + ruleString + "'" );

        SymbolType symbolType = GrammarSymbols::GetSymbolType( symbol );

        // If symbol is terminal
        // If rule symbol matches the token currently being pointed at, increment token ptr and continue
        // If symbol not to be skipped (e.g. {}, (), ;), add the token to children
        if ( SymbolType::Terminal == symbolType )
        {
            LOG_INFO_MEDIUM_LEVEL( "Symbol is terminal: '" + symbolString + "'" );
            TokenType terminalSymbol = static_cast< TokenType >( symbol );
            Token::Ptr frontToken = tokens.front();
            if ( terminalSymbol != frontToken->m_type )
            {
                LOG_INFO_MEDIUM_LEVEL( "Symbol doesn't match current token " + frontToken->ToString() + ", rejecting rule." );
                return false;
            }
            else
            {
                LOG_INFO_MEDIUM_LEVEL( "Symbol matches current token " + frontToken->ToString() + "." );
            }

            // If token type not to be skipped, add to elements and continue through the rest of the rule.
            if ( GrammarSymbols::g_skipForAstTerminals.end() == GrammarSymbols::g_skipForAstTerminals.find( terminalSymbol ) )
            {
                elements.push_back( frontToken );
                tokens.pop_front();
                LOG_INFO_LOW_LEVEL( "Added '" + symbolString + "' to children." );
            }
            else
            {
                // If token type is to be skipped, pop off the front without doing anything else
                LOG_INFO_LOW_LEVEL( "Skipping token " + frontToken->ToString() );
                tokens.pop_front();
            }
            LOG_INFO_LOW_LEVEL( "Tokens are now: " + Token::ConvertTokensToString( tokens, 3 ) + "..." );
        }

        // Else if symbol non terminal, call get AST node on that rule name
        // Then add this to elements
        else if ( SymbolType::NonTerminal == symbolType )
        {
            LOG_INFO_MEDIUM_LEVEL( "Symbol is non-terminal: '" + symbolString + "'" );
            GrammarSymbols::NT nonTerminalSymbol = static_cast< NT >( symbol );
            // Generate sub-tree from the non-terminal symbol.
            
            // If parent rule is not allowed leftover tokens AND this is the last symbol in the rule,
            // disallow leftover tokens on the generated AST
            bool callWithAllowLeftoverTokens{ true };
            if ( !allowLeftoverTokens && i == rule.size()-1 )
            {
                callWithAllowLeftoverTokens = false;
            }

            LOG_INFO_MEDIUM_LEVEL( "Generating AST for '" + symbolString + "'" );
            AstNode::Ptr astNode = GenerateAst( tokens, nonTerminalSymbol, callWithAllowLeftoverTokens );
            if ( nullptr == astNode )
            {
                LOG_INFO_MEDIUM_LEVEL( "GenerateAst() returned nullptr. Rejecting rule '" + ruleString + "'" );
                return false;
            }
            LOG_INFO_MEDIUM_LEVEL( "Successfully generated AST for '" + symbolString + "': adding to elements." );
            LOG_INFO_LOW_LEVEL( "Front tokens are now: " + Token::ConvertTokensToString( tokens, 3 ) + "..." );
            elements.push_back( astNode );
            continue;
        }

        // If symbol type not recognised
        else
        {
            LOG_ERROR( "Symbol doesn't belong to T or NT sets." );
            throw std::runtime_error( "Symbol doesn't belong to T or NT sets." );
        }
    }

    // If no symbols have been rejected, the rule matches.
    LOG_INFO_MEDIUM_LEVEL( "No symbols rejected, returning true for rule '" + ruleString + "'" );
    return true;
}