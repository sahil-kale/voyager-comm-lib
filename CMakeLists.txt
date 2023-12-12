cmake_minimum_required(VERSION 3.14)

set(This tests)

set(BINARY ${CMAKE_PROJECT_NAME})

project(${This} C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_GMOCK ON)

#enable Werror, Wextra, Wall, pedantic, and pedantic-errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wextra -Wall -Og -g -pedantic -pedantic-errors")

# enable testing
enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest)

# Glob recurse the sources inside control_loop/
file(GLOB_RECURSE GLOBBED_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Add a compiler definition -DUNITTEST to the compiler
add_definitions(-DUNIT_TEST)

# Glob recurse the headers inside test/
file(GLOB_RECURSE TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/test/*.hpp)
# Glob recurse the sources inside test/
file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)

# Include the headers in the above paths and also do so recursively as there is a hierarchy
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Mocks
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test/mocks)

# Test msgs
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test/test_msgs)

# Add an executable with the above sources
add_executable(${This} ${GLOBBED_SOURCE_FILES} ${UTILS_SOURCES} ${TEST_SOURCES})

# Link the executable with the GoogleTest libraries
target_link_libraries(${This} PUBLIC gtest gtest_main gmock)
