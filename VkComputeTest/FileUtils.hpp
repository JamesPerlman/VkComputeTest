//
//  FileUtils.hpp
//  VkComputeTest
//
//  Created by James Perlman on 10/23/21.
//

#ifndef FileUtils_hpp
#define FileUtils_hpp

#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <vector>

namespace FileUtils
{

std::vector<char> readLocalFile(const std::string& filename);

}

#endif /* FileUtils_hpp */
