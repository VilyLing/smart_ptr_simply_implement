cmake_minimum_required(VERSION 3.28)
project(TemplatePrimer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)

if(CMAKE_CXX_SANITIZERS)
    add_compile_options(-fsanitize=${CMAKE_CXX_SANITIZERS})
    add_link_options(-fsanitize=${CMAKE_CXX_SANITIZERS})
endif()

add_executable(app ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)

target_link_libraries(app PUBLIC GTest::gtest GTest::gtest_main)

target_include_directories(app PUBLIC 
 ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(app)
