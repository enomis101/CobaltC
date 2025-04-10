# Add this at the top of your CMakeLists.txt file

# Define a shared library target
set(SCANNER_LIB_TARGET scanner)
set(SCANNER_APP_TARGET scanner)

# Collect all .cpp files in the src directory excluding main.cpp
file(GLOB_RECURSE LIB_SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.cc
)

# Create the shared library
add_library(${SCANNER_LIB_TARGET} SHARED
    ${LIB_SRC_FILES}
)

# Set include directories for the library
target_include_directories(${SCANNER_LIB_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)

# Link the library dependencies
target_link_libraries(${SCANNER_LIB_TARGET}
    PUBLIC
        ${COMMON_LIB_TARGET}
)

# Create the executable with just main.cpp
add_executable(${SCANNER_APP_TARGET}
    main.cpp
)

# Link the executable to the library
target_link_libraries(${SCANNER_APP_TARGET}
    PRIVATE
        ${COMMON_LIB_TARGET}
        ${SCANNER_LIB_TARGET}
)


# For testing
if(ENABLE_TESTING)
    message(STATUS "Building tests for scanner.")
    # Add the tests directory
    add_subdirectory(tests)
endif()