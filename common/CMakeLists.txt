
# Collect all .cc files in the src directory and its subdirectories
file(GLOB_RECURSE SRC_FILES src/*.cc src/*.cpp)


add_library(${COMMON_LIB_TARGET} SHARED ${SRC_FILES})

# Always link the required libraries
target_link_libraries(${COMMON_LIB_TARGET} 
    PUBLIC 
        fmt::fmt
)


target_include_directories(${COMMON_LIB_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)

if(ENABLE_TESTING)
    message(STATUS "Building tests for common.")
    
    # Add the tests directory
    add_subdirectory(tests)
endif()
