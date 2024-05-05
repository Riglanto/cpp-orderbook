cmake_minimum_required(VERSION 3.5.0)
project(sq VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(sq main.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
