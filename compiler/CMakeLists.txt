# Add this at the top of your CMakeLists.txt file

# Define a shared library target


# Collect all .cpp files in the src directory excluding main.cpp
file(GLOB_RECURSE LIB_SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.cc
)

# Create the shared library
add_library(${COMPILER_LIB_TARGET} SHARED
    ${LIB_SRC_FILES}
)

string(REPLACE "-lib" "" COMPILER_LIB_OUTPUT_NAME "${COMPILER_LIB_TARGET}")
set_target_properties(${COMPILER_LIB_TARGET} PROPERTIES OUTPUT_NAME "${COMPILER_LIB_OUTPUT_NAME}")

# Set include directories for the library
target_include_directories(${COMPILER_LIB_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)

# Link the library dependencies
target_link_libraries(${COMPILER_LIB_TARGET}
    PUBLIC
        ${COMMON_LIB_TARGET}
        ${LEXER_LIB_TARGET}
        ${PARSER_LIB_TARGET}
        ${TACKY_LIB_TARGET}
        ${BACKEND_LIB_TARGET}
)

# Create the executable with just main.cpp
add_executable(${COMPILER_APP_TARGET}
    main.cpp
)

target_include_directories(${COMPILER_APP_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)

# Link the executable to the library
target_link_libraries(${COMPILER_APP_TARGET}
    PRIVATE
        ${COMPILER_LIB_TARGET}
)


# For testing
if(ENABLE_TESTING)
    message(STATUS "Building tests for compiler.")
    # Add the tests directory
    add_subdirectory(tests)
endif()

# Add a custom command to copy files during build
add_custom_command(
    TARGET ${COMPILER_APP_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/sources
            ${CMAKE_CURRENT_BINARY_DIR}/
    COMMENT "Copying resources to build directory"
)