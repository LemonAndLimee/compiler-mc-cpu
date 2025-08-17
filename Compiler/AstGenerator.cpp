/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include "Logger.h"
#include <stdexcept>

AstGenerator::AstGenerator(
    const Tokens& tokens,
    GrammarSymbols::NT startingNt
)
: m_tokens( tokens ),
  m_startingNonTerminal( startingNt )
{
}

/**
 * \brief Generates an Abstract Syntax tree from the class's stored set of tokens.
 *
 * \return  Pointer to the root of the generated tree. Nullptr if tokens are invalid.
 */
AstNode::Ptr
AstGenerator::GenerateAst()
{
    // TODO: consider an algorithm that is token-led, rather than one which checks each available rule for a match.
    // The current algorithm has a lot of excess checks, and is almost impossible to use to locate the syntax error.

    if ( m_tokens.empty() )
    {
        LOG_ERROR_AND_THROW( "Cannot generate AST from zero tokens.", std::invalid_argument );
    }

    std::string startingNtString = GrammarSymbols::ConvertSymbolToString( m_startingNonTerminal );
    if ( 0u == g_nonTerminalRuleSets.count( m_startingNonTerminal ) )
    {
        LOG_ERROR_AND_THROW( "Starting symbol " + startingNtString + " has no associated rules.", std::invalid_argument );
    }

    size_t currentTokenIndex{ 0u };
    constexpr bool allowLeftoverTokens{ false };
    return GenerateAstFromNt( currentTokenIndex, m_startingNonTerminal, allowLeftoverTokens );
}

/**
 * \brief Generates an Abstract Syntax tree from the class's stored set of tokens. If the syntax of the tokens
 *        is invalid, it returns nullptr.
 *
 * \param[in,out]  currentTokenIndex    Index of the next token to parse. Is only modified upon successful AST creation.
 * \param[in]      nt                   Non-terminal symbol identifying the rule to start the tree generation from.
 * \param[in]      allowLeftoverTokens  If set to false, will return nullptr if there are leftover tokens after
 *                                      resolving the rule. Can be used for root node of AST, as well as ending NTs in
 *                                      a rule.
 *
 * \return  Pointer to the root of the generated tree. Nullptr if tokens are invalid.
 */
AstNode::Ptr
AstGenerator::GenerateAstFromNt(
    size_t& currentTokenIndex,
    GrammarSymbols::NT nt,
    bool allowLeftoverTokens
)
{
    std::string startingNtString = GrammarSymbols::ConvertSymbolToString( nt );
    LOG_INFO_MEDIUM_LEVEL( "Generating AST for starting symbol " + startingNtString );

    if ( m_tokens.empty() )
    {
        LOG_ERROR_AND_THROW( "Cannot generate AST from zero tokens.", std::runtime_error );
    }
    LOG_INFO_LOW_LEVEL( "Tokens: " + Token::ConvertTokensToString( m_tokens, currentTokenIndex, 3 )
                        + "... Allow leftover tokens=" + std::to_string( allowLeftoverTokens ) );

    if ( 0u == g_nonTerminalRuleSets.count( nt ) )
    {
        LOG_ERROR_AND_THROW( "Starting symbol " + startingNtString + " has no associated rules.", std::runtime_error );
    }

    // Deque used to store symbols that have already been parsed correctly. This is to allow backtracking on a rule,
    // to try another that branches off from a certain point.
    std::deque< ParsedSymbolInfo > parsedStack{};

    // Try each rule belonging to the starting NT symbol
    Rules rules =  g_nonTerminalRuleSets.find( nt )->second;
    for ( Rule currentRule : rules )
    {
        // Child elements of the node we are building
        AstNode::Elements elements;

        // Make copy of token index to try this rule with
        size_t tokenIndexCopy = currentTokenIndex;

        std::string ruleString = GrammarRules::ConvertRuleToString( currentRule );
        LOG_INFO_MEDIUM_LEVEL( "Inside " + startingNtString + ": trying rule: " + ruleString );
        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokenIndexCopy, currentRule, allowLeftoverTokens, elements, parsedStack ) )
        {
            LOG_INFO_MEDIUM_LEVEL( "Inside " + startingNtString +  ": no match for rule '" + ruleString + "'" );
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens

        // If there are leftover tokens and this is not allowed, consider this a non-match for this rule and continue
        // looping through further rules.
        if ( !allowLeftoverTokens )
        {
            if ( tokenIndexCopy < m_tokens.size() )
            {
                LOG_INFO_MEDIUM_LEVEL( "Leftover tokens (" + Token::ConvertTokensToString( m_tokens, tokenIndexCopy, 3 )
                                       + "...) at the end: rejecting rule '" + ruleString );
                continue;
            }
        }

        if ( elements.empty() )
        {
            LOG_ERROR_AND_THROW( "Rule match found for '" + ruleString + "' but no child nodes or tokens created.",
                                 std::runtime_error );
        }

        // Modify the output parameter to reflect the new token index, as the rule match was a success
        currentTokenIndex = tokenIndexCopy;

        LOG_INFO_MEDIUM_LEVEL( "Found match for '" + ruleString + "', creating AST node from children..." );
        // Construct an AST node from children
        return AstNode::GetNodeFromRuleElements( elements, nt );
    }

    // If the loop is exited and no rule match has been found
    std::string errMsg = "No matching rule could be found for start symbol " + startingNtString + ": returning nullptr.";
    LOG_INFO_MEDIUM_LEVEL( errMsg );
    return nullptr;
}

/**
 * \brief  Tries to resolve a given rule (collection of symbols) from the stored list of tokens. Increments the token
 *         index as it consumes rule symbols. Populates the container of AST elements with gathered nodes/tokens from
 *         the rule.
 *
 * \param[in,out]  currentTokenIndex                Index of the next token to parse.
 * \param[in]      rule                             The current rule that is being tested. Consists of symbols,
 *                                                  either terminal or non-terminal.
 * \param[in]      allowLeftoverTokensOnLastSymbol  Whether to allow leftover tokens at the end of the rule,
 *                                                  if the last symbol is non-terminal.
 * \param[out]     elementsToPopulate               Child nodes or tokens belonging to the rule being tested.
 * \param[in,out]  currentParsedDeque               Deque of already verified symbols for the current set of
 *                                                  rules, and the associated AST element + next token index. While
 *                                                  this is implemented as a deque, this is to allow reading from
 *                                                  the bottom - it should function as a stack as far as pushing
 *                                                  and popping elements.
 *
 * \return  True if the rule could successfully be matched to the tokens, false otherwise.
 */
bool
AstGenerator::TryRule(
    size_t& currentTokenIndex,
    const Rule& rule,
    bool allowLeftoverTokensOnLastSymbol,
    AstNode::Elements& elementsToPopulate,
    std::deque< ParsedSymbolInfo >& currentParsedDeque
)
{
    std::string ruleString = GrammarRules::ConvertRuleToString( rule );
    LOG_INFO_MEDIUM_LEVEL( "Trying rule " + ruleString + " with tokens: "
                           + Token::ConvertTokensToString( m_tokens, currentTokenIndex, 3 )
                           + "..." );

    Rule ruleWorkingCopy = rule;

    // If there are parsed symbols from a previous rule check, check for an overlap with the start of this rule
    if ( 0u < currentParsedDeque.size() )
    {
        // If the front symbol of the rule matches the bottom of the stack, it can be skipped.
        size_t dequeIndex{ 0u };
        while ( !ruleWorkingCopy.empty()
                && dequeIndex < currentParsedDeque.size()
                && ruleWorkingCopy[0] == std::get< Symbol >( currentParsedDeque[dequeIndex] ) )
        {
            std::string symbolString = GrammarSymbols::ConvertSymbolToString( ruleWorkingCopy[0] );
            LOG_INFO_MEDIUM_LEVEL( "Skipping symbol '" + symbolString + "' as it was parsed by a previous attempt.");
            ruleWorkingCopy.erase( ruleWorkingCopy.begin() );
            // Update index to skip past the token(s) for the already verified element
            currentTokenIndex = std::get< size_t >( currentParsedDeque[dequeIndex] );
            // Add the previously created AST element to elements
            std::shared_ptr< AstNode::Element > storedElement
                = std::get< std::shared_ptr< AstNode::Element > >( currentParsedDeque[dequeIndex] );
            if ( storedElement != nullptr )
            {
                elementsToPopulate.push_back( *storedElement.get() );
            }
            // Increment deque index, as this symbol has now been skipped
            dequeIndex++;
        }

        // Pop the remaining non-reusable symbols off the back of the deque - the back will match the first
        // rule element again once the excess symbols have been popped.
        if ( !ruleWorkingCopy.empty() )
        {
            // Deque index currently describes how many entries to keep
            currentParsedDeque.resize( dequeIndex );
        }
    }

    // If there are still symbols to parse
    if ( !ruleWorkingCopy.empty() )
    {
        // Perform look-ahead, and check that any terminal symbols in the rule are present, and in order.
        // This allows early-stopping without generating further sub-trees.
        size_t indexToStartLookahead{ currentTokenIndex };
        for ( Symbol symbol : rule )
        {
            if ( SymbolType::Terminal == GrammarSymbols::GetSymbolType( symbol ) )
            {
                // Check through tokens, from the position of the last found terminal, to see if the current
                // terminal symbol exists.
                bool foundSymbol{ false };
                for ( size_t tokenIndex = indexToStartLookahead; tokenIndex < m_tokens.size(); ++tokenIndex )
                {
                    Token::Ptr token = m_tokens[tokenIndex];
                    if ( symbol == token->m_type )
                    {
                        indexToStartLookahead = tokenIndex;
                        foundSymbol = true;
                        break;
                    }
                }
                if ( !foundSymbol )
                {
                    std::string symbolString = GrammarSymbols::ConvertSymbolToString( symbol );
                    LOG_INFO_MEDIUM_LEVEL( "Lookahead: symbol " + symbolString + " could not be found. Rejecting rule "
                                           + ruleString );
                    return false;
                }
            }
        }

        // Check each symbol in the rule for a match
        for ( size_t i = 0; i < ruleWorkingCopy.size(); ++i )
        {
            Symbol symbol = ruleWorkingCopy[i];

            std::string symbolString = GrammarSymbols::ConvertSymbolToString( symbol );
            LOG_INFO_MEDIUM_LEVEL( "Trying symbol '" + symbolString + "' in rule '" + ruleString + "'" );

            // Reject if we've run out of tokens to consume
            if ( currentTokenIndex >= m_tokens.size() )
            {
                LOG_INFO_MEDIUM_LEVEL( "Run out of tokens to consume: rejecting rule " + ruleString );
                return false;
            }

            SymbolType symbolType = GrammarSymbols::GetSymbolType( symbol );

            // If a token or node was found/created for the current symbol (false if symbol was skipped).
            bool foundElementToStore{ false };
            AstNode::Element elementToStore;

            // If symbol is terminal
            if ( SymbolType::Terminal == symbolType )
            {
                LOG_INFO_LOW_LEVEL( "Symbol is terminal: '" + symbolString + "'" );
                TokenType terminalSymbol = static_cast< TokenType >( symbol );
                Token::Ptr currentToken = m_tokens[currentTokenIndex];

                // Check rule symbol matches the token currently being pointed at
                if ( terminalSymbol != currentToken->m_type )
                {
                    LOG_INFO_MEDIUM_LEVEL( "Symbol doesn't match current token " + currentToken->ToString() + ", rejecting rule." );
                    return false;
                }
                LOG_INFO_MEDIUM_LEVEL( "Symbol matches current token " + currentToken->ToString() + "." );

                // If token type not to be skipped (e.g. {}, (), ;), add to elements and continue through the rest of the rule.
                if ( GrammarSymbols::g_skipForAstTerminals.end() == GrammarSymbols::g_skipForAstTerminals.find( terminalSymbol ) )
                {
                    elementToStore = currentToken;
                    foundElementToStore = true;
                    LOG_INFO_LOW_LEVEL( "Adding '" + symbolString + "' to elements." );
                }
                else
                {
                    // If token type is to be skipped, pop off the front without doing anything else
                    LOG_INFO_LOW_LEVEL( "Skipping token " + currentToken->ToString() );
                }
                currentTokenIndex++;
            }

            // Else if symbol non terminal, call get AST node on that rule name
            else if ( SymbolType::NonTerminal == symbolType )
            {
                LOG_INFO_LOW_LEVEL( "Symbol is non-terminal: '" + symbolString + "'" );
                GrammarSymbols::NT nonTerminalSymbol = static_cast< NT >( symbol );

                // If not allowed leftover tokens AND this is the last symbol in the rule,
                // disallow leftover tokens on the generated AST
                bool callWithAllowLeftoverTokens{ true };
                if ( !allowLeftoverTokensOnLastSymbol && i == ruleWorkingCopy.size()-1 )
                {
                    callWithAllowLeftoverTokens = false;
                }

                // Generate sub-tree from the non-terminal symbol and add to elements.
                LOG_INFO_LOW_LEVEL( "Generating AST for '" + symbolString + "'" );

                size_t tokenIndexCopy = currentTokenIndex;
                AstNode::Ptr astNode = GenerateAstFromNt( tokenIndexCopy, nonTerminalSymbol, callWithAllowLeftoverTokens );

                if ( nullptr == astNode )
                {
                    LOG_INFO_MEDIUM_LEVEL( "GenerateAst() returned nullptr. Rejecting rule '" + ruleString + "'" );
                    return false;
                }

                LOG_INFO_MEDIUM_LEVEL( "Successfully generated AST for '" + symbolString + "': adding to elements." );
                elementToStore = astNode;
                foundElementToStore = true;

                currentTokenIndex = tokenIndexCopy;
                LOG_INFO_LOW_LEVEL( "Front tokens are now: "
                                    + Token::ConvertTokensToString( m_tokens, currentTokenIndex, 3 ) + "..." );
            }

            // If symbol type not recognised
            else
            {
                LOG_ERROR_AND_THROW( "Symbol doesn't belong to T or NT sets.", std::runtime_error );
            }

            // Push parsed symbol info to the deque of parsed symbols
            std::shared_ptr< AstNode::Element > pElement{ nullptr };
            if ( foundElementToStore )
            {
                pElement = std::make_shared< AstNode::Element >( elementToStore );
                elementsToPopulate.push_back( elementToStore );
            }
            currentParsedDeque.push_back( std::make_tuple( symbol, pElement, currentTokenIndex ) );
        }

    }

    // If no symbols have been rejected, the rule matches.
    LOG_INFO_LOW_LEVEL( "No symbols rejected, returning true for rule '" + ruleString + "'" );
    return true;
}