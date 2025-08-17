/**
 * Declaration of class responsible for generating an Abstract Syntax Tree.
 */

#pragma once
#include "Grammar.h"
#include "AstNode.h"
#include <deque>
#include <utility>

using namespace GrammarRules;

class AstGenerator
{
public:
    using UPtr = std::unique_ptr< AstGenerator >;
    AstGenerator( const Tokens& tokens, GrammarSymbols::NT startingNt );

    AstNode::Ptr GenerateAst();

protected:
    // Used to populate a parsed deque. Stores a given symbol, its resolved AST element, and the index of the next token
    // after it.
    using ParsedSymbolInfo = std::tuple< Symbol, std::shared_ptr< AstNode::Element >, size_t >;

    AstNode::Ptr GenerateAstFromNt( size_t& currentTokenIndex,
                                    GrammarSymbols::NT nt,
                                    bool allowLeftoverTokens );

    bool TryRule( size_t& currentTokenIndex,
                  const Rule& rule,
                  bool allowLeftoverTokens,
                  AstNode::Elements& elementsToPopulate,
                  std::deque< ParsedSymbolInfo >& currentParsedDeque );

    // Stores the collection of tokens being parsed for this AST
    Tokens m_tokens;

    // Non-terminal symbol from which to start parsing the program.
    GrammarSymbols::NT m_startingNonTerminal;
};