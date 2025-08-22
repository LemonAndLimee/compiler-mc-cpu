/**
 * Contains definition of class responsible for generating intermediate representation of code, in three-address-
 * instruction form.
 */

#include "IntermediateCode.h"

IntermediateCode::IntermediateCode(
    AstNode::Ptr abstractSyntaxTree
)
: m_ast( abstractSyntaxTree )
{
}

/**
 * \brief  Converts the stored AST in this class to three-address-code instructions.
 *
 * \return  Vector of TAC instructions.
 */
IntermediateCode::Instructions
IntermediateCode::GenerateIntermediateCode()
{

}