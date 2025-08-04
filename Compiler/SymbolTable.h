/**
 * Declaration of symbol table. This holds information about symbols in the code, for the purposes of error checking
 * and code generation.
 */

#pragma once

#include "SymbolTableEntry.h"

#include <unordered_map>
#include <string>

class SymbolTable
{
public:
    using Ptr = std::shared_ptr< SymbolTable >;
    SymbolTable( SymbolTable::Ptr parentTable )
    : m_table(),
      m_parentTable( parentTable )
    {
    }

    SymbolTableEntry::Ptr GetEntryIfExists( const std::string& identifier );

    void AddEntry( const std::string& identifier, SymbolTableEntry::Ptr entry );

protected:
    // The table itself. Maps string identifiers to structs containing associated information.
    std::unordered_map< std::string, SymbolTableEntry::Ptr > m_table;

    // The table of the parent scope. Null if this is the table associated with the root node of the AST.
    SymbolTable::Ptr m_parentTable;
};