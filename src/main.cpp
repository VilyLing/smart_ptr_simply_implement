#include "Unique_ptr.h"
#include <iostream>
#include <memory>
int main() {
    #ifdef _LIBCPP_VERSION
        std::cout <<"Clang" <<_LIBCPP_VERSION <<std::endl;
    #else
        std::cout <<"Gcc" << std::endl;
    #endif    

    
    std::unique_ptr<int > uptr;
    uptr.get_deleter();
    return 0;
 }
