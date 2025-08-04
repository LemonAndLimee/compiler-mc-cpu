/**
 * Declaration of symbol table entry struct.
 */

#pragma once
#include "TokenValue.h"

/**
 * \brief  Holds information about a symbol (e.g. identifier) in source code.
 */
struct SymbolTableEntry
{
    using Ptr = std::shared_ptr< SymbolTableEntry >;

    DataType dataType;
    bool isReadFrom;
    bool isWrittenTo;
    uint8_t memoryAddress;
};