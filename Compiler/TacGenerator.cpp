/**
 * Contains definition of class responsible for generating three-address code for complex operations.
 */

#include "TacGenerator.h"

TacGenerator::TacGenerator()
    : m_tempVarsInUse( 0u ),
    m_labelsInUse( 0u ),
    m_nextLabel( "" )
{
}

/**
 * \brief  Gets identifier representing the next temporary variable available to use. Uses a counter to keep track
 *         of how many are currently in use.
 *
 * \param[in]  hrfName  Optional identifier name to use in combination with the counter, to allow easier debugging.
 *
 * \return  String identifier of the next available temporary variable.
 */
std::string
TacGenerator::GetNewTempVar(
    std::string hrfName //= "temp"
)
{
    // Use a naming convention that isn't allowed by the grammar, to avoid naming clashes. This doesn't matter
    // at this point in compilation as any string is a valid representation.
    std::string id = std::to_string( m_tempVarsInUse ) + hrfName;
    ++m_tempVarsInUse;
    return id;
}

/**
 * \brief  Gets a new unique branch label. Uses a counter to keep track of the next available number. If the next label
 *         has already been pre-set, this is returned instead.
 *
 * \param[in]  hrfName  Optional label name to use in combination with the counter, to allow easier debugging.
 *
 * \return  Unique label in string form.
 */
std::string
TacGenerator::GetNewLabel(
    std::string hrfName //= "label"
)
{
    if ( "" != m_nextLabel )
    {
        std::string label = m_nextLabel;
        m_nextLabel = "";
        return label;
    }
    else
    {
        std::string label = hrfName + std::to_string( m_labelsInUse );
        ++m_labelsInUse;
        return label;
    }
}

/**
 * \brief  Configures the next label to be returned when a new one is requested.
 *
 * \param[in]  label  The string to be returned upon next label request.
 */
void
TacGenerator::SetNextLabel(
    const std::string& label
)
{
    m_nextLabel = label;
}

/**
 * \brief  Generates the instructions needed to for the multiplication operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]      op1           The first operand to be multiplied.
 * \param[in]      op2           The second operand to be multiplied.
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacGenerator::Multiply(
    Operand op1,
    Operand op2,
    Instructions& instructions
)
{
    if ( std::holds_alternative< std::monostate >( op1 ) || std::holds_alternative< std::monostate >( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for multiplication must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< Literal >( op1 ) && std::holds_alternative< uint8_t >( op2 ) )
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
     * BRZ shift lsb
     * result = result + multiplicand
     * shift: multiplicand = << multiplicand
     * multiplier = >> multiplier
     * bitCounter = bitCounter - 1
     * BRGT loop bitCounter 0
     *
     * (return result)
     */

    constexpr size_t numInstructionsToAdd{ 11u };
    Instructions tempIns{}; // Working copy of instructions - copied into the real one after successful pass.
    tempIns.reserve( numInstructionsToAdd );

    Operand emptyOp; // Empty operand to use when an operand is not in use.

    // Temp vars declarations

    std::string result = GetNewTempVar( "multResult" );
    constexpr uint8_t resultInit{ 0u };
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( result, Opcode::UNUSED, resultInit, emptyOp ) );

    // Copy operands into new temp vars because the values are edited.
    std::string multiplier = GetNewTempVar( "multiplier" );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( multiplier, Opcode::UNUSED, op1, emptyOp ) );
    std::string multiplicand = GetNewTempVar( "multiplicand" );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( multiplicand, Opcode::UNUSED, op2, emptyOp ) );

    std::string bitCounter = GetNewTempVar( "bitCounter" );
    constexpr uint8_t bitCtInit{ 8u }; // 8 bits in a byte, which is our current supported literal length.
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( bitCounter, Opcode::UNUSED, bitCtInit, emptyOp ) );


    // Main loop
    std::string mainLoopLabel = GetNewLabel( "multLoop" );
    std::string lsb = GetNewTempVar( "lsb" ); // Use bitmask to retrieve the LSB, in bit form.
    constexpr uint8_t lsbBitmask{ 0xfe };
    tempIns.push_back(
        std::make_shared< ThreeAddrInstruction >( lsb, Opcode::AND, multiplier, lsbBitmask, mainLoopLabel )
    );

    std::string shiftLabel = GetNewLabel( "shift" );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( shiftLabel, Opcode::BRZ, lsb, emptyOp ) );

    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( result, Opcode::ADD, result, multiplicand ) );

    // The location of the BRZ jump - shift the multiplier and multiplicand to move onto the next LSB
    tempIns.push_back(
        std::make_shared< ThreeAddrInstruction >( multiplicand, Opcode::LS, multiplicand, emptyOp, shiftLabel )
    );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( multiplier, Opcode::RS, multiplier, emptyOp ) );

    constexpr uint8_t decrement{ 1u };
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( bitCounter, Opcode::SUB, bitCounter, decrement ) );

    constexpr uint8_t brgtValue{ 0u };
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( mainLoopLabel, Opcode::BRGT, bitCounter, brgtValue ) );


    instructions.insert( instructions.end(), tempIns.begin(), tempIns.end() );
    return result;
}

/**
 * \brief  Generates the instructions needed for the division operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]      op1           The first operand (the dividend/numerator).
 * \param[in]      op2           The second operand (the quotient/denominator).
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacGenerator::Divide(
    Operand op1,
    Operand op2,
    Instructions& instructions
)
{
    return AddDivModInstructions( op1, op2, instructions, DivMod::DIV );
}

/**
 * \brief  Generates the instructions needed for the modulo operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]      op1           The first operand (the dividend/numerator).
 * \param[in]      op2           The second operand (the quotient/denominator).
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacGenerator::Modulo(
    Operand op1,
    Operand op2,
    Instructions& instructions
)
{
    return AddDivModInstructions( op1, op2, instructions, DivMod::MOD );
}

/**
 * \brief  Generates the instructions needed for the division/modulo operations, as the process is shared between them.
 *         Returns an operand containing the final result, determined by the specified return type (div or mod).
 *
 * \param[in]      op1           The first operand (the dividend/numerator).
 * \param[in]      op2           The second operand (the quotient/denominator).
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 * \param[in]      returnType    Whether to return the result from the division or modulo operation.
 *
 * \return  Operand describing the result of the operation.
 */
Operand
TacGenerator::AddDivModInstructions(
    Operand op1,
    Operand op2,
    Instructions& instructions,
    DivMod returnType )
{
    if ( std::holds_alternative< std::monostate >( op1 ) || std::holds_alternative< std::monostate >( op2 ) )
    {
        std::string operation = ( DivMod::MOD == returnType ) ? "modulo" : "division";
        LOG_ERROR_AND_THROW( "Operands for " + operation + " must both contain a value.", std::invalid_argument );
    }

    if ( std::holds_alternative< uint8_t >( op2 ) )
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
     * loop: BRGT end quotient dividend
     * result = result + 1
     * dividend = dividend - quotient
     * BRU loop
     * end:
     *
     * (div = result, mod = dividend)
     */

    constexpr size_t numInstructionsToAdd{ 7u };
    Instructions tempIns{}; // Working copy of instructions - copied into the real one after successful pass.
    tempIns.reserve( numInstructionsToAdd );

    Operand emptyOp; // Empty operand to use when an operand is not in use.

    // Temp vars declarations

    std::string result = GetNewTempVar( "divResult" );
    constexpr uint8_t resultInit{ 0u };
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( result, Opcode::UNUSED, resultInit, emptyOp ) );

    // Copy operands into new temp vars because the values are edited.
    std::string dividend = GetNewTempVar( "dividend" );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( dividend, Opcode::UNUSED, op1, emptyOp ) );
    std::string quotient = GetNewTempVar( "quotient" );
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( quotient, Opcode::UNUSED, op2, emptyOp ) );


    // Main loop

    std::string mainLoopLabel = GetNewLabel( "divLoop" );
    std::string endLabel = GetNewLabel( "end" );
    tempIns.push_back(
        std::make_shared< ThreeAddrInstruction >( endLabel, Opcode::BRGT, quotient, dividend, mainLoopLabel )
    );

    constexpr uint8_t increment{ 1u };
    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( result, Opcode::ADD, result, increment ) );

    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( dividend, Opcode::SUB, dividend, quotient ) );

    tempIns.push_back( std::make_shared< ThreeAddrInstruction >( mainLoopLabel, Opcode::BRU, emptyOp, emptyOp ) );


    SetNextLabel( endLabel );

    instructions.insert( instructions.end(), tempIns.begin(), tempIns.end() );

    if ( DivMod::DIV == returnType )
    {
        return result;
    }
    else if ( DivMod::MOD == returnType )
    {
        return dividend;
    }
    else
    {
        LOG_ERROR_AND_THROW( "Unknown return specifier: can only be DIV or MOD. Value = " + std::to_string( returnType ),
                             std::invalid_argument );
    }
}