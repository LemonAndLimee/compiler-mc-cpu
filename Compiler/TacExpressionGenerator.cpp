/**
 * Contains definition of class responsible for generating three-address code for complex operations.
 */

#include "TacExpressionGenerator.h"

TacExpressionGenerator::TacExpressionGenerator( TacInstructionFactory::Ptr instrFactory )
: m_instructionFactory( instrFactory )
{
}

/**
 * \brief  Generates the instructions needed to for the multiplication operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]  op1  The first operand to be multiplied.
 * \param[in]  op2  The second operand to be multiplied.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Multiply(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for multiplication must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) * std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * result = 0
     * multiplier = op1
     * multiplicand = op2
     * bitCounter = 8
     *
     * loop: lsb = multiplier && 0xFE
     * BRE shift lsb 0
     * result = result + multiplicand
     * shift: multiplicand = << multiplicand
     * multiplier = >> multiplier
     * bitCounter = bitCounter - 1
     * BRLT loop 0 bitCounter
     *
     * (return result)
     */

    // Temp vars declarations

    std::string result = m_instructionFactory->GetNewTempVar( "multResult" );
    constexpr uint8_t resultInit{ 0u };
    m_instructionFactory->AddAssignmentInstruction( result, resultInit );

    // Copy operands into new temp vars because the values are edited.
    std::string multiplier = m_instructionFactory->GetNewTempVar( "multiplier" );
    m_instructionFactory->AddAssignmentInstruction( multiplier, op1 );
    std::string multiplicand = m_instructionFactory->GetNewTempVar( "multiplicand" );
    m_instructionFactory->AddAssignmentInstruction( multiplicand, op2 );

    std::string bitCounter = m_instructionFactory->GetNewTempVar( "bitCounter" );
    constexpr uint8_t bitCtInit{ 8u }; // 8 bits in a byte, which is our current supported literal length.
    m_instructionFactory->AddAssignmentInstruction( bitCounter, bitCtInit );


    // Main loop
    std::string mainLoopLabel = m_instructionFactory->GetNewLabel( "multLoop" );
    m_instructionFactory->SetNextInstructionLabel( mainLoopLabel );
    std::string lsb = m_instructionFactory->GetNewTempVar( "lsb" ); // Use bitmask to retrieve the LSB, in bit form.
    constexpr uint8_t lsbBitmask{ 0xfe };
    m_instructionFactory->AddInstruction( lsb, Opcode::AND, multiplier, lsbBitmask );

    std::string shiftLabel = m_instructionFactory->GetNewLabel( "shift" );
    m_instructionFactory->AddInstruction( shiftLabel, Opcode::BRE, lsb, 0u );

    m_instructionFactory->AddInstruction( result, Opcode::ADD, result, multiplicand );

    // The location of the BRZ jump - shift the multiplier and multiplicand to move onto the next LSB
    m_instructionFactory->SetNextInstructionLabel( shiftLabel );
    m_instructionFactory->AddSingleOperandInstruction( multiplicand, Opcode::LS, multiplicand );
    m_instructionFactory->AddSingleOperandInstruction( multiplier, Opcode::RS, multiplier );

    constexpr uint8_t decrement{ 1u };
    m_instructionFactory->AddInstruction( bitCounter, Opcode::SUB, bitCounter, decrement );

    m_instructionFactory->AddInstruction( mainLoopLabel, Opcode::BRLT, 0u, bitCounter );

    return result;
}

/**
 * \brief  Generates the instructions needed for the division operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]  op1  The first operand (the dividend/numerator).
 * \param[in]  op2  The second operand (the quotient/denominator).
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Divide(
    Operand op1,
    Operand op2
)
{
    return AddDivModInstructions( op1, op2, DivMod::DIV );
}

/**
 * \brief  Generates the instructions needed for the modulo operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]  op1  The first operand (the dividend/numerator).
 * \param[in]  op2  The second operand (the quotient/denominator).
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Modulo(
    Operand op1,
    Operand op2
)
{
    return AddDivModInstructions( op1, op2, DivMod::MOD );
}

/**
 * \brief  Generates the instructions needed for the division/modulo operations, as the process is shared between them.
 *         Returns an operand containing the final result, determined by the specified return type (div or mod).
 *
 * \param[in]  op1         The first operand (the dividend/numerator).
 * \param[in]  op2         The second operand (the quotient/denominator).
 * \param[in]  returnType  Whether to return the result from the division or modulo operation.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::AddDivModInstructions(
    Operand op1,
    Operand op2,
    DivMod returnType
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        std::string operation = ( DivMod::MOD == returnType ) ? "modulo" : "division";
        LOG_ERROR_AND_THROW( "Operands for " + operation + " must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op2 ) )
    {
        Literal value2{ std::get< Literal >( op2 ) };
        if ( 0u == value2 )
        {
            std::string value1String = std::holds_alternative< Literal >( op1 )
                                       ? std::to_string( std::get< Literal >( op1 ) ) : std::get< std::string >( op1 );
            std::string operation = ( DivMod::MOD == returnType ) ? " % " : " / ";
            LOG_ERROR_AND_THROW( "Division by zero not allowed: " + value1String + operation
                + std::to_string( value2 ), std::invalid_argument );
        }

        if ( std::holds_alternative< Literal >( op1 ) )
        {
            Literal value1{ std::get< Literal >( op1 ) };
            Operand literalResult;
            if ( DivMod::DIV == returnType )
            {
                literalResult = value1 / value2;
            }
            else if ( DivMod::MOD == returnType )
            {
                literalResult = value1 % value2;
            }
            else
            {
                LOG_ERROR_AND_THROW( "Unknown return specifier: can only be DIV or MOD. Value = "
                                     + std::to_string( returnType ), std::invalid_argument );
            }
            return literalResult;
        }
    }
    /**
     * Use the following algorithm:
     *
     * result = 0
     * dividend = op1
     * quotient = op2
     *
     * loop: BRLT end dividend quotient
     * result = result + 1
     * dividend = dividend - quotient
     * jump to loop
     * end:
     *
     * (div = result, mod = dividend)
     */

    // Temp vars declarations

    std::string result = m_instructionFactory->GetNewTempVar( "divResult" );
    constexpr uint8_t resultInit{ 0u };
    m_instructionFactory->AddAssignmentInstruction( result, resultInit );

    // Copy operands into new temp vars because the values are edited.
    std::string dividend = m_instructionFactory->GetNewTempVar( "dividend" );
    m_instructionFactory->AddAssignmentInstruction( dividend, op1 );
    std::string quotient = m_instructionFactory->GetNewTempVar( "quotient" );
    m_instructionFactory->AddAssignmentInstruction( quotient, op2 );


    // Main loop
    std::string mainLoopLabel = m_instructionFactory->GetNewLabel( "divLoop" );
    m_instructionFactory->SetNextInstructionLabel( mainLoopLabel );

    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRLT, dividend, quotient );
    // Retrieve pointer to this instruction to replace the target label at the end.
    ThreeAddrInstruction::Ptr branchToEndInstr = m_instructionFactory->GetLatestInstruction();

    constexpr uint8_t increment{ 1u };
    m_instructionFactory->AddInstruction( result, Opcode::ADD, result, increment );
    m_instructionFactory->AddInstruction( dividend, Opcode::SUB, dividend, quotient );

    // Unconditional branch
    m_instructionFactory->AddInstruction( mainLoopLabel, Opcode::BRE, result, result );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEndInstr, "divModEnd" );

    if ( DivMod::DIV == returnType )
    {
        return result;
    }
    else if ( DivMod::MOD == returnType )
    {
        return dividend;
    }
    LOG_ERROR_AND_THROW( "Unknown return specifier: can only be DIV or MOD. Value = " + std::to_string( returnType ),
                         std::invalid_argument );

    return ""; // This is never reached, but used to satisfy compiler warning.
}

/**
 * \brief  Generates the instructions needed for a bool representing the == operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Equals(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for == must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) == std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isEq = 1
     * BRE end op1 op2
     * isEq = 0
     * end:
     */

    const std::string resultName = "isEq";
    const Literal valueIfBranchTrue{ 1u }; // True if the equals branch succeeds and skips to the end
    return AddComparisonInstructions( resultName,
                                      Opcode::BRE,
                                      op1,
                                      op2,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the != operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::NotEquals(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for != must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) != std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isNeq = 0
     * BRE end op1 op2
     * isNeq = 1
     * end:
     */

    const std::string resultName = "isNeq";
    const Literal valueIfBranchTrue{ 0u }; // False if the equals branch succeeds and skips to the end
    return AddComparisonInstructions( resultName,
                                      Opcode::BRE,
                                      op1,
                                      op2,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the <= operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Leq(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for <= must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) <= std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isLeq = 0
     * BRGT end op1 op2
     * isLeq = 1
     * end:
     */

    const std::string resultName = "isLeq";
    const Literal valueIfBranchTrue{ 0u }; // False if op1 > op2, as this is !(<=)
    // This comes out to equivalent to op2 < op1
    return AddComparisonInstructions( resultName,
                                      Opcode::BRLT,
                                      op2,
                                      op1,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the >= operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::Geq(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for >= must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) >= std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isGeq = 0
     * BRLT end op1 op2
     * isGeq = 1
     * end:
     */

    const std::string resultName = "isGeq";
    const Literal valueIfBranchTrue{ 0u }; // False if op1 < op2, as this is !(>=)
    return AddComparisonInstructions( resultName,
                                      Opcode::BRLT,
                                      op1,
                                      op2,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the < operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::LessThan(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for < must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) < std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isLt = 1
     * BRLT end op1 op2
     * isLt = 0
     * end:
     */

    const std::string resultName = "isLt";
    const Literal valueIfBranchTrue{ 1u }; // True if the less than branch is successful.
    return AddComparisonInstructions( resultName,
                                      Opcode::BRLT,
                                      op1,
                                      op2,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the > operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::GreaterThan(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for > must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< Literal >( op2 ) )
    {
        Operand literalResult = std::get< Literal >( op1 ) > std::get< Literal >( op2 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * isGt = 1
     * BRGT end op1 op2
     * isGt = 0
     * end:
     */

    const std::string resultName = "isGt";
    const Literal valueIfBranchTrue{ 1u }; // True if the greater than branch is successful.
    // True if op1 > op2, aka if op2 < op1
    return AddComparisonInstructions( resultName,
                                      Opcode::BRLT,
                                      op2,
                                      op1,
                                      valueIfBranchTrue );
}

/**
 * \brief  Generates the instructions needed for a bool representing the ! operation.
 *
 * \param[in]  op1  The operand being logically inverted.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::LogicalNot(
    Operand op1
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) )
    {
        LOG_ERROR_AND_THROW( "Operand for ! must contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) )
    {
        Operand literalResult = !std::get< Literal >( op1 );
        return literalResult;
    }

    /**
     * Use the following algorithm:
     *
     * not = 1
     * BRGT end op1 0
     * not = 0
     * end:
     */

    const std::string resultName = "not";
    const Literal valueIfBranchTrue{ 1u }; // True if the operand is >0, therefore !op = 1, as the branch succeeded.
    const Operand zeroOp{ 0u };
    // op > 0 is the same as 0 < op
    return AddComparisonInstructions( resultName,
                                      Opcode::BRLT,
                                      zeroOp,
                                      op1,
                                      valueIfBranchTrue );
}


/**
 * \brief  Generates the instructions needed for a comparison operation. As they all share the same instructions
 *         pattern, this is a shared utility method.
 *
 * \param[in]  resultName         Name with which to create the temp var to store the comparison result.
 * \param[in]  branchType         The opcode describing the desired branching operation.
 * \param[in]  branchOperand1     The first operand in the branch instruction.
 * \param[in]  branchOperand2     The second operand in the branch instruction.
 * \param[in]  valueIfBranchTrue  The value the result will have if branch condition is true.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::AddComparisonInstructions(
    const std::string& resultName,
    Opcode branchType,
    Operand branchOperand1,
    Operand branchOperand2,
    Literal valueIfBranchTrue
)
{
    /**
     * Use the following algorithm:
     *
     * [resultName] = [valueIfBranchTrue]
     * [branchType] end [operands in specified order]
     * [resultName] = ![valueIfBranchTrue]
     * end:
     */

    std::string result = m_instructionFactory->GetNewTempVar( resultName );
    m_instructionFactory->AddAssignmentInstruction( result, valueIfBranchTrue );

    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, branchType, branchOperand1, branchOperand2 );
    ThreeAddrInstruction::Ptr branchToEndInstr = m_instructionFactory->GetLatestInstruction();

    bool branchTrueBool{ static_cast< bool >( valueIfBranchTrue ) };
    const uint8_t skippableValue{ !branchTrueBool };
    m_instructionFactory->AddAssignmentInstruction( result, skippableValue );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEndInstr, "comparisonEnd" );

    return result;
}

/**
 * \brief  Generates the instructions needed for a bool representing the logical OR operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::LogicalOr(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for || must both contain a value.", std::invalid_argument );
    }

    bool isOp1ZeroLiteral{ false };
    if ( std::holds_alternative< Literal >( op1 ) )
    {
        if ( std::get< Literal >( op1 ) > 0 )
        {
            Operand isTrue{ 1u };
            return isTrue;
        }
        isOp1ZeroLiteral = true;
    }

    if ( std::holds_alternative< Literal >( op2 ) )
    {
        if ( std::get< Literal >( op2 ) > 0 )
        {
            Operand isTrue{ 1u };
            return isTrue;
        }
        // If we reach this point and op1 is literal, this means both op1 and op2 are zero
        else if ( isOp1ZeroLiteral )
        {
            Operand isFalse{ 0u };
            return isFalse;
        }
        // If op2 is 0 and op1 is not a literal, this means the truth table resolves to "op1", so we can
        // simply return op1
        else
        {
            return op1;
        }
    }
    // If we reach this point, op2 is not a literal, so we should return op2
    else if ( isOp1ZeroLiteral )
    {
        return op2;
    }

    /**
     * Use the following algorithm:
     *
     * or = 1
     * // branch to the end (i.e. keeping the 'true' result) if either op1 or op2 is true
     * BRLT end 0 op1
     * BRLT end 0 op2
     * or = 0
     * end:
     */

    const Literal valueIfBranchTrue{ 1u }; // True if either greater than branch is successful.
    const Literal valueIfBranchFalse{ 0u };

    const std::string resultName = "isGt";
    std::string result = m_instructionFactory->GetNewTempVar( resultName );
    m_instructionFactory->AddAssignmentInstruction( result, valueIfBranchTrue );

    const Operand zeroOp{ 0u };
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRLT, zeroOp, op1 );
    ThreeAddrInstruction::Ptr branchToEnd1 = m_instructionFactory->GetLatestInstruction();
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRLT, zeroOp, op2 );
    ThreeAddrInstruction::Ptr branchToEnd2 = m_instructionFactory->GetLatestInstruction();

    m_instructionFactory->AddAssignmentInstruction( result, valueIfBranchFalse );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd1, "orEnd" );
    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd2, "orEnd" );

    return result;
}

/**
 * \brief  Generates the instructions needed for a bool representing the logical AND operation.
 *
 * \param[in]  op1  The first operand.
 * \param[in]  op2  The second operand.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacExpressionGenerator::LogicalAnd(
    Operand op1,
    Operand op2
)
{
    if ( ThreeAddrInstruction::IsOperandEmpty( op1 ) || ThreeAddrInstruction::IsOperandEmpty( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for && must both contain a value.", std::invalid_argument );
    }

    bool op1IsTrueLiteral{ false };
    if ( std::holds_alternative< Literal >( op1 ) )
    {
        if ( std::get< Literal >( op1 ) <= 0 )
        {
            Operand falseResult{ 0u };
            return falseResult;
        }
        op1IsTrueLiteral = true;
    }
    if ( std::holds_alternative< Literal >( op2 ) )
    {
        if ( std::get< Literal >( op2 ) <= 0 )
        {
            Operand falseResult{ 0u };
            return falseResult;
        }
        // If both operands hold a true value literal
        else if ( op1IsTrueLiteral )
        {
            Operand trueResult{ 1u };
            return trueResult;
        }
        // If op2 holds a true result but op1 is not a literal, return op1
        else
        {
            return op1;
        }
    }
    // If op1 is true literal but op2 is not a literal, return op2
    else if ( op1IsTrueLiteral )
    {
        return op2;
    }

    /**
     * Use the following algorithm:
     *
     * and = 0
     * // branch to the end (i.e. keeping the 'false' result) if either op1 or op2 is true
     * // therefore, the 'true' result is only reached if both ops are true
     * BRLT end 0 op1
     * BRLT end 0 op2
     * and = 1
     * end:
     */

    const Literal valueIfBranchTrue{ 0u }; // True if either greater than branch is successful.
    const Literal valueIfBranchFalse{ 1u };

    const std::string resultName = "isGt";
    std::string result = m_instructionFactory->GetNewTempVar( resultName );
    m_instructionFactory->AddAssignmentInstruction( result, valueIfBranchTrue );

    const Operand zeroOp{ 0u };
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRLT, zeroOp, op1 );
    ThreeAddrInstruction::Ptr branchToEnd1 = m_instructionFactory->GetLatestInstruction();
    m_instructionFactory->AddInstruction( TacInstructionFactory::PLACEHOLDER, Opcode::BRLT, zeroOp, op2 );
    ThreeAddrInstruction::Ptr branchToEnd2 = m_instructionFactory->GetLatestInstruction();

    m_instructionFactory->AddAssignmentInstruction( result, valueIfBranchFalse );

    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd1, "andEnd" );
    m_instructionFactory->SetInstructionBranchToNextLabel( branchToEnd2, "andEnd" );

    return result;
}