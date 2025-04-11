cmake_minimum_required(VERSION 3.10)
project(compiler VERSION 1.0.0)

#Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_TESTING "Enables unit tests generation" OFF)
#option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
#option(ENABLE_ADSB_BASE "Enable adsb-base decoding" ON)
#option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

message(STATUS "Current CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Force default flags
set(CMAKE_CXX_FLAGS "-fPIC -Wall")

# Force debug flags
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

#Force release flags
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Find required packages with enhanced diagnostics
message(STATUS "Searching for required packages...")

# Find fmt
find_package(fmt REQUIRED)
if(fmt_FOUND)
    message(STATUS "Found fmt: ${fmt_VERSION}")
    message(STATUS "  - Include dirs: ${fmt_INCLUDE_DIRS}")
    message(STATUS "  - Libraries: ${fmt_LIBRARIES}")
else()
    message(FATAL_ERROR "fmt package not found. Please install libfmt-dev or equivalent.")
endif()

message(STATUS "All required components successfully configured")

#TARGET NAMES
set(COMMON_LIB_TARGET ${PROJECT_NAME}-common) 
set(LEXER_LIB_TARGET lexer)
set(COMPILER_LIB_TARGET ${PROJECT_NAME})
set(COMPILER_APP_TARGET ${PROJECT_NAME})

if(ENABLE_TESTING)
    # Enable testing
    enable_testing()

    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1  # Use a specific version/tag
    )
    
    # Check for FetchContent errors
    FetchContent_MakeAvailable(googletest)
    
    # Verify googletest was properly fetched
    if(NOT TARGET gtest OR NOT TARGET gtest_main)
        message(FATAL_ERROR "GoogleTest was not properly fetched. Check network connection and git access.")
    else()
        message(STATUS "GoogleTest successfully fetched")
    endif()
endif()


add_subdirectory(common)