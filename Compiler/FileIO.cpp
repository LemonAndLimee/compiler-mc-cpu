/**
 * Definition of utility methods for reading from and writing to files.
 */

#include "FileIO.h"
#include <fstream>

/**
 * \brief  Reads entire file into string.
 *
 * \param[in]  filePath  Path to file.
 *
 * \return
 */
std::string
FileIO::ReadFileToString(
    const std::string& filePath
)
{
    std::ifstream file( filePath );
    if ( !file.is_open() )
    {
        throw std::invalid_argument( "Failed to open file " + filePath );
    }

    std::string fileString;

    std::string line;
    while ( std::getline( file, line ) )
    {
        fileString += line + "\n";
    }

    return fileString;
}