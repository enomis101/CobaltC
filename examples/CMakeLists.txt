# CMakeLists.txt - Compile each .c file into a separate executable
cmake_minimum_required(VERSION 3.10)

project(examples)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find all .c files in the current directory
file(GLOB C_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Create an executable for each .c file
foreach(C_FILE ${C_SOURCE_FILES})
    # Get the filename without directory and extension
    get_filename_component(EXEC_NAME ${C_FILE} NAME_WE)
    
    # Create executable with the same name as the .c file (without extension)
    add_executable(${EXEC_NAME} ${C_FILE})
endforeach()

# Optional: Print found files for debugging
message(STATUS "Found C source files: ${C_SOURCE_FILES}")