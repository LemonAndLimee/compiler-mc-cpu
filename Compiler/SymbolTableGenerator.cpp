/**
 * Definition of symbol table generator class.
 */

#include "SymbolTableGenerator.h"
#include <stdexcept>

/**
 * \brief  Creates symbol table to store information about all symbols within this scope of this AST, and stores it
 *         inside the root tree node. Recursively adds symbol tables for any scope-defining subtrees.
 *
 * \param[in]  treeRootNode  The root node of the AST defining the scope of the symbol table.
 */
void
SymbolTableGenerator::GenerateSymbolTableForAst(
    AstNode::Ptr treeRootNode
)
{
    if ( nullptr == treeRootNode )
    {
        LOG_ERROR_AND_THROW( "Generate symbol table called with nullptr AST node.", std::invalid_argument );
    }

    if ( nullptr != treeRootNode->m_symbolTable )
    {
        LOG_ERROR_AND_THROW( "Cannot generate symbol table: node already has an existing table.", std::runtime_error );
    }

    CreateTableForAstFromParent( nullptr, treeRootNode );
}

/**
 * \brief  Internal method for creating a symbol table for a given AST node, from a given parent table.
 *
 * \param[in]  parentTable   The parent symbol table to this one. Nullptr if is the root table.
 * \param[in]  treeRootNode  The root node of the AST defining the scope of the symbol table.
 */
void
SymbolTableGenerator::CreateTableForAstFromParent(
    SymbolTable::Ptr parentTable,
    AstNode::Ptr treeRootNode
)
{
    SymbolTable::Ptr symbolTable = std::make_shared< SymbolTable >( parentTable );
    treeRootNode->m_symbolTable = symbolTable;

    PopulateTableFromSubTree( symbolTable, treeRootNode );
}

/**
 * \brief  Considers each child node and populates the table with any symbols it finds. If subtrees are found, it
 *         either creates a new symbol table, or recursively calls to continue traversing the nodes in this scope.
 *
 * \param[in]  table       The symbol table of the current scope, to populate with entries.
 * \param[in]  parentNode  Parent of child nodes to check. Pass this instead of children so we can check the operation
 *                         e.g. if it is an assignment.
 */
void
SymbolTableGenerator::PopulateTableFromSubTree(
    SymbolTable::Ptr table,
    AstNode::Ptr parentNode
)
{
    // If node is holding child nodes and not a token, throw error
    if ( parentNode->IsStoringToken() && parentNode->IsStorageInUse() )
    {
        // Expect that this method will only be called on scope nodes, so it should contain a valid storage of
        // child nodes.
        LOG_ERROR_AND_THROW( "Unexpected lack of children for a scope-defining AST node.", std::runtime_error );
    }
    AstNode::Children children = parentNode->GetChildren();

    // If child is an identifier, locate existing entry or create new one
    // If child is a scope node, generate new table for this child
    // Else if child is not a scope-definer but has children of its own, repeat the same process for its children
    for ( size_t i = 0; i < children.size(); i++ )
    {
        auto child = children[i];

        if ( child->IsStorageInUse() )
        {
            // If child holds a token
            if ( child->IsStoringToken() )
            {
                if ( TokenType::IDENTIFIER == child->m_nodeLabel )
                {
                    std::string identifier = child->GetToken()->m_value->m_value.stringValue;
                    SymbolTableEntry::Ptr entry = table->GetEntryIfExists( identifier );
                    // If on left side of assignment, it's a write operation
                    if ( TokenType::ASSIGN == parentNode->m_nodeLabel && 0u == i )
                    {
                        // Because this is an identifier and not a (data type + identifier), we expect there to already
                        // an entry.
                        if ( nullptr == entry )
                        {
                            LOG_ERROR_AND_THROW( "Trying to write to undeclared identifier: '" + identifier + "'",
                                                 std::runtime_error );
                        }

                        entry->isWrittenTo = true;
                    }
                    // Else it's a read operation
                    else
                    {
                        // Read operation expects an entry to exist
                        if ( nullptr == entry )
                        {
                            LOG_ERROR_AND_THROW( "Trying to read from undeclared identifier: '" + identifier + "'",
                                                 std::runtime_error );
                        }

                        entry->isReadFrom = true;
                    }
                }
                // Ignore any other token types
            }
            // Else if child is holding a "variable" rule (it still represents a single identifier)
            else if ( NT::Variable == child->m_nodeLabel )
            {
                AstNode::Children variableChildren = child->GetChildren();
                // Get rightmost child to get identifier node. With two children this should be 2.
                if ( 2u != variableChildren.size() )
                {
                    LOG_ERROR_AND_THROW( "Encountered 'variable' rule node with unexpected number of children: "
                                         + std::to_string( variableChildren.size() ), std::runtime_error );
                }
                AstNode::Ptr idNode = variableChildren[1];
                std::string identifier = idNode->GetToken()->m_value->m_value.stringValue;

                // Expect no existing entry as it is being declared
                SymbolTableEntry::Ptr entry = table->GetEntryIfExists( identifier );
                if ( nullptr != entry )
                {
                    LOG_ERROR_AND_THROW( "Trying to re-declare existing variable: '" + identifier + "'",
                                         std::runtime_error );
                }

                // Create new entry and add to table
                AstNode::Ptr dataTypeNode = variableChildren[0];
                DataType dataType = dataTypeNode->GetToken()->m_value->m_value.dataTypeValue;

                entry = std::make_shared< SymbolTableEntry >();
                entry->dataType = dataType;
                table->AddEntry( identifier, entry );
            }
            // If child represents sub-tree
            else
            {
                // If scope-defining sub-tree, create new table
                if ( child->IsScopeDefiningNode() )
                {
                    CreateTableForAstFromParent( table, child );
                }
                // Else continue populating in this scope
                else
                {
                    PopulateTableFromSubTree( table, child );
                }
            }
        }
        else
        {
            LOG_ERROR_AND_THROW( "Trying to populate symbol table: AST node not storing any value.",
                                 std::runtime_error );
        }
    }
}