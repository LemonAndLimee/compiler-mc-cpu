/**
 * Contains utility methods for creating simulated ASTs for test cases.
 */

#include "AstNode.h"

#pragma once

namespace AstSimulator
{
    AstNode::Ptr CreateAssignNodeFromByteValue( const std::string& varName, bool isNewVar, uint8_t value );
    AstNode::Ptr CreateAssignNodeFromVar( const std::string& varName, bool isNewVar, const std::string& valueVar );
    AstNode::Ptr CreateAssignStatementSubtree( const std::string& varName, bool isNewVar, Token::Ptr valueToken );
}