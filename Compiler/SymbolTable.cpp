/**
 * Definition of symbol table methods. This holds information about symbols in the code, for the purposes of error
 * checking and code generation.
 */

#include <stdexcept>

#include "SymbolTable.h"
#include "Logger.h"

/**
 * \brief  Searches for and returns entry corresponding to the identifier, either in this table or a parent table.
 *
 * \param[in]  identifier  The string identifier of the symbol which the entry corresponds to.
 *
 * \return  Pointer to the associated symbol table entry, or nullptr if one couldn't be found.
 */
SymbolTableEntry::Ptr
SymbolTable::GetEntryIfExists(
    const std::string& identifier
)
{
    if ( 0u < m_table.count( identifier ) )
    {
        return m_table.find( identifier )->second;
    }
    // If no entry in this table, try parent table if we have one.
    else if ( nullptr != m_parentTable )
    {
        return m_parentTable->GetEntryIfExists( identifier );
    }
    else
    {
        return nullptr;
    }
}

/**
 * \brief  Adds entry to symbol table.
 *
 * \param[in]  identifier  The string identifier of the symbol which the entry corresponds to.
 * \param[in]  entry       Ptr to the created entry.
 */
void
SymbolTable::AddEntry(
    const std::string& identifier,
    SymbolTableEntry::Ptr entry
)
{
    if ( nullptr == entry )
    {
        std::string errMsg = "Could not add symbol table entry for '" + identifier
                             + "': given a nullptr entry struct.";
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }

    // If table already contains this entry, throw error
    if ( 0u < m_table.count( identifier ) )
    {
        std::string errMsg = "Could not add symbol table entry for '" + identifier
                             + "': entry already exists";
        LOG_ERROR( errMsg );
        throw std::runtime_error( errMsg );
    }

    m_table.insert( { identifier, entry } );
}