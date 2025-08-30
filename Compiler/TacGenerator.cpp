/**
 * Contains definition of class responsible for generating three-address code for complex operations.
 */

#include "TacGenerator.h"

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
 * \brief  Gets a new unique branch label. Uses a counter to keep track of the next available number.
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
    std::string label = hrfName + std::to_string( m_labelsInUse );
    ++m_labelsInUse;
    return label;
}

/**
 * \brief  Generates the instructions needed to for the multiplication operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]      op1           The first operand to be multiplied.
 * \param[in]      op2           The second operand to be multiplied.
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 *
 * \return  Operand describing the result of the operation (in this case, an identifier).
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

    return std::monostate{}; // stub return for compilation
}