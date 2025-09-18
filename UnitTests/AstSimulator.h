/**
 * Contains utility methods for creating simulated ASTs for test cases.
 */

#include "AstNode.h"

#pragma once

namespace AstSimulator
{
    enum IsDeclaration
    {
        TRUE = true,
        FALSE = false
    };
    AstNode::Ptr CreateAssignNodeFromByteValue( const std::string& varName, uint8_t value, IsDeclaration isDeclaration );
    AstNode::Ptr CreateAssignNodeFromVar( const std::string& varName, const std::string& valueVar, IsDeclaration isDeclaration );
    AstNode::Ptr CreateAssignStatementSubtree( const std::string& varName, Token::Ptr valueToken, IsDeclaration isDeclaration );

    AstNode::Ptr WrapNodesInBlocks( AstNode::Children nodes );
}