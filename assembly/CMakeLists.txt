# Collect all .cpp files in the src directory excluding main.cpp
file(GLOB_RECURSE LIB_SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.cc
)

# Create the shared library
add_library(${ASSEMBLY_LIB_TARGET} SHARED
    ${LIB_SRC_FILES}
)

string(REPLACE "-lib" "" ASSEMBLY_LIB_OUTPUT_NAME "${ASSEMBLY_LIB_TARGET}")
set_target_properties(${ASSEMBLY_LIB_TARGET} PROPERTIES OUTPUT_NAME "${ASSEMBLY_LIB_OUTPUT_NAME}")

# Set include directories for the library
target_include_directories(${ASSEMBLY_LIB_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)

# Link the library dependencies
target_link_libraries(${ASSEMBLY_LIB_TARGET}
    PUBLIC
        ${COMMON_LIB_TARGET}
        ${PARSER_LIB_TARGET}
        ${TACKY_LIB_TARGET}
)

# Create the executable with just main.cpp
add_executable(${ASSEMBLY_APP_TARGET}
    main.cpp
)

# Link the executable to the library
target_link_libraries(${ASSEMBLY_APP_TARGET}
    PRIVATE
        ${COMMON_LIB_TARGET}
        ${ASSEMBLY_LIB_TARGET}
)


# For testing
if(ENABLE_TESTING)
    message(STATUS "Building tests for assembly.")
    # Add the tests directory
    add_subdirectory(tests)
endif()