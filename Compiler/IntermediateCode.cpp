/**
 * Contains definition of class responsible for converting an abstract syntax tree into three-address code.
 */

#include "IntermediateCode.h"

IntermediateCode::IntermediateCode(
    TacGenerator::Ptr tacGenerator
)
: m_tacGenerator( tacGenerator )
{
}

 /**
  * \brief  Converts the given AST to three-address-code instructions.
  *
  * \param[in]  astNode  The root node of the AST being converted to TAC.
  *
  * \return  Vector of TAC instructions.
  */
Instructions
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
    // Calculate unique identifier using the symbol table, to get 'result' attribute.
    std::string uniqueLhsId = CalculateUniqueIdentifier( identifier, currentSt );

    // RHS should either be a literal, an ID, or an expression (which may need breaking down further).
    AstNode::Ptr rhsNode = children[1];
    Instructions prerequisiteInstructions{};
    ExpressionInfo expressionInfo = GetExpressionInfo( rhsNode, currentSt, prerequisiteInstructions );


    // Create assignment statement from the expression info, targeting the LHS identifier.
    Opcode opcode = std::get< Opcode >( expressionInfo );
    Operand operand1 = std::get< 1 >( expressionInfo );
    Operand operand2 = std::get< 2 >( expressionInfo );

    instructions.push_back( std::make_shared< ThreeAddrInstruction >( uniqueLhsId, opcode, operand1, operand2 ) );
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
    LOG_ERROR_AND_THROW( "Unrecognised LHS node label: "
                         + GrammarSymbols::ConvertSymbolToString( lhsNode->m_nodeLabel ),
                         std::invalid_argument );
    return ""; // Added to satisfy compiler, will never be reached due to exception
}

/**
 * \brief  Generates a unique global identifier for a given identifier in a specific symbol table, that can be used to
 *         identify it regardless of scope. Combines the two to produce a string.
 *
 * \param[in]  currentIdentifier  The original identifier, as represented in the AST.
 * \param[in]  symbolTable        Pointer to the symbol table corresponding to this instance of the identifier.
 *
 * \return  The unique identifier string calculated.
 */
std::string
IntermediateCode::CalculateUniqueIdentifier(
    const std::string& currentIdentifier,
    SymbolTable::Ptr symbolTable
)
{
    // Use the pointer of the specific symbol table entry - if we use the table itself then a child scope of a variable
    // will produce a different unique ID.
    SymbolTableEntry::Ptr entry = symbolTable->GetEntryIfExists( currentIdentifier );
    if ( nullptr == entry )
    {
        LOG_ERROR_AND_THROW( "Could not find entry for '" + currentIdentifier + "'.", std::runtime_error );
    }
    char* stPointerBytes = static_cast< char* >( static_cast< void* >( entry.get() ) );
    std::string outputStr = currentIdentifier + std::string( stPointerBytes );
    return outputStr;
}

/**
 * \brief  Retrieves the relevant opcode and operand(s) from an AST representing an expression. If the expression
 *         contains sub-expressions, it generates instructions for temporary variables first (which will then be
 *         used for the operand(s)).
 *
 * \param[in]   expressionNode   The AST node representing the expression being converted.
 * \param[in]   currentSt        The current symbol table of this scope.
 * \param[out]  preInstructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Expression info tuple containing the opcode and operand(s).
 */
IntermediateCode::ExpressionInfo
IntermediateCode::GetExpressionInfo(
    AstNode::Ptr expressionNode,
    SymbolTable::Ptr currentSt,
    Instructions& preInstructions
)
{
    Opcode opcode{ Opcode::UNUSED };
    Operand operand1{};
    Operand operand2{};

    GrammarSymbols::Symbol nodeLabel = expressionNode->m_nodeLabel;

    if ( T::BYTE == nodeLabel )
    {
        operand1 = expressionNode->GetToken()->m_value->m_value.numericValue;
    }
    else if ( T::IDENTIFIER == nodeLabel )
    {
        std::string identifier = expressionNode->GetToken()->m_value->m_value.stringValue;
        std::string uniqueId = CalculateUniqueIdentifier( identifier, currentSt );
        operand1 = uniqueId;
    }
    // If node is expression, fetch opcode and operands - if any child is a sub-expression, call recursively.
    else
    {
        // First resolve the operands themselves, as they may need prerequisite instructions
        AstNode::Children children = expressionNode->GetChildren();
        ExpressionInfo lhsInfo = GetExpressionInfo( children[0], currentSt, preInstructions );
        Operand lhs = GetOperandFromExpressionInfo( lhsInfo, preInstructions );

        Operand rhs;
        if ( 2u == children.size() )
        {
            ExpressionInfo rhsInfo = GetExpressionInfo( children[1], currentSt, preInstructions );
            Operand rhs = GetOperandFromExpressionInfo( rhsInfo, preInstructions );
        }

        // For opcodes that directly map e.g. ADD, this is more simple
        if ( g_symbolsToOpcodesMap.end() != g_symbolsToOpcodesMap.find( nodeLabel ) )
        {
            opcode = g_symbolsToOpcodesMap.find( nodeLabel )->second;
            operand1 = lhs;
            operand2 = rhs;
        }
        // For more complex cases like divide, will need to generate pre-instructions
        else
        {
            // Call on the TAC generator to create pre-instructions, and return an operand pointing to where the result
            // is being stored. Use this returned operand to return an assignment expression from this method.
            switch ( nodeLabel )
            {
            case T::MULTIPLY:
                operand1 = m_tacGenerator->Multiply( lhs, rhs, preInstructions );
                break;
            case T::DIVIDE:
                operand1 = m_tacGenerator->Divide( lhs, rhs, preInstructions );
                break;
            case T::MOD:
                operand1 = m_tacGenerator->Modulo( lhs, rhs, preInstructions );
                break;
            case T::EQ:
                operand1 = m_tacGenerator->Equals( lhs, rhs, preInstructions );
                break;
            case T::NEQ:
                operand1 = m_tacGenerator->NotEquals( lhs, rhs, preInstructions );
                break;
            case T::LEQ:
                operand1 = m_tacGenerator->Leq( lhs, rhs, preInstructions );
                break;
            case T::GEQ:
                operand1 = m_tacGenerator->Geq( lhs, rhs, preInstructions );
                break;
            case T::LT:
                operand1 = m_tacGenerator->LessThan( lhs, rhs, preInstructions );
                break;
            case T::GT:
                operand1 = m_tacGenerator->GreaterThan( lhs, rhs, preInstructions );
                break;
            case T::NOT: // Logical NOT
                if ( !std::holds_alternative< std::monostate >( rhs ) )
                {
                    LOG_ERROR_AND_THROW( "Cannot generate intermediate code for NOT operation with 2 operands.",
                                         std::invalid_argument );
                }
                operand1 = m_tacGenerator->LogicalNot( lhs, preInstructions );
                break;
            case T::OR:  // Logical OR
                operand1 = m_tacGenerator->LogicalOr( lhs, rhs, preInstructions );
                break;
            case T::AND: // Logical AND
                operand1 = m_tacGenerator->LogicalAnd( lhs, rhs, preInstructions );
                break;
            default:
                LOG_ERROR_AND_THROW( "Invalid or unrecognised node label for expression: "
                                     + GrammarSymbols::ConvertSymbolToString( nodeLabel ), std::invalid_argument );
                break;
            }
        }
    }

    return std::make_tuple( opcode, operand1, operand2 );
}

/**
 * \brief  Takes information gathered about an expression and returns an operand value (either the direct single value,
 *         or a temporary variable through creating an assignment statement.
 *
 * \param[in]   info          Information about the expression (if exists, the opcode and operand(s)).
 * \param[out]  instructions  Container in which any generated instructions for temporary variables are stored.
 *
 * \return  Expression info tuple containing the opcode and operand(s).
 */
Operand
IntermediateCode::GetOperandFromExpressionInfo(
    ExpressionInfo info,
    Instructions& instructions
)
{
    Opcode opcode = std::get< Opcode >( info );
    Operand operand1 = std::get< 1 >( info );
    Operand operand2 = std::get< 2 >( info );

    // If opcode isn't being used, it represents only a single value and should therefore be storing 1 operand only.
    if ( Opcode::UNUSED == opcode )
    {
        if ( std::holds_alternative< std::monostate >( operand1 ) || !std::holds_alternative< std::monostate >( operand2 ) )
        {
            LOG_ERROR_AND_THROW( "Expression info with no opcode should hold one valid operand.", std::invalid_argument );
        }
        return operand1;
    }

    // If opcode is being used, we need to create an assignment instruction for a temporary variable, which will then
    // become the returned operand.
    std::string tempVarId = m_tacGenerator->GetNewTempVar();
    ThreeAddrInstruction::Ptr instruction
        = std::make_shared< ThreeAddrInstruction >( tempVarId, opcode, operand1, operand2 );
    instructions.push_back( instruction );
    return tempVarId;
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
    // TODO: implement
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
    // TODO: implement
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
    // TODO: implement
    LOG_ERROR_AND_THROW( "Not implemented yet!", std::runtime_error );
}