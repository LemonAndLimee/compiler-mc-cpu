#include <iostream>
#include <string>
#include "Tokeniser.h"
#include "FileIO.h"
#include "AstGenerator.h"
#include "Logger.h"

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
    TokensVector tokens;
    try
    {
        std::string inputFileString = FileIO::ReadFileToString( inputFile );
        Tokeniser::UPtr tokeniser = std::make_unique< Tokeniser >();
        tokens = tokeniser->ConvertStringToTokens( inputFileString );
    }
    catch ( std::exception& e )
    {
        LOG_ERROR( "Caught exception while converting file to tokens: " + std::string( e.what() ) );
        return false;
    }

    AstNode::Ptr abstractSyntaxTree;
    try
    {
        AstGenerator::UPtr astGenerator = std::make_unique< AstGenerator >();
        constexpr GrammarSymbols::NT startingNtSymbol { Block };
        abstractSyntaxTree = astGenerator->GenerateAst( tokens, startingNtSymbol, false );
    }
    catch ( std::exception& e )
    {
        LOG_ERROR( "Caught exception while generating abstract syntax tree: " + std::string( e.what() ) );
        return false;
    }

    // TODO: Generate assembly code here...
    LOG_WARN( "No further stages of compilation have been added yet: exiting program." );
    return false;
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
            helpMsg += "-l (--logLevel)\tLogging level:\n"
                       "\t\t- 0: NONE\n\t\t- 1: ERROR\n\t\t- 2: WARN\n\t\t- 3: INFO\n\t\t- 4: INFO_LOW_LEVEL\n";
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
        else if ( "--logLevel" == currentArg || "-l" == currentArg )
        {
            ++index;
            std::string logLevelStr = argv[index];
            if ( argc <= index )
            {
                throw std::invalid_argument( "No value given for log level argument." );
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
                    { "INFO_LOW_LEVEL", LogLevel::INFO_LOW_LEVEL }
                };

                if ( 0 < logLevelStrMappings.count( logLevelStr ) )
                {
                    Logger::GetInstance()->SetLogLevel( logLevelStrMappings.find( logLevelStr )->second );
                }
                else
                {
                    throw std::invalid_argument( "Log level argument '" + logLevelStr + "' not recognised." );
                }
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

        if ( !RunCompiler( inputFile, outputFile ) )
        {
            LOG_ERROR( "RunCompiler() returned false: exception raised during runtime." );
            std::cout << "Compilation failed. See log for more details.\n";
        }
        else
        {
            std::cout << "Compilation successful!\n";
        }
    }
}