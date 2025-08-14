#include <iostream>
#include <string>
#include "Tokeniser.h"
#include "FileIO.h"
#include "AstGenerator.h"
#include "Logger.h"
#include "SymbolTableGenerator.h"

/**
 * \brief  Runs compiler steps to produce generated assembly language.
 *
 * \param[in]  inputFile   Input file path, containing high-level code.
 * \param[in]  outputFile  Output file path, containing generated assembly.
 *
 * \return  True if successful, false otherwise.
 */
bool
RunCompiler(
    const std::string& inputFile,
    const std::string& outputFile
)
{
    Tokens tokens;
    try
    {
        LOG_AND_COUT( "Converting program file into tokens..." );
        std::string inputFileString = FileIO::ReadFileToString( inputFile );
        Tokeniser::UPtr tokeniser = std::make_unique< Tokeniser >();
        tokens = tokeniser->ConvertStringToTokens( inputFileString );

        if ( tokens.empty() )
        {
            LOG_WARN( "No tokens found - is your program file empty?" );
            return true;
        }
    }
    catch ( std::exception& e )
    {
        LOG_ERROR( "Caught exception while converting file to tokens: " + std::string( e.what() ) );
        return false;
    }
    LOG_AND_COUT( "Successfully converted into tokens!" );

    AstNode::Ptr abstractSyntaxTree;
    try
    {
        LOG_AND_COUT( "Converting tokens into an abstract syntax tree..." );
        AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >();
        constexpr GrammarSymbols::NT startingNtSymbol { Block };
        abstractSyntaxTree = astGenerator->GenerateAst( tokens, startingNtSymbol, false );

        if ( nullptr == abstractSyntaxTree )
        {
            LOG_ERROR( "Failed to generate abstract syntax tree: nullptr was returned." );
            return false;
        }
    }
    catch ( std::exception& e )
    {
        LOG_ERROR( "Caught exception while generating abstract syntax tree: " + std::string( e.what() ) );
        return false;
    }
    LOG_AND_COUT( "Successfully created abstract syntax tree!" );

    SymbolTable::Ptr symbolTable;
    try
    {
        LOG_AND_COUT( "Generating symbol table from abstract syntax tree..." );
        SymbolTableGenerator::UPtr symbolTableGenerator = std::make_unique< SymbolTableGenerator >();
        symbolTableGenerator->GenerateSymbolTableForAst( abstractSyntaxTree );

        symbolTable = abstractSyntaxTree->m_symbolTable;

        if ( nullptr == symbolTable )
        {
            LOG_ERROR( "Failed to generate symbol table: no table assigned to tree node." );
            return false;
        }
    }
    catch ( std::exception& e )
    {
        LOG_ERROR( "Caught exception while creating symbol table: " + std::string( e.what() ) );
        return false;
    }
    LOG_AND_COUT( "Successfully created symbol table!" );

    // TODO: consider defining custom exception types, e.g. syntax error, to specify whether error is internal or from
    // incorrect input. Consider how better to display non-internal errors, e.g. to the terminal as well as the logs.

    // TODO: Generate assembly code here...
    LOG_WARN( "No further stages of compilation have been added yet: exiting program." );
    return false;
}

/**
 * \brief  Prints help message to console.
 */
void
PrintHelpMessage()
{
    std::string helpMsg = "Command line arguments:\n";
    helpMsg += "-h (--help)\tPrints this message.\n";
    helpMsg += "-i (--input)\tPath to input file containing code to be compiled.\n";
    helpMsg += "-o (--output)\tPath to output file containing generated assembly language."
               " If left blank will default to ./output.txt\n";
    helpMsg += "-l (--logLevel)\tLogging level:\n"
               "\t\t- 0: NONE\n\t\t- 1: ERROR\n\t\t- 2: WARN\n\t\t- 3: INFO\n\t\t"
               "- 4: INFO_MEDIUM_LEVEL\n\t\t- 5: INFO_LOW_LEVEL\n";
    std::cout << helpMsg;
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
            PrintHelpMessage();
        }
        else if ( "--input" == currentArg || "-i" == currentArg )
        {
            ++index;
            if ( argc <= index )
            {
                std::string errMsg = "No value given for input file argument.";
                std::cout << errMsg << "\n\n";
                PrintHelpMessage();
                return -1;
            }
            inputFile = argv[index];
        }
        else if ( "--output" == currentArg || "-o" == currentArg )
        {
            ++index;
            outputFile = argv[index];
            if ( argc <= index )
            {
                std::string errMsg = "No value given for output file argument.";
                std::cout << errMsg << "\n\n";
                PrintHelpMessage();
                return -1;
            }
        }
        else if ( "--logLevel" == currentArg || "-l" == currentArg )
        {
            ++index;
            std::string logLevelStr = argv[index];
            if ( argc <= index )
            {
                std::string errMsg = "No value given for log level argument.";
                std::cout << errMsg << "\n\n";
                PrintHelpMessage();
                return -1;
            }

            try
            {
                int logLevelInt = std::stoi( logLevelStr );
                Logger::GetInstance()->SetLogLevel( static_cast< LogLevel >( logLevelInt ) );
            }
            catch ( std::exception& e )
            {
                // If log level not int, parse the string form
                const std::unordered_map< std::string, LogLevel > logLevelStrMappings
                {
                    { "NONE", LogLevel::NONE },
                    { "ERROR", LogLevel::ERROR },
                    { "WARN", LogLevel::WARN },
                    { "INFO", LogLevel::INFO },
                    { "INFO_MEDIUM_LEVEL", LogLevel::INFO_MEDIUM_LEVEL },
                    { "INFO_LOW_LEVEL", LogLevel::INFO_LOW_LEVEL }
                };

                if ( 0 < logLevelStrMappings.count( logLevelStr ) )
                {
                    Logger::GetInstance()->SetLogLevel( logLevelStrMappings.find( logLevelStr )->second );
                }
                else
                {
                    std::string errMsg = "Log level argument '" + logLevelStr + "' not recognised.";
                    std::cout << errMsg << "\n\n";
                    PrintHelpMessage();
                    return -1;
                }
            }
        }

        ++index;
    }

    if ( !helpCalled )
    {
        if ( inputFile.empty() )
        {
            std::string errMsg = "No input file argument provided.";
            std::cout << errMsg << "\n\n";
            PrintHelpMessage();
            return -1;
        }
        if ( outputFile.empty() )
        {
            // Fill in default value
            outputFile = "output.txt";
        }

        if ( !RunCompiler( inputFile, outputFile ) )
        {
            LOG_ERROR( "RunCompiler() returned false: exception raised during runtime." );
            std::cout << "Compilation failed. See log for more details.\n";
            return -1;
        }
        else
        {
            LOG_AND_COUT( "Compilation successful!" );
            return 0;
        }
    }
}