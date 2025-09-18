/**
 * Contains definition of class responsible for converting an abstract syntax tree into three-address code.
 */

#include "IntermediateCode.h"

IntermediateCode::IntermediateCode(
    TacInstructionFactory::Ptr instrFactory,
    ITacExpressionGenerator::Ptr tacExprGenerator
)
: m_instructionFactory( instrFactory ),
  m_tacExpressionGenerator( tacExprGenerator )
{
}

 /**
  * \brief  Converts the given AST to three-address-code instructions, stored in the factory class.
  *
  * \param[in]  astNode  The root node of the AST being converted to TAC.
  */
void
IntermediateCode::GenerateIntermediateCode(
    AstNode::Ptr astNode
)
{
    if ( nullptr == astNode )
    {
        LOG_ERROR_AND_THROW( "Cannot generate intermediate code from a nullptr AST.", std::invalid_argument );
    }
    if ( nullptr == astNode->m_symbolTable )
    {
        LOG_ERROR_AND_THROW( "Can't generate intermediate code for an AST that doesn't have a symbol table.",
                             std::invalid_argument );
    }

    // Call internal method - this will handle error checking.
    ConvertAstToInstructions( astNode, astNode->m_symbolTable );
}

/**
 * \brief  Converts the given AST sub-tree to three-address-code instructions.
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertAstToInstructions(
    AstNode::Ptr astNode,
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
            ConvertAssign( astNode, currentSt );
            break;
        case T::IF:
            ConvertIfElse( astNode, currentSt );
            break;
        case T::FOR:
            ConvertForLoop( astNode, currentSt );
            break;
        case T::WHILE:
            ConvertWhileLoop( astNode, currentSt );
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
                ConvertAstToInstructions( child, currentSt );
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
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertAssign(
    AstNode::Ptr astNode,
    SymbolTable::Ptr currentSt
)
{
    if ( T::ASSIGN != astNode->m_nodeLabel )
    {
        LOG_ERROR_AND_THROW( "AST node has wrong label. Expected ASSIGN, got: "
                             + GrammarSymbols::ConvertSymbolToString( astNode->m_nodeLabel ), std::invalid_argument );
    }
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
    ExpressionInfo expressionInfo = GetExpressionInfo( rhsNode, currentSt );


    // Create assignment statement from the expression info, targeting the LHS identifier.
    Opcode opcode = std::get< Opcode >( expressionInfo );
    Operand operand1 = std::get< 1 >( expressionInfo );
    Operand operand2 = std::get< 2 >( expressionInfo );

    // If opcode is invalid, expect there to be a single RHS operand.
    if ( Opcode::INVALID == opcode )
    {
        if ( ThreeAddrInstruction::IsOperandEmpty( operand1 ) )
        {
            LOG_ERROR_AND_THROW( "For assignment to '" + uniqueLhsId + "': operand1 must be non-empty",
                                 std::runtime_error );
        }
        if ( !ThreeAddrInstruction::IsOperandEmpty( operand2 ) )
        {
            LOG_ERROR_AND_THROW( "For assignment to '" + uniqueLhsId + "': expected operand2 to be empty.",
                                 std::runtime_error );
        }
        m_instructionFactory->AddAssignmentInstruction( uniqueLhsId, operand1 );
    }
    else
    {
        m_instructionFactory->AddInstruction( uniqueLhsId, opcode, operand1, operand2 );
    }
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
    void* voidEntryPtr = static_cast< void* >( entry.get() );
    char stPointerBytes[17u]; // Size of pointer + 1 for terminating char
    sprintf_s( stPointerBytes, "%p", voidEntryPtr );
    std::string outputStr = currentIdentifier + std::string( stPointerBytes );
    return outputStr;
}

/**
 * \brief  Retrieves the relevant opcode and operand(s) from an AST representing an expression. If the expression
 *         contains sub-expressions, it generates instructions for temporary variables first (which will then be
 *         used for the operand(s)).
 *
 * \param[in]   expressionNode  The AST node representing the expression being converted.
 * \param[in]   currentSt       The current symbol table of this scope.
 *
 * \return  Expression info tuple containing the opcode and operand(s).
 */
IntermediateCode::ExpressionInfo
IntermediateCode::GetExpressionInfo(
    AstNode::Ptr expressionNode,
    SymbolTable::Ptr currentSt
)
{
    Opcode opcode{ Opcode::INVALID };
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
        ExpressionInfo lhsInfo = GetExpressionInfo( children[0], currentSt );
        Operand lhs = GetOperandFromExpressionInfo( lhsInfo );

        Operand rhs;
        if ( 2u == children.size() )
        {
            ExpressionInfo rhsInfo = GetExpressionInfo( children[1], currentSt );
            rhs = GetOperandFromExpressionInfo( rhsInfo );
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
                operand1 = m_tacExpressionGenerator->Multiply( lhs, rhs );
                break;
            case T::DIVIDE:
                operand1 = m_tacExpressionGenerator->Divide( lhs, rhs );
                break;
            case T::MOD:
                operand1 = m_tacExpressionGenerator->Modulo( lhs, rhs );
                break;
            case T::EQ:
                operand1 = m_tacExpressionGenerator->Equals( lhs, rhs );
                break;
            case T::NEQ:
                operand1 = m_tacExpressionGenerator->NotEquals( lhs, rhs );
                break;
            case T::LEQ:
                operand1 = m_tacExpressionGenerator->Leq( lhs, rhs );
                break;
            case T::GEQ:
                operand1 = m_tacExpressionGenerator->Geq( lhs, rhs );
                break;
            case T::LT:
                operand1 = m_tacExpressionGenerator->LessThan( lhs, rhs );
                break;
            case T::GT:
                operand1 = m_tacExpressionGenerator->GreaterThan( lhs, rhs );
                break;
            case T::NOT: // Logical NOT
                if ( !ThreeAddrInstruction::IsOperandEmpty( rhs ) )
                {
                    LOG_ERROR_AND_THROW( "Cannot generate intermediate code for NOT operation with 2 operands.",
                                         std::invalid_argument );
                }
                operand1 = m_tacExpressionGenerator->LogicalNot( lhs );
                break;
            case T::OR:  // Logical OR
                operand1 = m_tacExpressionGenerator->LogicalOr( lhs, rhs );
                break;
            case T::AND: // Logical AND
                operand1 = m_tacExpressionGenerator->LogicalAnd( lhs, rhs );
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
 * \param[in]  info  Information about the expression (if exists, the opcode and operand(s)).
 *
 * \return  Expression info tuple containing the opcode and operand(s).
 */
Operand
IntermediateCode::GetOperandFromExpressionInfo(
    ExpressionInfo info
)
{
    Opcode opcode = std::get< Opcode >( info );
    Operand operand1 = std::get< 1 >( info );
    Operand operand2 = std::get< 2 >( info );

    // If opcode isn't being used, it represents only a single value and should therefore be storing 1 operand only.
    if ( Opcode::INVALID == opcode )
    {
        if ( ThreeAddrInstruction::IsOperandEmpty( operand1 ) || !ThreeAddrInstruction::IsOperandEmpty( operand2 ) )
        {
            LOG_ERROR_AND_THROW( "Expression info with no opcode should hold one valid operand.", std::invalid_argument );
        }
        return operand1;
    }

    // If opcode is being used, we need to create an instruction to be stored in a temporary variable, which will then
    // become the returned operand.
    std::string tempVarId = m_instructionFactory->GetNewTempVar();
    m_instructionFactory->AddInstruction( tempVarId, opcode, operand1, operand2 );
    return tempVarId;
}

/**
 * \brief  Converts an AST representing an if/else statement to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertIfElse(
    AstNode::Ptr astNode,
    SymbolTable::Ptr currentSt
)
{
    if ( T::IF != astNode->m_nodeLabel )
    {
        LOG_ERROR_AND_THROW( "AST node has wrong label. Expected IF, got: "
                             + GrammarSymbols::ConvertSymbolToString( astNode->m_nodeLabel ), std::invalid_argument );
    }

    AstNode::Children children = astNode->GetChildren();
    // If there are 2 children, this means there is a condition and a block
    // If there are 3 children, there is an addition else, which contains a block
    if ( 2u > children.size() || 3u < children.size() )
    {
        LOG_ERROR_AND_THROW( "Trying to convert if/else statement: expected 2 or 3 children, got: "
                             + std::to_string( children.size() ), std::invalid_argument );
    }

    SymbolTable::Ptr ifSymbolTable = astNode->m_symbolTable;
    if ( !astNode->IsScopeDefiningNode() || nullptr == ifSymbolTable )
    {
        LOG_ERROR_AND_THROW( "'If' AST node has no symbol table.", std::invalid_argument );
    }

    // Get the condition
    AstNode::Ptr conditionNode = children[0];
    ExpressionInfo conditionExpressionInfo = GetExpressionInfo( conditionNode, ifSymbolTable );
    Operand conditionOperand = GetOperandFromExpressionInfo( conditionExpressionInfo );


    // Branch if NOT condition (i.e. if condition == 0)
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, conditionOperand, 0u );
    ThreeAddrInstruction::Ptr branchToElse = m_instructionFactory->GetLatestInstruction();

    // Add the if block instructions
    AstNode::Ptr ifBlockNode = children[1];
    ConvertAstToInstructions( ifBlockNode, ifSymbolTable );

    // Set the else label to be the next instruction - this will either point to the soon-to-be-added else block, or
    // the next instruction that gets added.
    m_instructionFactory->SetInstructionBranchToNextLabel( branchToElse, "else" );

    // If there is an else, add that block and attach the else label
    if ( 3u == children.size() )
    {
        AstNode::Ptr elseNode = children[2];
        if ( T::ELSE != elseNode->m_nodeLabel )
        {
            LOG_ERROR_AND_THROW( "AST node has wrong label. Expected ELSE, got: "
                                 + GrammarSymbols::ConvertSymbolToString( astNode->m_nodeLabel ),
                                 std::invalid_argument );
        }

        // Add unconditional jump to after the else block, in the case that the main if condition was true.
        m_instructionFactory->AddInstruction(
            TacInstructionFactory::PLACEHOLDER, Opcode::BRE, conditionOperand, conditionOperand
        );
        ThreeAddrInstruction::Ptr branchToEnd = m_instructionFactory->GetLatestInstruction();

        AstNode::Children elseChildren = elseNode->GetChildren();
        for ( auto child : elseChildren )
        {
            ConvertAstToInstructions( child, ifSymbolTable );
        }

        m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd, "skipElse" );
    }
}

/**
 * \brief  Converts an AST representing a for loop to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertForLoop(
    AstNode::Ptr astNode,
    SymbolTable::Ptr currentSt
)
{
    if ( T::FOR != astNode->m_nodeLabel )
    {
        LOG_ERROR_AND_THROW( "AST node has wrong label. Expected FOR, got: "
                             + GrammarSymbols::ConvertSymbolToString( astNode->m_nodeLabel ), std::invalid_argument );
    }

    // Expect for loop node to have 2 children: the initialising section, and the actual block.
    AstNode::Children children = astNode->GetChildren();
    if ( 2u != children.size() )
    {
        LOG_ERROR_AND_THROW( "Trying to convert for loop: expected 2 children, got: "
                             + std::to_string( children.size() ), std::invalid_argument );
    }

    AstNode::Ptr initNode = children[0];
    AstNode::Ptr blockNode = children[1];
    // The for init section should have 3 children: a statement (1), a comparison, and another statement (2).
    AstNode::Children initChildren = initNode->GetChildren();
    if ( 3u != initChildren.size() )
    {
        LOG_ERROR_AND_THROW( "Trying to convert for loop initialisation section: expected 3 children, got: "
                             + std::to_string( initChildren.size() ), std::invalid_argument );
    }
    AstNode::Ptr statement1 = initChildren[0];
    AstNode::Ptr comparison = initChildren[1];
    AstNode::Ptr statement2 = initChildren[2];

    SymbolTable::Ptr forSymbolTable = astNode->m_symbolTable;
    if ( !astNode->IsScopeDefiningNode() || nullptr == forSymbolTable )
    {
        LOG_ERROR_AND_THROW( "'For' AST node has no symbol table.", std::invalid_argument );
    }

    // The TAC code should look like this:

    // execute statement 1
    // condition: if comparison == 0 aka is false, jump to end
    // execute block
    // execute statement 2
    // jump to condition
    // end:

    ConvertAssign( statement1, forSymbolTable );

    std::string conditionLabel = m_instructionFactory->GetNewLabel( "forCondition" );
    m_instructionFactory->SetNextInstructionLabel( conditionLabel );
    // Evaluate comparison expression
    ExpressionInfo comparisonInfo = GetExpressionInfo( comparison, forSymbolTable );
    Operand comparisonOperand = GetOperandFromExpressionInfo( comparisonInfo );
    // If comparison == 0 aka comparison is false, branch to end
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, comparisonOperand, 0u );
    ThreeAddrInstruction::Ptr branchToEnd = m_instructionFactory->GetLatestInstruction();

    ConvertAstToInstructions( blockNode, forSymbolTable );
    ConvertAssign( statement2, forSymbolTable );

    // Unconditional branch using an operand we have access to, i.e. if x == x
    m_instructionFactory->AddInstruction( conditionLabel, Opcode::BRE, comparisonOperand, comparisonOperand );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd, "end" );
}

/**
 * \brief  Converts an AST representing a while loop to TAC instruction(s).
 *
 * \param[in]   astNode       The root node of the AST being converted to TAC.
 * \param[in]   currentSt     Current symbol table being used by the parent of this AST node.
 */
void
IntermediateCode::ConvertWhileLoop(
    AstNode::Ptr astNode,
    SymbolTable::Ptr currentSt
)
{
    if ( T::WHILE != astNode->m_nodeLabel )
    {
        LOG_ERROR_AND_THROW( "AST node has wrong label. Expected WHILE, got: "
                             + GrammarSymbols::ConvertSymbolToString( astNode->m_nodeLabel ), std::invalid_argument );
    }

    // Expect to hold 2 children: an expression, and a block.
    AstNode::Children children = astNode->GetChildren();
    if ( 2u != children.size() )
    {
        LOG_ERROR_AND_THROW( "Trying to convert while loop: expected 2 children, got: "
                             + std::to_string( children.size() ), std::invalid_argument );
    }
    AstNode::Ptr expressionNode = children[0];
    AstNode::Ptr blockNode = children[1];

    SymbolTable::Ptr whileSymbolTable = astNode->m_symbolTable;
    if ( !astNode->IsScopeDefiningNode() || nullptr == whileSymbolTable )
    {
        LOG_ERROR_AND_THROW( "'While' AST node has no symbol table.", std::invalid_argument );
    }

    // The TAC code should look like this:

    // condition: if expression is false (i.e. == 0), branch to end
    // execute block
    // jump to condition
    // end:

    std::string conditionLabel = m_instructionFactory->GetNewLabel( "whileCondition" );
    m_instructionFactory->SetNextInstructionLabel( conditionLabel );
    // Evaluate expression
    ExpressionInfo expressionInfo = GetExpressionInfo( expressionNode, whileSymbolTable );
    Operand expressionOperand = GetOperandFromExpressionInfo( expressionInfo );
    // If expression == 0 aka is false, branch to end
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, expressionOperand, 0u );
    ThreeAddrInstruction::Ptr branchToEnd = m_instructionFactory->GetLatestInstruction();

    ConvertAstToInstructions( blockNode, whileSymbolTable );

    // Unconditional branch using an operand we have access to, i.e. if x == x
    m_instructionFactory->AddInstruction( conditionLabel, Opcode::BRE, expressionOperand, expressionOperand );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd, "end" );
}