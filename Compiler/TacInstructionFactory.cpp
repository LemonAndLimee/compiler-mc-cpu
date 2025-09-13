/**
 * Contains definition of factory class for Three-Address-Code instructions.
 */

#include "TacInstructionFactory.h"

TacInstructionFactory::TacInstructionFactory()
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
TacInstructionFactory::GetNewTempVar(
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
TacInstructionFactory::GetNewLabel(
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
TacInstructionFactory::SetNextLabel(
    const std::string& label
)
{
    m_nextLabel = label;
}