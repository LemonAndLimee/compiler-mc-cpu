/**
 * Definition of class responsible for generating an Abstract Syntax Tree.
 */

#include "AstGenerator.h"
#include "Logger.h"
#include <stdexcept>

/**
 * \brief Generates an Abstract Syntax tree from a given set of tokens. If the syntax of the tokens
 *        is invalid, it returns nullptr.
 *
 * \param[in]      tokens               String of tokens representing the code to be converted.
 * \param[in,out]  tokenIndex           The index of the next token to try consume.
 * \param[in]      startingNt           Non-terminal symbol identifying the rule to start the tree generation from.
 * \param[in]      allowLeftoverTokens  If set to false, will return nullptr if there are leftover tokens after
 *                                      resolving the rule. Can be used for root node of AST, as well as ending NTs in
 *                                      a rule.
 *
 * \return  Pointer to the root of the generated tree. Nullptr if tokens are invalid.
 */
AstNode::Ptr
AstGenerator::GenerateAst(
    const Tokens& tokens,
    size_t& tokenIndex,
    GrammarSymbols::NT startingNt,
    bool allowLeftoverTokens
)
{
    std::string startingNtString = GrammarSymbols::ConvertSymbolToString( startingNt );
    if ( 0u == g_nonTerminalRuleSets.count( startingNt ) )
    {
        std::string errMsg = "Starting symbol " + startingNtString + " has no associated rules.";
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }

    LOG_INFO( "Generating AST for starting symbol " + startingNtString + "." );
    // Try each rule belonging to the starting NT symbol
    Rules rules =  g_nonTerminalRuleSets.find( startingNt )->second;
    for ( Rule currentRule : rules )
    {
        // Children of the node we are building
        std::vector< AstNode::Child > children;
        // Index of the current token we are consuming from the input collection.
        // Make copy so we can revert to the original value if the rule is rejected.
        size_t currentRuleTokenIndex{ tokenIndex };

        std::string ruleString = GrammarRules::ConvertRuleToString( currentRule );
        LOG_INFO( "Inside " + startingNtString + ": Trying rule '" + ruleString + "', token index: "
                  + std::to_string( currentRuleTokenIndex ) );
        // If rule doesn't match tokens list, ignore and continue
        if ( !TryRule( tokens, currentRule, children, currentRuleTokenIndex ) )
        {
            LOG_INFO( "No match for rule '" + ruleString + "', continuing..." );
            continue;
        }

        // If this point is reached, the current rule matches the list of tokens

        // If there are leftover tokens and this is not allowed, consider this a non-match for this rule and continue
        // looping through further rules.
        if ( !allowLeftoverTokens )
        {
            if ( tokens.size() <= currentRuleTokenIndex )
            {
                LOG_INFO( "Leftover tokens at the end: rejecting rule '" + ruleString + "'." );
                continue;
            }
        }
        
        if ( children.empty() )
        {
            std::string errMsg = "Rule match found for '" + ruleString + "' but no child nodes or tokens created.";
            LOG_ERROR( errMsg );
            throw std::runtime_error( errMsg );
        }

        LOG_INFO( "Found match for '" + ruleString + "', creating AST node..." );
        // Construct an AST node from children
        return CreateNodeFromChildren( children, startingNt );
    }

    // If the loop is exited and no rule match has been found
    std::string errMsg = "No matching rule could be found for start symbol " + startingNtString;
    LOG_INFO_LOW_LEVEL( errMsg + " and tokens '" + Token::ConvertTokensToString( tokens ) + "'");
    return nullptr;
}

/**
 * \brief  Tries to resolve a given rule (collection of symbols) from the given list of tokens.
 *
 * \param[in]      tokens      String of tokens representing the code to be converted.
 * \param[in]      rule        The current rule that is being tested. Consists of symbols, either terminal or
 *                             non-terminal.
 * \param[out]     children    Child nodes or tokens belonging to the rule being tested.
 * \param[in,out]  tokenIndex  The index of the next token to consume. Used by a calling
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
        std::string symbolString = GrammarSymbols::ConvertSymbolToString( symbol );
        LOG_INFO_LOW_LEVEL( "Trying symbol '" + symbolString + "', token index=" + std::to_string( tokenIndex ) );

        SymbolType symbolType = GrammarSymbols::GetSymbolType( symbol );

        // If symbol is terminal
        // If rule symbol matches the token currently being pointed at, increment token ptr and continue
        // If symbol not to be skipped (e.g. {}, (), ;), add the token to children
        if ( SymbolType::Terminal == symbolType )
        {
            LOG_INFO_LOW_LEVEL( "Symbol is terminal: '" + symbolString + "'" );
            TokenType terminalSymbol = static_cast< TokenType >( symbol );
            if ( terminalSymbol != tokens[tokenIndex]->m_type )
            {
                LOG_INFO_LOW_LEVEL( "Symbol doesn't match current token " + tokens[tokenIndex]->ToString() + ", rejecting rule." );
                return false;
            }

            // If token type not to be skipped, add to children and continue through the rest of the rule.
            if ( GrammarSymbols::g_skipForAstTerminals.end() == GrammarSymbols::g_skipForAstTerminals.find( terminalSymbol ) )
            {
                children.push_back( tokens[tokenIndex] );
                tokenIndex++;
                LOG_INFO_LOW_LEVEL( "Added '" + symbolString + "' to children, token index=" + std::to_string( tokenIndex ) );
                continue;
            }
        }

        // Else if symbol non terminal, call get AST node on that rule name
        // Then add this to children
        else if ( SymbolType::NonTerminal == symbolType )
        {
            LOG_INFO_LOW_LEVEL( "Symbol is non-terminal: '" + symbolString + "'" );
            GrammarSymbols::NT nonTerminalSymbol = static_cast< NT >( symbol );
            // Generate sub-tree from the non-terminal symbol, allowing leftover tokens.
            AstNode::Ptr astNode = GenerateAst( tokens, tokenIndex, nonTerminalSymbol, true );
            if ( nullptr == astNode )
            {
                LOG_INFO_LOW_LEVEL( "GenerateAst() returned nullptr. Rejecting current rule..." );
                return false;
            }
            LOG_INFO_LOW_LEVEL( "Generated AST node for '" + symbolString + "', adding to children..." );
            children.push_back( astNode );
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
    return true;
}

/**
 * \brief  Creates an AST node instance from a given set of child nodes/tokens. Assigns the node label according to
 *         the set of children. Children should be validated already so throws if anything is invalid.
 *
 * \param[in]  children  Child nodes or tokens belonging to the node being created.
 * \param[in]  nodeNt    The NT symbol to which this node's rule belongs. If an operator cannot be decided, this is
 *                       used as the node label.
 *
 * \return  Ptr to the created AST node.
 */
AstNode::Ptr
AstGenerator::CreateNodeFromChildren(
    const std::vector< AstNode::Child >& children,
    GrammarSymbols::NT nodeNt
)
{
    // Set to valid value if a terminal symbol label is found - if still invalid after all children have been
    // checked, the node NT will be used.
    GrammarSymbols::T terminalNodeLabel = T::INVALID_TOKEN;
    size_t terminalNodeLabelIndex;

    // Search through children for node label if exists - throws if more than 1 is found.
    for ( size_t i = 0; i < children.size(); ++i )
    {
        AstNode::Child child = children[i];
        // If child is a token
        if ( const Token::Ptr* pToken = std::get_if< Token::Ptr >( &child ) )
        {
            TokenType tokenType = ( *pToken )->m_type;
            // If token is node label type, mark it as such.
            if ( g_nodeLabelTerminals.end() != g_nodeLabelTerminals.find( tokenType ) )
            {
                if ( T::INVALID_TOKEN != terminalNodeLabel )
                {
                    std::string errMsg = "Creating node for " + GrammarSymbols::ConvertSymbolToString( nodeNt );
                    errMsg += ": gathered children have more than one node label type: ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( terminalNodeLabel ) + ", ";
                    errMsg += TokenTypes::ConvertTokenTypeToString( tokenType );
                    LOG_ERROR( errMsg );
                    throw std::runtime_error( "Creating node: children have >1 node label type." );
                }
                terminalNodeLabel = tokenType;
                terminalNodeLabelIndex = i;
            }
        }
    }

    // If a terminal node label was found, use it and remove it from children
    if ( T::INVALID_TOKEN != terminalNodeLabel )
    {
        std::vector< AstNode::Child > childrenCopy = children;
        childrenCopy.erase( childrenCopy.begin() + terminalNodeLabelIndex );
        return std::make_shared< AstNode >( terminalNodeLabel, childrenCopy );
    }
    else
    {
        // Else use NT label and original set of children
        return std::make_shared< AstNode >( nodeNt, children );
    }
}