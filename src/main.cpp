#include <iostream>
#include <memory>
int main() {
    #ifdef _LIBCPP_VERSION
        std::cout <<"Clang" <<_LIBCPP_VERSION <<std::endl;
    #else
        std::cout <<"Gcc" << std::endl;
    #endif    

    


    return 0;
 }
