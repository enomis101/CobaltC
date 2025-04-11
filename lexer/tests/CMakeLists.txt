# Include GoogleTest module for test discovery
include(GoogleTest)

# Define the list of test files
set(TEST_FILES
    # Add other test files here
)

# Create an executable for each test file
foreach(TEST_FILE ${TEST_FILES})
    # Extract the test name from the file name (removing extension)
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    
    # Create executable
    add_executable(${TEST_NAME} ${TEST_FILE})
    
    # Link against project libraries and Google Test/Mock
    # (FetchContent makes targets like gtest, gtest_main, gmock, gmock_main available)
    target_link_libraries(${TEST_NAME}
        PRIVATE
        ${COMMON_LIB_TARGET}
        ${LEXER_LIB_TARGET}
        gtest
        gtest_main
        gmock
        gmock_main
        fmt::fmt
    )
    
    # Add include directories if needed
    target_include_directories(${TEST_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lexer/include
    )
    
    # Discover tests
    gtest_discover_tests(${TEST_NAME})
endforeach()