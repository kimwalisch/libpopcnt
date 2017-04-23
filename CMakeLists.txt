cmake_minimum_required(VERSION 2.8)
project(libpopcnt C CXX)
set(CMAKE_BUILD_TYPE Release)
include_directories(.)

add_executable(benchmark benchmark.cpp)
enable_testing()
add_subdirectory(test)

install(FILES libpopcnt.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
