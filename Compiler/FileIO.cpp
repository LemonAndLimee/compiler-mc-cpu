/**
 * Definition of utility methods for reading from and writing to files.
 */

#include "FileIO.h"
#include "Logger.h"
#include <fstream>

/**
 * \brief  Reads entire file into string.
 *
 * \param[in]  filePath  Path to file.
 *
 * \return  String containing the contents of the file.
 */
std::string
FileIO::ReadFileToString(
    const std::string& filePath
)
{
    std::ifstream file( filePath );
    if ( !file.is_open() )
    {
        std::string errMsg = "Failed to open file " + filePath;
        LOG_ERROR( errMsg );
        throw std::invalid_argument( errMsg );
    }

    std::string fileString;

    std::string line;
    while ( std::getline( file, line ) )
    {
        fileString += line + "\n";
    }

    return fileString;
}

/**
 * \brief  Appends line to the end of given file.
 *
 * \param[in]  line      String to append to file.
 * \param[in]  filePath  Path to file.
 */
void
FileIO::AppendLineToFile(
    const std::string& line,
    const std::string& filePath
)
{
    std::ofstream file;
    file.open( filePath, std::ios::app );

    file << line + "\n";
}