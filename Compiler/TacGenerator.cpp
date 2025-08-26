/**
 * Contains definition of class responsible for generating three-address code for complex operations.
 */

#include "TacGenerator.h"

 /**
  * \brief  Gets identifier representing the next temporary variable available to use. Uses a counter to keep track
  *         of how many are currently in use.
  *
  * \return  String identifier of the next available temporary variable.
  */
std::string
TacGenerator::GetNewTempVar()
{
    // Use a naming convention that isn't allowed by the grammar, to avoid naming clashes. This doesn't matter
    // at this point in compilation as any string is a valid representation.
    std::string id = std::to_string( m_tempVarsInUse ) + "temp";
    ++m_tempVarsInUse;
    return id;
}

/**
 * \brief  Generates the instructions needed to for the multiplication operation. Returns an operand containing the
 *         final result.
 *
 * \param[in]      op1           The first operand to be multiplied.
 * \param[in]      op2           The second operand to be multiplied.
 * \param[in,out]  instructions  Container in which any prerequisite instructions for temporary variables are stored.
 * \param[in,out]  numTempVars   The number of temporary variables currently in use. Used to name the next one.
 *
 * \return  Operand describing the result of the operation (in this case, an identifier).
 */
Operand
TacGenerator::Multiply(
    Operand op1,
    Operand op2,
    Instructions& instructions,
    size_t& numTempVars
)
{
    if ( std::holds_alternative< std::monostate >( op1 ) || std::holds_alternative< std::monostate >( op2 ) )
    {
        LOG_ERROR_AND_THROW( "Operands for multiplication must both contain a value.", std::invalid_argument );
    }

}