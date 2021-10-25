//
//  main.cpp
//  VkComputeTest
//
//  Created by James Perlman on 10/23/21.
//

#include <iostream>

#include "VulkanComputeApplication.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    
    auto application = VulkanComputeApplication();
    application.run();
    
    return 0;
}
