cmake_minimum_required(VERSION 3.28)
project(TemplatePrimer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES 
${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_executable(app ${SOURCES})

target_include_directories(app PRIVATE include)