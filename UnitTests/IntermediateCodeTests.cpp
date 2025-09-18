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
    const std::string varName{ "var" };
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, 5u, IsDeclaration::TRUE );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assignNode } );

    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if the given AST contains a variable not in the
 * symbol table.
 */
BOOST_AUTO_TEST_CASE( VarNotInSymbolTable )
{
    // Create otherwise-valid program consisting of an assignment statement.
    const std::string varName{ "var" };
    AstNode::Ptr assignNode = CreateAssignNodeFromByteValue( varName, 5u, IsDeclaration::TRUE );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assignNode } );

    const std::string wrongVarName{ "wrongName" };
    CreateAndAttachFakeSymbolTable( blockNode, { wrongVarName } );

    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::runtime_error );
}

BOOST_AUTO_TEST_SUITE_END() // ValidationTests

BOOST_AUTO_TEST_SUITE( TerminalSymbolTests )

BOOST_AUTO_TEST_SUITE( Assign )

/**
 * Tests that the method for converting an AST will throw an error if given an AST with an assignment node with the
 * wrong number of children (i.e. not 2).
 */
BOOST_AUTO_TEST_CASE( WrongNumChildren )
{
    const std::string varName{ "var" };
    AstNode::Ptr varNode
        = std::make_shared< AstNode >( T::IDENTIFIER, std::make_shared< Token >( T::IDENTIFIER, varName ) );

    AstNode::Children oneChild{ varNode };
    AstNode::Ptr invalidAssignNode_OneChild = std::make_shared< AstNode >( T::ASSIGN, oneChild );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { invalidAssignNode_OneChild } );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );

    AstNode::Children tooManyChildren{ varNode, invalidAssignNode_OneChild, blockNode };
    AstNode::Ptr invalidAssignNode_ThreeChildren = std::make_shared< AstNode >( T::ASSIGN, tooManyChildren );
    AstNode::Ptr blockNode_TooManyChildren = WrapNodesInBlocks( { invalidAssignNode_ThreeChildren } );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode_TooManyChildren ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to a
 * single RHS literal value.
 */
BOOST_AUTO_TEST_CASE( AssignsSingleLiteral )
{
    const std::string varName{ "var" };
    constexpr uint8_t literalValue{ 3u };
    AstNode::Ptr assign = CreateAssignNodeFromByteValue( varName, literalValue, IsDeclaration::TRUE );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { varName } );

    // Expect a single assignment TAC instruction to be added

    TAC::Operand rhsOperand;
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .with( mock::any, mock::retrieve( rhsOperand ) );

    m_codeGenerator->GenerateIntermediateCode( blockNode );

    BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( rhsOperand ) );
    TAC::Literal rhsValue = std::get< TAC::Literal >( rhsOperand );
    BOOST_CHECK_EQUAL( literalValue, rhsValue );
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to a
 * single RHS identifier value.
 */
BOOST_AUTO_TEST_CASE( AssignsSingleIdentifier )
{
    const std::string varName{ "var" };
    const std::string rhsIdentifier{ "value" };
    AstNode::Ptr assign = CreateAssignNodeFromVar( varName, rhsIdentifier, IsDeclaration::TRUE );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { varName, rhsIdentifier } );

    // Expect a single assignment TAC instruction to be added

    TAC::Operand rhsOperand;
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .with( mock::any, mock::retrieve( rhsOperand ) );

    m_codeGenerator->GenerateIntermediateCode( blockNode );

    BOOST_REQUIRE( std::holds_alternative< std::string >( rhsOperand ) );
    // Can't check the operand contents because a new unique id is calculated.
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to an
 * expression, consisting of an operation and 2 operands that have a 1-to-1 relationship with a TAC instruction.
 */
BOOST_AUTO_TEST_CASE( AssignsExpression_MapsToTacOpcode )
{
    const std::string varName{ "var" };
    constexpr GrammarSymbols::T expressionLabel{ T::PLUS };
    const std::string rhsOperand1{ "operand1" };
    constexpr uint8_t rhsOperand2{ 10u };
    AstNode::Ptr assign = CreateTwoOperandStatement< std::string, uint8_t >( varName,
                                                                             IsDeclaration::TRUE,
                                                                             expressionLabel,
                                                                             rhsOperand1,
                                                                             rhsOperand2 );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { varName, rhsOperand1 } );

    // Expect a single TAC instruction to be added
    constexpr TAC::Opcode expectedTacOpcode{ ADD };
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .with( mock::any, expectedTacOpcode, mock::any, mock::any )
        .calls(
            [&]( std::string, TAC::Opcode, TAC::Operand operand1, TAC::Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                // Can't check the operand contents because a new unique id is calculated.

                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< TAC::Literal >( operand2 ) );
            }
        );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to an
 * expression, resolving the expression to a single literal if it is made up of 2 literals, and maps 1-to-1 with a
 * TAC operation.
 */
BOOST_AUTO_TEST_CASE( AssignsExpression_MapsToTacOpcode_TwoLiterals )
{
    const std::string varName{ "var" };
    constexpr GrammarSymbols::T expressionLabel{ T::PLUS };
    constexpr uint8_t rhsOperand1{ 2u };
    constexpr uint8_t rhsOperand2{ 10u };
    AstNode::Ptr assign = CreateTwoOperandStatement< uint8_t, uint8_t >( varName,
                                                                         IsDeclaration::TRUE,
                                                                         expressionLabel,
                                                                         rhsOperand1,
                                                                         rhsOperand2 );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { varName } );

    // Expect a single assignment TAC instruction to be added
    constexpr uint8_t expectedRhs{ rhsOperand1 + rhsOperand2 };
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .calls(
            [&]( std::string, TAC::Operand rhs )
            {
                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( rhs ) );
                BOOST_CHECK_EQUAL( expectedRhs, std::get< TAC::Literal >( rhs ) );
            }
        );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to an
 * expression, consisting of an operation and 2 operands that do not correspond to a TAC instruction. Expect a call to
 * the TAC expression generator to resolve the operation.
 */
BOOST_AUTO_TEST_CASE( AssignsExpression_DoesNotMapToTacOpcode )
{
    const std::string varName{ "var" };
    constexpr GrammarSymbols::T expressionLabel{ T::MOD }; // Not an accepted TAC opcode
    const std::string rhsOperand1{ "operand1" };
    constexpr uint8_t rhsOperand2{ 10u };
    AstNode::Ptr assign = CreateTwoOperandStatement< std::string, uint8_t >( varName,
                                                                             IsDeclaration::TRUE,
                                                                             expressionLabel,
                                                                             rhsOperand1,
                                                                             rhsOperand2 );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { varName, rhsOperand1 } );

    mock::sequence s;

    // Expect a call to the TAC generator to get the rhs operand
    const Operand expressionOperandToReturn{ "expressionResult" };
    MOCK_EXPECT( m_exprGeneratorMock->Modulo )
        .once()
        .in( s )
        .calls(
            [&]( Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                // Can't check the operand contents because a new unique id is calculated.

                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< TAC::Literal >( operand2 ) );

                return expressionOperandToReturn;
            }
        );

    // Expect a single assignment TAC instruction to be added
    constexpr TAC::Opcode expectedTacOpcode{ ADD };
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s )
        .calls(
            [&]( std::string, TAC::Operand operand )
            {
                BOOST_CHECK( operand == expressionOperandToReturn );
            }
        );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

/**
 * Tests that the method for converting an AST will successfully generate TAC instructions for an assignment to a
 * set of nested expressions.
 */
BOOST_AUTO_TEST_CASE( AssignsNestedExpression)
{
    // Create the following expression: ((a - 1) > (6 / 2)) & 5
    const std::string varA{ "a" };
    constexpr uint8_t decrement{ 1u };
    constexpr T subExpr1NodeLabel{ MINUS };
    AstNode::Ptr subExpr1 = CreateTwoOpExpression( subExpr1NodeLabel, varA, decrement );

    constexpr uint8_t dividend{ 6u };
    constexpr uint8_t quotient{ 2u };
    constexpr T subExpr2NodeLabel{ DIVIDE };
    AstNode::Ptr subExpr2 = CreateTwoOpExpression( subExpr2NodeLabel, dividend, quotient );

    const T jointSubExprNodeLabel{ GT };
    AstNode::Ptr jointSubExpr = CreateTwoOpExpression( jointSubExprNodeLabel, subExpr1, subExpr2 );

    const std::string targetVar{ "var" };
    constexpr uint8_t rhsOperand2{ 5u };
    constexpr T exprNodeLabel{ BITWISE_AND };
    AstNode::Ptr assign = CreateTwoOperandStatement< AstNode::Ptr, uint8_t >( targetVar,
                                                                              IsDeclaration::TRUE,
                                                                              exprNodeLabel,
                                                                              jointSubExpr,
                                                                              rhsOperand2 );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { assign } );
    CreateAndAttachFakeSymbolTable( blockNode, { targetVar, varA } );

    mock::sequence s;

    // Expect an instruction to a temp var to store the first minus sub-expression.
    const std::string minusTempVar{ "minus" };
    MOCK_EXPECT( m_instrFactoryMock->GetNewTempVar )
        .once()
        .in( s )
        .returns( minusTempVar );
    constexpr Opcode minusOpcode{ Opcode::SUB };
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( minusTempVar, minusOpcode, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                // Can't check the operand contents because a new unique id is calculated.
                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( decrement, std::get< TAC::Literal >( operand2 ) );
            }
        );

    // Expect a call to resolve the divide sub-expression - return a single literal as both operands are literal.
    constexpr uint8_t divideValueToReturn{ dividend / quotient };
    const Operand divideOperandToReturn{ divideValueToReturn };
    MOCK_EXPECT( m_exprGeneratorMock->Divide )
        .once()
        .in( s )
        .calls(
            [&]( Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand1 ) );
                BOOST_CHECK_EQUAL( dividend, std::get< TAC::Literal >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( quotient, std::get< TAC::Literal >( operand2 ) );

                return divideOperandToReturn;
            }
        );

    // Expect an instruction to a temp var to store the greater than sub-expression.
    const Operand gtOperandToReturn{ "gtResult" };
    MOCK_EXPECT( m_exprGeneratorMock->GreaterThan )
        .once()
        .in( s )
        .calls(
            [&]( Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                BOOST_CHECK_EQUAL( minusTempVar, std::get< std::string >( operand1 ) );
                BOOST_CHECK( divideOperandToReturn == operand2 );

                return gtOperandToReturn;
            }
        );

    // Expect an instruction to the target var, with the bitwise and expression.
    const Opcode bitwiseAndOpcode{ Opcode::AND };
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( mock::any, bitwiseAndOpcode, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_CHECK( gtOperandToReturn == operand1 );
                BOOST_REQUIRE( std::holds_alternative< TAC::Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< TAC::Literal >( operand2 ) );
            }
        );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
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