/**
 * Declaration of utility methods for reading from and writing to files.
 */

#include <string>

namespace FileIO
{
    std::string ReadFileToString( const std::string& filePath );

    void AppendLineToFile( const std::string& line, const std::string& filePath );
}