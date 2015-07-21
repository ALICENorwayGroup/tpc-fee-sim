# Check if cmake has the required version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11 FATAL_ERROR)

# Set name of project
project(AliceTPC-FEE-simulation)

#Check the compiler and set the compile and link flags
If(NOT CMAKE_BUILD_TYPE)
  Message("Set BuildType DEBUG")
  set(CMAKE_BUILD_TYPE Debug)
EndIf(NOT CMAKE_BUILD_TYPE)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(INCLUDE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/include")

# Check if the user attempts to build the project in the source
# directory
#CHECK_OUT_OF_SOURCE_BUILD()

# Check if we are on an UNIX system. If not stop with an error
# message
IF(NOT UNIX)
  MESSAGE(FATAL_ERROR "You're not on an UNIX system. The project was up to now only tested on UNIX systems, so we break here. If you want to go on please edit the CMakeLists.txt in the source directory.")
ENDIF(NOT UNIX)

# Check if the compiler support specific C++11 features
#include(CheckCXX11Features)

# Initialize CXXFLAGS.
set(CMAKE_CXX_FLAGS                "-Wall -std=c++11")
#set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
#set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Check if the compilation flag -std=c++11 is set
#If(NOT CMAKE_CXX_FLAGS)
#  Message(FATAL_ERROR "C++11 support required, not found ... aborting.")
#EndIF(NOT CMAKE_CXX_FLAGS)

# supported from cmake 3.1.0, activate later
#target_compile_features(foobar PRIVATE cxx_range_for)

Message(STATUS "CMAKE_C_FLAGS:      ${CMAKE_C_FLAGS}")
Message(STATUS "CMAKE_CXX_FLAGS:    ${CMAKE_CXX_FLAGS}")
Message(STATUS "CMAKE_SOURCE_DIR:   ${CMAKE_SOURCE_DIR}")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}" ${CMAKE_MODULE_PATH})
FIND_PACKAGE(SystemC)

add_subdirectory (CRU)
add_subdirectory (generator)
