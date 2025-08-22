/**
 * Contains declaration of class responsible for generating intermediate representation of code, in three-address-
 * instruction form.
 */

#pragma once

#include "AstNode.h"
#include "ThreeAddrInstruction.h"

/**
 * \brief  Class responsible for generating an intermediate representation of code. Takes an abstract syntax tree
 *         (which contains symbol tables for its scopes) and converts it into a list of TAC instructions.
 */
class IntermediateCode
{
public:
    using UPtr = std::unique_ptr< IntermediateCode >;
    using Instructions = std::vector< TAC::ThreeAddrInstruction::Ptr >;

    IntermediateCode( AstNode::Ptr abstractSyntaxTree );

    Instructions GenerateIntermediateCode();

protected:
    // Stored Abstract Syntax Tree, to convert into TAC instructions
    AstNode::Ptr m_ast;

    // Generated instructions list - is stored internally and updated as the AST is traversed.
    Instructions m_instructions;
};