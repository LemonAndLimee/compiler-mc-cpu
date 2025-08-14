/**
 * Declaration of class responsible for writing logs.
 */

#pragma once

#include <string>
#include <memory>
#include <iostream>

enum LogLevel
{
    NONE,
    ERROR,
    WARN,
    INFO,
    INFO_MEDIUM_LEVEL,
    INFO_LOW_LEVEL
};

class Logger
{
public:
    using Ptr = std::shared_ptr< Logger >;

    Logger( LogLevel logLevel );
    static Ptr GetInstance() {
        // Default to log level INFO
        static Ptr instance = std::make_shared< Logger >( LogLevel::INFO );
        return instance;
    }
    ~Logger() = default;

    void LogMessage( LogLevel logLevel, const std::string message, const char* codeFile, const char* codeFunc, int lineNum );

    #define LOG( logLevel, message ) Logger::GetInstance()->LogMessage( logLevel, message, __FILE__, __FUNCTION__, __LINE__ )
    #define LOG_ERROR( message ) LOG( LogLevel::ERROR, message )
    #define LOG_WARN( message ) LOG( LogLevel::WARN, message )
    #define LOG_INFO( message ) LOG( LogLevel::INFO, message )
    #define LOG_INFO_MEDIUM_LEVEL( message ) LOG( LogLevel::INFO_MEDIUM_LEVEL, message )
    #define LOG_INFO_LOW_LEVEL( message ) LOG( LogLevel::INFO_LOW_LEVEL, message )

    template< class E >
    void LogAndThrow(
        LogLevel logLevel,
        const std::string message,
        const char* codeFile,
        const char* codeFunc,
        int lineNum )
    {
        static_assert( std::is_base_of<std::exception, E>{} );
        LogMessage( logLevel, message, codeFile, codeFunc, lineNum );
        throw E( message );
    }

    static void LogAndCout(
        const std::string message
    )
    {
        LOG_INFO( message );
        std::cout << message + "\n";
    }

    #define LOG_AND_THROW( logLevel, message, eType ) Logger::GetInstance()->LogAndThrow<eType>( logLevel, message, __FILE__, __FUNCTION__, __LINE__ )
    #define LOG_ERROR_AND_THROW( message, eType ) LOG_AND_THROW( LogLevel::ERROR, message, eType )

    #define LOG_AND_COUT( message ) Logger::LogAndCout( message )

    void SetLogLevel( LogLevel level );

private:
    std::string LogLevelToString( LogLevel logLevel );

    LogLevel m_logLevel;
    std::string m_logFilePath;
};