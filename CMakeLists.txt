cmake_minimum_required(VERSION 3.10)
project(cobaltc VERSION 1.0.0)

#Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_TESTING "Enables unit tests generation" OFF)
#option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
#option(ENABLE_ADSB_BASE "Enable adsb-base decoding" ON)
#option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

configure_file(
  "${PROJECT_SOURCE_DIR}/common/cmake/build_options.h.in"
  "${PROJECT_BINARY_DIR}/common/build_options.h"
)

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
set(COMMON_LIB_TARGET ${PROJECT_NAME}-common-lib) 
set(LEXER_LIB_TARGET ${PROJECT_NAME}-lexer-lib)
set(LEXER_APP_TARGET ${PROJECT_NAME}-lexer)
set(PARSER_LIB_TARGET ${PROJECT_NAME}-parser-lib)
set(PARSER_APP_TARGET ${PROJECT_NAME}-parser)
set(TACKY_LIB_TARGET ${PROJECT_NAME}-tacky-lib)
set(TACKY_APP_TARGET ${PROJECT_NAME}-tacky)
set(ASMGEN_LIB_TARGET ${PROJECT_NAME}-asmgen-lib)
set(ASMGEN_APP_TARGET ${PROJECT_NAME}-asmgen)
set(COMPILER_LIB_TARGET ${PROJECT_NAME}-compiler-lib)
set(COMPILER_APP_TARGET ${PROJECT_NAME}-compiler)

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
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(tacky)
add_subdirectory(asmgen)
add_subdirectory(compiler)

# Set environment variables for the target when run from the build directory
set_property(TARGET ${COMPILER_APP_TARGET} PROPERTY
    ENVIRONMENT "LOG_CONFIG_PATH=${CMAKE_CURRENT_SOURCE_DIR}/configs/log-config.json")