#include <iostream>
#include "Tokeniser.h"
#include "FileIO.h"
#include "AstGenerator.h"

/**
 * \brief  Runs compiler steps to produce generated assembly language.
 *
 * \param[in]  inputFile   Input file path, containing high-level code.
 * \param[in]  outputFile  Output file path, containing generated assembly.
 */
void
RunCompiler(
    const std::string& inputFile,
    const std::string& outputFile
)
{
    std::string inputFileString = FileIO::ReadFileToString( inputFile );
    Tokeniser::UPtr tokeniser = std::make_unique< Tokeniser >();
    TokensVector tokens = tokeniser->ConvertStringToTokens( inputFileString );

    AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >();
    constexpr GrammarSymbols::NT startingNtSymbol { Block };
    AstNode::Ptr abstractSyntaxTree = astGenerator->GenerateAst( tokens, startingNtSymbol, false );

    // TODO: Generate assembly code here...
    throw std::runtime_error( "Not implemented." );
}

int
main(
    int argc,
    char *argv[]
)
{
    // Set to true if help argument is called - in this case do not run the compiler.
    bool helpCalled{ false };
    std::string inputFile, outputFile;

    size_t index = 1u;
    while ( index < argc )
    {
        std::string currentArg = argv[index];
        if ( "--help" == currentArg || "-h" == currentArg )
        {
            helpCalled = true;
            std::string helpMsg = "Command line arguments:\n";
            helpMsg += "-h (--help)\tPrints this message.\n";
            helpMsg += "-i (--input)\tPath to input file containing code to be compiled.\n";
            helpMsg += "-o (--output)\tPath to output file containing generated assembly language."
                       " If left blank will default to ./output.txt\n";
            std::cout << helpMsg;
        }
        else if ( "--input" == currentArg || "-i" == currentArg )
        {
            ++index;
            if ( argc <= index )
            {
                throw std::invalid_argument( "No value given for input file argument." );
            }
            inputFile = argv[index];
        }
        else if ( "--output" == currentArg || "-o" == currentArg )
        {
            ++index;
            outputFile = argv[index];
            if ( argc <= index )
            {
                throw std::invalid_argument( "No value given for output file argument." );
            }
        }

        ++index;
    }

    if ( !helpCalled )
    {
        if ( inputFile.empty() )
        {
            throw std::invalid_argument( "No input file argument provided." );
        }
        if ( outputFile.empty() )
        {
            // Fill in default value
            outputFile = "output.txt";
        }
        RunCompiler( inputFile, outputFile );
    }
}