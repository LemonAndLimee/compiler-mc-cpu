/**
 * Declaration of class responsible for writing logs.
 */

#include <string>

#ifndef LOG_LEVEL
    #define LOG_LEVEL LogLevel::INFO
#endif

enum LogLevel
{
    NONE,
    ERROR,
    WARN,
    INFO,
    INFO_LOW_LEVEL
};

class Logger
{
public:
    using Ptr = std::shared_ptr< Logger >;

    Logger( LogLevel logLevel );
    static Ptr GetInstance() {
        static Ptr instance = std::make_shared< Logger >( LOG_LEVEL );
        return instance;
    }
    ~Logger() = default;

    void LogMessage( LogLevel logLevel, const std::string message, const char* codeFile, const char* codeFunc, int lineNum );
    #define LOG( logLevel, message ) Logger::GetInstance()->LogMessage( logLevel, message, __FILE__, __FUNCTION__, __LINE__ )
    #define LOG_ERROR( message ) LOG( LogLevel::ERROR, message )
    #define LOG_WARN( message ) LOG( LogLevel::WARN, message )
    #define LOG_INFO( message ) LOG( LogLevel::INFO, message )
    #define LOG_INFO_LOW_LEVEL( message ) LOG( LogLevel::INFO_LOW_LEVEL, message )

private:
    std::string LogLevelToString( LogLevel logLevel );

    LogLevel m_logLevel;
    std::string m_logFilePath;
};