#include <boost/test/unit_test.hpp>

#include "IntermediateCode.h"

#include "TacExpressionGeneratorMock.h"
#include "TacInstructionFactoryMock.h"
#include "AstSimulator.h"

using namespace AstSimulator;

class IntermediateCodeTestsFixture
{
public:
    IntermediateCodeTestsFixture()
    : m_instrFactoryMock( std::make_shared< TacInstructionFactoryMock >() ),
      m_exprGeneratorMock( std::make_shared< TacExpressionGeneratorMock >() )
    {
        m_codeGenerator = std::make_shared< IntermediateCode >( m_instrFactoryMock, m_exprGeneratorMock );
    }

    IntermediateCode::Ptr m_codeGenerator;

    TacExpressionGeneratorMock::Ptr m_exprGeneratorMock;
    TacInstructionFactoryMock::Ptr m_instrFactoryMock;
};

BOOST_FIXTURE_TEST_SUITE( IntermediateCodeTests, IntermediateCodeTestsFixture )

BOOST_AUTO_TEST_SUITE( ValidationTests )

/**
 * Tests that the method for converting an AST will throw an error if the given AST is nullptr.
 */
BOOST_AUTO_TEST_CASE( NullptrAst )
{
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( nullptr ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if the given AST is storing a token instead of
 * child nodes.
 */
BOOST_AUTO_TEST_CASE( AstStoresToken )
{
    Token::Ptr token = std::make_shared< Token >( T::MINUS );
    AstNode::Ptr tokenNode = std::make_shared< AstNode >( T::MINUS, token );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( tokenNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if the given AST has no child nodes stored.
 */
BOOST_AUTO_TEST_CASE( AstNoChildren )
{
    AstNode::Children children{}; // Empty children
    AstNode::Ptr ast = std::make_shared< AstNode >( T::AND, children );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( ast ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if the given AST has no symbol table.
 */
BOOST_AUTO_TEST_CASE( AstNoSymbolTable )
{
    // Create otherwise-valid program consisting of an assignment statement.
    std::string varName{ "var" };
    AstNode::Ptr assignNode = AstSimulator::CreateAssignNodeFromByteValue( varName, 5u, IsDeclaration::TRUE );
    AstNode::Ptr blockNode = AstSimulator::WrapNodesInBlocks( { assignNode } );

    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

BOOST_AUTO_TEST_SUITE_END() // ValidationTests

BOOST_AUTO_TEST_SUITE( TerminalSymbolTests )

BOOST_AUTO_TEST_SUITE( Assign )

// - Assign:
//   - Wrong num children
//   - Assign single literal
//   - Assign single var
//   - Assign expression
//   - Assign nested expression

/**
 * Tests that the method for converting an AST will throw an error if given an AST with an assignment node with the
 * wrong number of children (i.e. not 2).
 */
BOOST_AUTO_TEST_CASE( WrongNumChildren )
{
    std::string varName{ "var" };
    AstNode::Ptr varNode
        = std::make_shared< AstNode >( T::IDENTIFIER, std::make_shared< Token >( T::IDENTIFIER, varName ) );

    AstNode::Children oneChild{ varNode };
    AstNode::Ptr invalidAssignNode_OneChild = std::make_shared< AstNode >( T::ASSIGN, oneChild );
    AstNode::Ptr blockNode = AstSimulator::WrapNodesInBlocks( { invalidAssignNode_OneChild } );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );

    AstNode::Children tooManyChildren{ varNode, invalidAssignNode_OneChild, blockNode };
    AstNode::Ptr invalidAssignNode_ThreeChildren = std::make_shared< AstNode >( T::ASSIGN, tooManyChildren );
    AstNode::Ptr blockNode_TooManyChildren = AstSimulator::WrapNodesInBlocks( { invalidAssignNode_ThreeChildren } );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode_TooManyChildren ), std::invalid_argument );
}

BOOST_AUTO_TEST_SUITE_END() // Assign

BOOST_AUTO_TEST_SUITE( IfElse )

// - If/else:
//   - Wrong num children
//   - Has no symbol table
//   - If single operand condition
//   - If expression condition
//   - 3rd child not else label
//   - If with valid else block

BOOST_AUTO_TEST_SUITE_END() // IfElse

BOOST_AUTO_TEST_SUITE( For )

// - For loop:
//   - Wrong num children
//   - Init has wrong num children
//   - No symbol table
//   - Simple case (single op condition)
//   - Complex case (expression assignment, comparison condition)

BOOST_AUTO_TEST_SUITE_END() // For

BOOST_AUTO_TEST_SUITE( While )

// - While loop:
//   - Wrong num children
//   - No symbol table
//   - Single op condition
//   - Expression condition

BOOST_AUTO_TEST_SUITE_END() // While

BOOST_AUTO_TEST_SUITE_END() // TerminalSymbolTests

BOOST_AUTO_TEST_SUITE( NonTerminalSymbolTests )

// Non-Terminal:
// - Invalid node symbol (not Block)
// - Multiple terminals
// - Multiple nested terminals

BOOST_AUTO_TEST_SUITE_END() // NonTerminalSymbolTests

BOOST_AUTO_TEST_SUITE_END() // IntermediateCodeTests