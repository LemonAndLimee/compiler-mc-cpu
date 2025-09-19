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

    ThreeAddrInstruction::Ptr MakeDummyUniqueTacInstr()
    {
        return std::make_shared< ThreeAddrInstruction >( "target", Opcode::INVALID, "op1", "op2" );
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
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );

    AstNode::Children tooManyChildren{ varNode, invalidAssignNode_OneChild, blockNode };
    AstNode::Ptr invalidAssignNode_ThreeChildren = std::make_shared< AstNode >( T::ASSIGN, tooManyChildren );
    AstNode::Ptr blockNode_TooManyChildren = WrapNodesInBlocks( { invalidAssignNode_ThreeChildren } );
    CreateAndAttachFakeSymbolTable( blockNode_TooManyChildren, {} );
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

    BOOST_REQUIRE( std::holds_alternative< Literal >( rhsOperand ) );
    Literal rhsValue = std::get< Literal >( rhsOperand );
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

                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< Literal >( operand2 ) );
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
                BOOST_REQUIRE( std::holds_alternative< Literal >( rhs ) );
                BOOST_CHECK_EQUAL( expectedRhs, std::get< Literal >( rhs ) );
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

                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< Literal >( operand2 ) );

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
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( decrement, std::get< Literal >( operand2 ) );
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
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand1 ) );
                BOOST_CHECK_EQUAL( dividend, std::get< Literal >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( quotient, std::get< Literal >( operand2 ) );

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
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( rhsOperand2, std::get< Literal >( operand2 ) );
            }
        );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

BOOST_AUTO_TEST_SUITE_END() // Assign

BOOST_AUTO_TEST_SUITE( IfElse )

/**
 * Tests that the method for converting an AST will throw an error if given an AST with an if node with the
 * wrong number of children.
 */
BOOST_AUTO_TEST_CASE( WrongNumChildren )
{
    constexpr uint8_t byteValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE, std::make_shared< Token >( T::BYTE, byteValue ) );

    AstNode::Children oneChild{ conditionNode };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, oneChild );
    CreateAndAttachFakeSymbolTable( ifNode, {} );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if given an AST with an if node with no symbol table.
 */
BOOST_AUTO_TEST_CASE( NoSymbolTable )
{
    constexpr uint8_t byteValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE, std::make_shared< Token >( T::BYTE, byteValue ) );

    const std::string dummyVarName{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVarName, 5u, IsDeclaration::TRUE );

    AstNode::Children ifChildren{ conditionNode, dummyAssign };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, ifChildren );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    // Give the outer block node a symbol table, but not the if node.
    CreateAndAttachFakeSymbolTable( blockNode, {} );

    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will correctly add the expected instructions if given an if
 * node with a single operand condition, and no else section.
 */
BOOST_AUTO_TEST_CASE( SingleOperandCondition )
{
    constexpr uint8_t conditionValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE,
                                                              std::make_shared< Token >( T::BYTE, conditionValue ) );

    const std::string dummyVarName{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVarName, 5u, IsDeclaration::TRUE );

    AstNode::Children ifChildren{ conditionNode, dummyAssign };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, ifChildren );
    CreateAndAttachFakeSymbolTable( ifNode, { dummyVarName } );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );

    mock::sequence s;

    // Expect a branch instruction to the end, if condition == 0.
    // The label is not yet known at this time so expect a placeholder.
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand1 ) );
                BOOST_CHECK_EQUAL( conditionValue, std::get< Literal >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( 0u, std::get< Literal >( operand2 ) );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToElseInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToElseInstr );

    // Expect the assign statement to be added - this is already tested so we're not interested in 100% validation.
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s );

    // Expect a call to set the branch target to the current end point
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToElseInstr, mock::any );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

/**
 * Tests that the method for converting an AST will correctly add the expected instructions if given an if
 * node with an expression condition, and no else section.
 */
BOOST_AUTO_TEST_CASE( ExpressionCondition )
{
    // Make expression: a <= b
    const std::string varA{ "a" };
    const std::string varB{ "b" };
    const T conditionNodeLabel{ T::LEQ };
    AstNode::Ptr conditionNode = CreateTwoOpExpression( conditionNodeLabel, varA, varB );

    const std::string dummyVarName{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVarName, 5u, IsDeclaration::TRUE );

    AstNode::Children ifChildren{ conditionNode, dummyAssign };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, ifChildren );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    CreateAndAttachFakeSymbolTable( blockNode, { varA, varB } );

    CreateAndAttachFakeSymbolTable( ifNode, { dummyVarName }, blockNode->m_symbolTable );

    mock::sequence s;

    // Expect call to get condition operand
    const Operand conditionOperandToReturn{ "condition" };
    MOCK_EXPECT( m_exprGeneratorMock->Leq )
        .once()
        .in( s )
        .calls(
            [&]( Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand2 ) );
                return conditionOperandToReturn;
            }
        );

    // Expect a branch instruction to the end, if condition == 0.
    // The label is not yet known at this time so expect a placeholder.
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_CHECK( conditionOperandToReturn == operand1 );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( 0u, std::get< Literal >( operand2 ) );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToElseInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToElseInstr );

    // Expect the assign statement to be added - this is already tested so we're not interested in 100% validation.
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s );

    // Expect a call to set the branch target to the current end point
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToElseInstr, mock::any );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

/**
 * Tests that the method for converting an AST will throw an error if given an if node with three children, and the
 * third child is not an else node.
 */
BOOST_AUTO_TEST_CASE( ThirdChildNotElse )
{
    constexpr uint8_t conditionValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE,
                                                              std::make_shared< Token >( T::BYTE, conditionValue ) );

    const std::string dummyVarName{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVarName, 5u, IsDeclaration::TRUE );

    AstNode::Ptr thirdChild = CreateAssignNodeFromByteValue( dummyVarName, 5u, IsDeclaration::TRUE );

    AstNode::Children ifChildren{ conditionNode, dummyAssign, thirdChild };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, ifChildren );
    CreateAndAttachFakeSymbolTable( ifNode, { dummyVarName } );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );

    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will successfully convert an if sub-tree with a valid else block.
 */
BOOST_AUTO_TEST_CASE( ValidElse )
{
    constexpr uint8_t conditionValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE,
                                                              std::make_shared< Token >( T::BYTE, conditionValue ) );

    const std::string dummyIfVarName{ "dummyIfVar" };
    AstNode::Ptr dummyIfAssign = CreateAssignNodeFromByteValue( dummyIfVarName, 5u, IsDeclaration::TRUE );

    const std::string dummyElseVarName{ "dummyElseVar" };
    AstNode::Ptr dummyElseAssign = CreateAssignNodeFromByteValue( dummyElseVarName, 5u, IsDeclaration::TRUE );
    AstNode::Children elseChildren{ dummyElseAssign };
    AstNode::Ptr elseNode = std::make_shared< AstNode >( T::ELSE, elseChildren );

    AstNode::Children ifChildren{ conditionNode, dummyIfAssign, elseNode };
    AstNode::Ptr ifNode = std::make_shared< AstNode >( T::IF, ifChildren );
    CreateAndAttachFakeSymbolTable( ifNode, { dummyIfVarName, dummyElseVarName } );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { ifNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );

    mock::sequence s;

    // Expect a branch instruction to the end, if condition == 0.
    // The label is not yet known at this time so expect a placeholder.
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand1 ) );
                BOOST_CHECK_EQUAL( conditionValue, std::get< Literal >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( 0u, std::get< Literal >( operand2 ) );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToElseInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToElseInstr );

    // Expect the assign statement to be added - this is already tested so we're not interested in 100% validation.
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s );

    // Expect an unconditional branch to the end, skipping past the else block
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                // The contents of the operands don't matter, only that they are equal.
                BOOST_CHECK( operand1 == operand2 );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToEndInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToEndInstr );

    // The ELSE section:

    // Expect a call to set the branch target to the current else point
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToElseInstr, mock::any );

    // Expect the else assign statement to be added - this is already tested so we're not interested in 100% validation.
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s );

    // Expect a call to set the branch target to the current end point
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToEndInstr, mock::any );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

BOOST_AUTO_TEST_SUITE_END() // IfElse

BOOST_AUTO_TEST_SUITE( For )

/**
 * Tests that the method for converting an AST will throw an error if given an AST with a for node with the
 * wrong number of children.
 */
BOOST_AUTO_TEST_CASE( WrongNumChildren )
{
    constexpr uint8_t byteValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE, std::make_shared< Token >( T::BYTE, byteValue ) );

    AstNode::Children oneChild{ conditionNode };
    AstNode::Ptr forNode = std::make_shared< AstNode >( T::FOR, oneChild );
    CreateAndAttachFakeSymbolTable( forNode, {} );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { forNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if given an AST with a for node with an initialiser
 * node with the wrong number of children.
 */
BOOST_AUTO_TEST_CASE( ForInit_WrongNumChildren )
{
    const std::string initVar{ "initVar" };
    AstNode::Ptr initAssign = CreateAssignNodeFromByteValue( initVar, 0u, IsDeclaration::TRUE );
    AstNode::Children initChildren{ initAssign }; // Missing the other 2 parts of the init
    AstNode::Ptr initNode = std::make_shared< AstNode >( NT::For_init, initChildren );

    const std::string dummyVar{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVar, 5u, IsDeclaration::TRUE );

    AstNode::Children forChildren{ initNode, dummyAssign };
    AstNode::Ptr forNode = std::make_shared< AstNode >( T::FOR, forChildren );
    CreateAndAttachFakeSymbolTable( forNode, { initVar, dummyVar } );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { forNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if given an AST with a for node that has no
 * symbol table.
 */
BOOST_AUTO_TEST_CASE( NoSymbolTable )
{
    const std::string initVar{ "initVar" };
    AstNode::Ptr initAssign = CreateAssignNodeFromByteValue( initVar, 0u, IsDeclaration::TRUE );
    constexpr uint8_t conditionValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE,
                                                              std::make_shared< Token >( T::BYTE, conditionValue ) );
    constexpr uint8_t increment{ 1u };
    AstNode::Ptr initIncrement = CreateTwoOperandStatement( initVar, IsDeclaration::FALSE, T::PLUS, initVar, increment );
    AstNode::Children initChildren{ initAssign, conditionNode, initIncrement };
    AstNode::Ptr initNode = std::make_shared< AstNode >( NT::For_init, initChildren );

    const std::string dummyVar{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVar, 5u, IsDeclaration::TRUE );

    AstNode::Children forChildren{ initNode, dummyAssign };
    AstNode::Ptr forNode = std::make_shared< AstNode >( T::FOR, forChildren );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { forNode } );
    // Give the block node a symbol table, but not the for node.
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will successfully convert a valid for loop sub-tree.
 */
BOOST_AUTO_TEST_CASE( For_Success )
{
    const std::string initVar{ "initVar" };
    constexpr uint8_t initValue{ 0u };
    AstNode::Ptr initAssign = CreateAssignNodeFromByteValue( initVar, initValue, IsDeclaration::TRUE );
    constexpr uint8_t conditionValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE,
                                                              std::make_shared< Token >( T::BYTE, conditionValue ) );
    constexpr uint8_t increment{ 1u };
    AstNode::Ptr incrementNode = CreateTwoOperandStatement( initVar, IsDeclaration::FALSE, T::PLUS, initVar, increment );
    AstNode::Children initChildren{ initAssign, conditionNode, incrementNode };
    AstNode::Ptr initNode = std::make_shared< AstNode >( NT::For_init, initChildren );

    const std::string dummyVar{ "dummyVar" };
    constexpr uint8_t dummyValue{ 5u };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVar, 5u, IsDeclaration::TRUE );

    AstNode::Children forChildren{ initNode, dummyAssign };
    AstNode::Ptr forNode = std::make_shared< AstNode >( T::FOR, forChildren );
    CreateAndAttachFakeSymbolTable( forNode, { initVar, dummyVar } );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { forNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );


    mock::sequence s;

    // Expect the init assign statement to be added
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s )
        .calls(
            [&]( std::string, Operand operand )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand ) );
                BOOST_CHECK_EQUAL( initValue, std::get< Literal >( operand ) );
            }
    );

    // Expect conditional branch - if condition == 0, branch to end.
    // Expect it to have a label so it can be jumped to after one loop is done.
    const std::string conditionLabelToReturn{ "conditionLabel" };
    MOCK_EXPECT( m_instrFactoryMock->GetNewLabel )
        .once()
        .in( s )
        .returns( conditionLabelToReturn );
    MOCK_EXPECT( m_instrFactoryMock->SetNextInstructionLabel )
        .once()
        .in( s )
        .with( conditionLabelToReturn );
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand1 ) );
                BOOST_CHECK_EQUAL( conditionValue, std::get< Literal >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( 0u, std::get< Literal >( operand2 ) );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToEndInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToEndInstr );

    // Expect the for block contents to be added, i.e. the dummy assign
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s )
        .calls(
            [&]( std::string, Operand operand )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand ) );
                BOOST_CHECK_EQUAL( dummyValue, std::get< Literal >( operand ) );
            }
        );

    // Expect statement 2 from the init to be added.
    const Opcode expectedTacOpcode{ Opcode::ADD };
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .with( mock::any, expectedTacOpcode, mock::any, mock::any )
        .calls(
            [&]( std::string, TAC::Opcode, TAC::Operand operand1, TAC::Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                // Can't check the operand contents because a new unique id is calculated.

                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( increment, std::get< Literal >( operand2 ) );
            }
    );

    // Expect an unconditional jump to the condition label
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( conditionLabelToReturn, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                // The contents of the operands don't matter, only that they are equal.
                BOOST_CHECK( operand1 == operand2 );
            }
        );

    // Expect the end target label to be set after all the instructions have been added.
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToEndInstr, mock::any );


    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

BOOST_AUTO_TEST_SUITE_END() // For

BOOST_AUTO_TEST_SUITE( While )

/**
 * Tests that the method for converting an AST will throw an error if given an AST with a while node with the
 * wrong number of children.
 */
BOOST_AUTO_TEST_CASE( WrongNumChildren )
{
    constexpr uint8_t byteValue{ 1u };
    AstNode::Ptr conditionNode = std::make_shared< AstNode >( T::BYTE, std::make_shared< Token >( T::BYTE, byteValue ) );

    AstNode::Children oneChild{ conditionNode };
    AstNode::Ptr whileNode = std::make_shared< AstNode >( T::WHILE, oneChild );
    CreateAndAttachFakeSymbolTable( whileNode, {} );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { whileNode } );
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will throw an error if given an AST with a while node that has no
 * symbol table.
 */
BOOST_AUTO_TEST_CASE( NoSymbolTable )
{
    // Make expression: a <= b
    const std::string varA{ "a" };
    const std::string varB{ "b" };
    const T conditionNodeLabel{ T::LEQ };
    AstNode::Ptr conditionNode = CreateTwoOpExpression( conditionNodeLabel, varA, varB );

    const std::string dummyVar{ "dummyVar" };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVar, 5u, IsDeclaration::TRUE );

    AstNode::Children whileChildren{ conditionNode, dummyAssign };
    AstNode::Ptr whileNode = std::make_shared< AstNode >( T::WHILE, whileChildren );

    AstNode::Ptr blockNode = WrapNodesInBlocks( { whileNode } );
    // Give the block node a symbol table, but not the for node.
    CreateAndAttachFakeSymbolTable( blockNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( blockNode ), std::invalid_argument );
}

/**
 * Tests that the method for converting an AST will successfully convert a while loop sub-tree.
 */
BOOST_AUTO_TEST_CASE( While_Success )
{
    // Make expression: a <= b
    const std::string varA{ "a" };
    const std::string varB{ "b" };
    const T conditionNodeLabel{ T::LEQ };
    AstNode::Ptr conditionNode = CreateTwoOpExpression( conditionNodeLabel, varA, varB );

    const std::string dummyVar{ "dummyVar" };
    constexpr uint8_t dummyValue{ 5u };
    AstNode::Ptr dummyAssign = CreateAssignNodeFromByteValue( dummyVar, 5u, IsDeclaration::TRUE );

    AstNode::Children whileChildren{ conditionNode, dummyAssign };
    AstNode::Ptr whileNode = std::make_shared< AstNode >( T::WHILE, whileChildren );
    CreateAndAttachFakeSymbolTable( whileNode, { varA, varB, dummyVar } );
    AstNode::Ptr blockNode = WrapNodesInBlocks( { whileNode } );
    // Give the block node a symbol table, but not the for node.
    CreateAndAttachFakeSymbolTable( blockNode, {} );

    mock::sequence s;

    // Expect conditional branch - if condition == 0, branch to end.
    // Expect it to have a label so it can be jumped to after one loop is done.
    const std::string conditionLabelToReturn{ "conditionLabel" };
    MOCK_EXPECT( m_instrFactoryMock->GetNewLabel )
        .once()
        .in( s )
        .returns( conditionLabelToReturn );
    MOCK_EXPECT( m_instrFactoryMock->SetNextInstructionLabel )
        .once()
        .in( s )
        .with( conditionLabelToReturn );
    // Expect the condition operand to be fetched
    const Operand conditionOperandToReturn{ "conditionOperand" };
    MOCK_EXPECT( m_exprGeneratorMock->Leq )
        .once()
        .in( s )
        .calls(
            [&]( Operand operand1, Operand operand2 )
            {
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand1 ) );
                BOOST_REQUIRE( std::holds_alternative< std::string >( operand2 ) );

                return conditionOperandToReturn;
            }
        );
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( TacInstructionFactory::PLACEHOLDER, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                BOOST_CHECK( conditionOperandToReturn == operand1 );
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand2 ) );
                BOOST_CHECK_EQUAL( 0u, std::get< Literal >( operand2 ) );
            }
        );
    // Expect a get request for this instruction so it can go back later and assign the target label - return mock.
    ThreeAddrInstruction::Ptr branchToEndInstr = MakeDummyUniqueTacInstr();
    MOCK_EXPECT( m_instrFactoryMock->GetLatestInstruction )
        .once()
        .in( s )
        .returns( branchToEndInstr );

    // Expect the for block contents to be added, i.e. the dummy assign
    MOCK_EXPECT( m_instrFactoryMock->AddAssignmentInstruction )
        .once()
        .in( s )
        .calls(
            [&]( std::string, Operand operand )
            {
                BOOST_REQUIRE( std::holds_alternative< Literal >( operand ) );
                BOOST_CHECK_EQUAL( dummyValue, std::get< Literal >( operand ) );
            }
        );

    // Expect an unconditional jump to the condition label
    MOCK_EXPECT( m_instrFactoryMock->AddInstruction )
        .once()
        .in( s )
        .with( conditionLabelToReturn, Opcode::BRE, mock::any, mock::any )
        .calls(
            [&]( std::string, Opcode, Operand operand1, Operand operand2 )
            {
                // The contents of the operands don't matter, only that they are equal.
                BOOST_CHECK( operand1 == operand2 );
            }
        );

    // Expect the end target label to be set after all the instructions have been added.
    MOCK_EXPECT( m_instrFactoryMock->SetInstructionBranchToNextLabel )
        .once()
        .in( s )
        .with( branchToEndInstr, mock::any );

    m_codeGenerator->GenerateIntermediateCode( blockNode );
}

BOOST_AUTO_TEST_SUITE_END() // While

BOOST_AUTO_TEST_SUITE_END() // TerminalSymbolTests

BOOST_AUTO_TEST_SUITE( NonTerminalSymbolTests )

/**
 * Tests that the method for converting an AST will throw an error if given an AST with an invalid non-terminal symbol,
 * i.e., it is not a 'Block'.
 */
BOOST_AUTO_TEST_CASE( InvalidNodeSymbol )
{
    AstNode::Children fakeChildren{ nullptr };
    constexpr NT invalidNodeLabel{ NT::Negation }; // Not block
    AstNode::Ptr invalidNode = std::make_shared< AstNode >( invalidNodeLabel, fakeChildren );
    CreateAndAttachFakeSymbolTable( invalidNode, {} );
    BOOST_CHECK_THROW( m_codeGenerator->GenerateIntermediateCode( invalidNode ), std::invalid_argument );
}

// Non-Terminal:
// - Invalid node symbol (not Block)
// - Multiple terminals
// - Multiple nested terminals

BOOST_AUTO_TEST_SUITE_END() // NonTerminalSymbolTests

BOOST_AUTO_TEST_SUITE_END() // IntermediateCodeTests