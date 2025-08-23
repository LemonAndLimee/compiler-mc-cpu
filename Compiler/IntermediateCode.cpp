/**
 * Contains definition of class responsible for generating intermediate representation of code, in three-address-
 * instruction form.
 */

#include "IntermediateCode.h"

 /**
  * \brief  Converts the given AST to three-address-code instructions.
  *
  * \param[in]  astNode  The root node of the AST being converted to TAC.
  *
  * \return  Vector of TAC instructions.
  */
IntermediateCode::Instructions
IntermediateCode::GenerateIntermediateCode(
    AstNode::Ptr astNode
)
{
    Instructions instructionsToPopulate{};

    if ( nullptr == astNode->m_symbolTable )
    {
        LOG_ERROR_AND_THROW( "Can't generate intermediate code for an AST that doesn't have a symbol table.",
                             std::invalid_argument );
    }

    // Call internal method - this will handle error checking.
    ConvertAstToInstructions( astNode, instructionsToPopulate, astNode->m_symbolTable );
    return instructionsToPopulate;
}

/**
 * \brief  Converts the given AST sub-tree to three-address-code instructions.
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[out]  instructions  The container of instructions to append to.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertAstToInstructions(
    AstNode::Ptr astNode,
    Instructions& instructions,
    SymbolTable::Ptr currentSt
)
{
    // AST validation
    if ( nullptr == astNode )
    {
        LOG_ERROR_AND_THROW( "Cannot generate intermediate code from a nullptr AST.", std::invalid_argument );
    }
    if ( astNode->IsStoringToken() )
    {
        LOG_ERROR_AND_THROW( "AST must be storing a valid program, not a token.", std::invalid_argument );
    }
    if ( !astNode->IsStorageInUse() )
    {
        LOG_ERROR_AND_THROW( "AST node storage not in use.", std::invalid_argument );
    }

    GrammarSymbols::Symbol nodeLabel = astNode->m_nodeLabel;
    std::string nodeLabelString = GrammarSymbols::ConvertSymbolToString( nodeLabel );

    // If node label is a terminal
    if ( SymbolType::Terminal == GrammarSymbols::GetSymbolType( nodeLabel ) )
    {
        // If AST is representing an operation, we can stop and convert this into instruction(s)
        switch ( nodeLabel )
        {
        case T::ASSIGN:
            ConvertAssign( astNode, instructions, currentSt );
            break;
        case T::IF:
            ConvertIfElse( astNode, instructions );
            break;
        case T::FOR:
            ConvertForLoop( astNode, instructions );
            break;
        case T::WHILE:
            ConvertWhileLoop( astNode, instructions );
            break;
        default:
            LOG_ERROR_AND_THROW( "Node label not suitable for an instruction: " + nodeLabelString,
                                 std::invalid_argument );
        }
    }

    // Else if node label is non-terminal, it represents a further sub-tree
    else if ( SymbolType::NonTerminal == GrammarSymbols::GetSymbolType( nodeLabel ) )
    {
        // "Block" is the only acceptable NT rule to call this method with, as all other NTs are unused in the AST, or
        // belong to a specific statement/loop that should be handled within the code regarding that specific operation.
        if ( NT::Block == nodeLabel )
        {
            for ( auto child : astNode->GetChildren() )
            {
                // No need to check symbol table here as any new scope should be introduced as part of a specific
                // operation, as described above - and should therefore be handled there.
                ConvertAstToInstructions( child, instructions, currentSt );
            }
        }
        else
        {
            LOG_ERROR_AND_THROW( "AST node has non-terminal label that is not valid for this operation: "
                                 + nodeLabelString, std::invalid_argument );
        }
    }

    else
    {
        LOG_ERROR_AND_THROW( "Unrecognised symbol: " + nodeLabelString, std::runtime_error );
    }
}

/**
 * \brief  Converts an AST representing an assignment statement to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[out]  instructions  The container of instructions to append to.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertAssign(
    AstNode::Ptr astNode,
    Instructions& instructions,
    SymbolTable::Ptr currentSt
)
{
    AstNode::Children children = astNode->GetChildren();
    // Expect there to be 2 children, a LHS and RHS
    if ( 2u != children.size() )
    {
        LOG_ERROR_AND_THROW( "Trying to convert assignment statement: unexpected number of children: "
                             + std::to_string( children.size() ), std::invalid_argument );
    }

    // LHS should be an identifier or a declaration of an identifier.
    AstNode::Ptr lhsNode = children[0];
    std::string identifier = GetIdentifierFromLhsNode( lhsNode );
    // TODO: calculate unique identifier using the symbol table, to get 'result' attribute.

    // RHS should either be a literal, an ID, or an expression (which may need breaking down further).
    AstNode::Ptr rhsNode = children[1];
    Instructions prerequisiteInstructions{};
    ExpressionInfo expressionInfo = ResolveExpression( rhsNode, prerequisiteInstructions );

    // TODO: if all operands are literals (e.g. = 1+2), resolve the operation here and store as literal.
    // TODO: if result is now storing a literal AND it is read-only in the ST, skip this instruction and
    // mark the identifier as replaced with a constant.

    LOG_ERROR_AND_THROW( "Not implemented yet!", std::runtime_error );
}

/**
 * \brief  Gets the identifier string from a LHS assignment node (i.e. an identifier node OR a variable sub-tree).
 *
 * \param[in]  lhsNode  The LHS node from which to retrieve the identifier.
 *
 * \return  The identifier string being stored by this node.
 */
std::string
IntermediateCode::GetIdentifierFromLhsNode(
    AstNode::Ptr lhsNode
)
{
    if ( T::IDENTIFIER == lhsNode->m_nodeLabel )
    {
        return lhsNode->GetToken()->m_value->m_value.stringValue;
    }
    else if ( NT::Variable == lhsNode->m_nodeLabel )
    {
        AstNode::Children varChildren = lhsNode->GetChildren();
        if ( 2u != varChildren.size() )
        {
            LOG_ERROR_AND_THROW( "Unexpected number of children for variable node: "
                                 + std::to_string( varChildren.size() ), std::invalid_argument );
        }
        if ( T::IDENTIFIER != varChildren[1]->m_nodeLabel )
        {
            LOG_ERROR_AND_THROW( "Expected an identifier node in variable sub-tree, got: "
                                 + GrammarSymbols::ConvertSymbolToString( varChildren[1]->m_nodeLabel ),
                                 std::invalid_argument );
        }
        return varChildren[1]->GetToken()->m_value->m_value.stringValue;
    }
    else
    {
        LOG_ERROR_AND_THROW( "Unrecognised LHS node label: "
                             + GrammarSymbols::ConvertSymbolToString( lhsNode->m_nodeLabel ),
                             std::invalid_argument );
    }
}

/**
 * \brief  Retrieves the relevant opcode and operand(s) from an AST representing an expression. If the expression
 *         contains sub-expressions, it generates instructions for temporary variables first (which will then be
 *         used for the operand(s)).
 *
 * \param[in]   expressionNode   The AST node representing the expression being converted.
 * \param[out]  preInstructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Expression info tuple containing the opcode and operand(s).
 */
IntermediateCode::ExpressionInfo
IntermediateCode::ResolveExpression(
    AstNode::Ptr expressionNode,
    Instructions& preInstructions
)
{
    TAC::Opcode opcode{ TAC::Opcode::UNUSED };
    TAC::Operand operand1{};
    TAC::Operand operand2{};

    if ( T::BYTE == expressionNode->m_nodeLabel )
    {
        operand1 = expressionNode->GetToken()->m_value->m_value.numericValue;
    }
    // TODO:
    // If node is identifier, check if has been replaced (use new global id) and substitute with its literal.
    // If node is expression, fetch opcode and operands - if any child is a sub-expression, call recursively.

    return std::make_tuple( opcode, operand1, operand2 );
}

/**
 * \brief  Converts an AST representing an if/else statement to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[out]  instructions  The container of instructions to append to.
 */
void
IntermediateCode::ConvertIfElse(
    AstNode::Ptr astNode,
    Instructions& instructions
)
{
    LOG_ERROR_AND_THROW( "Not implemented yet!", std::runtime_error );
}

/**
 * \brief  Converts an AST representing a for loop to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[out]  instructions  The container of instructions to append to.
 */
void
IntermediateCode::ConvertForLoop(
    AstNode::Ptr astNode,
    Instructions& instructions
)
{
    LOG_ERROR_AND_THROW( "Not implemented yet!", std::runtime_error );
}

/**
 * \brief  Converts an AST representing a while loop to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[out]  instructions  The container of instructions to append to.
 */
void
IntermediateCode::ConvertWhileLoop(
    AstNode::Ptr astNode,
    Instructions& instructions
)
{
    LOG_ERROR_AND_THROW( "Not implemented yet!", std::runtime_error );
}