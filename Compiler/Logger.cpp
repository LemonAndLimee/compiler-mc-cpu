/**
 * Definition of class responsible for writing logs.
 */

#include "Logger.h"
#include "FileIO.h"
#include <ctime>
#include <filesystem>

Logger::Logger( LogLevel logLevel )
: m_logLevel( logLevel )
{
    std::time_t timestamp = time( NULL );
    struct tm datetime = *localtime( &timestamp );

    char datetimeStr[128];
    std::string conversionFormat = "%m_%d_%y__%H_%M_%S";
    std::strftime( datetimeStr, sizeof( datetimeStr ), conversionFormat.c_str(), &datetime );

    const std::string logDir = "./Logs";

    if ( !std::filesystem::exists( logDir ) )
    {
        if ( !std::filesystem::create_directories( logDir ) )
        {
            throw std::runtime_error( "Failed to create log dir: " + logDir );
        }
    }

    m_logFilePath = logDir + "/Compiler__" + std::string( datetimeStr ) + ".txt";
}

/**
 * \brief  Prints message to log file.
 *
 * \param[in]  logLevel   Level of message. Is only logged if is <= m_logLevel.
 * \param[in]  message    The message to be logged.
 * \param[in]  codeFile   Name of the calling program file.
 * \param[in]  codeFunc   Name of the calling function.
 * \param[in]  lineNum    Line number in the calling program file.
 */
void
Logger::LogMessage(
    LogLevel logLevel,
    const std::string message,
    const char* codeFile,
    const char* codeFunc,
    int lineNum 
)
{
    if ( logLevel <= m_logLevel )
    {
        std::string logMessage;
        logMessage += LogLevelToString( logLevel ) + ": ";
        logMessage += std::string( codeFile ) + ", " + std::string( codeFunc );
        logMessage += ", line " + std::to_string( lineNum ) + ": ";
        logMessage += message;
        FileIO::AppendLineToFile( logMessage, m_logFilePath );
    }
}

/**
 * \brief  Converts log level to string form.
 *
 * \param[in]  logLevel  Log level in numeric form.
 *
 * \return  String representing log level in human-readable form.
 */
std::string
Logger::LogLevelToString(
    LogLevel logLevel
)
{
    switch ( logLevel )
    {
        case NONE:
            return "NONE";
        case ERROR:
            return "ERROR";
        case WARN:
            return "WARN";
        case INFO:
            return "INFO";
        case INFO_LOW_LEVEL:
            return "INFO_LOW_LEVEL";
        default:
            throw std::runtime_error( "Unknown log level" + std::to_string( logLevel ) );
    }
}

/**
 * \brief  Sets log level of this logger.
 *
 * \param[in]  level  Log level in numeric form.
 */
void
Logger::SetLogLevel(
    LogLevel level
)
{
    m_logLevel = level;
}