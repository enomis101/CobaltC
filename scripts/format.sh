#!/bin/bash

# Default directories to process if no arguments are provided
# You can modify this array with your preferred hardcoded directories
DEFAULT_DIRS=("./common" "./lexer")

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format not found. Please install it first."
    echo "  For Ubuntu: sudo apt update && sudo apt install clang-format"
    exit 1
fi

# Check if any arguments were provided
if [ $# -eq 0 ]; then
    echo "No directories specified. Using default directories:"
    for dir in "${DEFAULT_DIRS[@]}"; do
        echo "  - $dir"
    done
    SEARCH_DIRS=("${DEFAULT_DIRS[@]}")
else
    echo "Processing specified directories:"
    for dir in "$@"; do
        echo "  - $dir"
    done
    SEARCH_DIRS=("$@")
fi

# Check if .clang-format exists in the current directory
if [ ! -f "./.clang-format" ]; then
    echo "Warning: No .clang-format file found in the current directory."
    echo "Using clang-format's default style. Consider creating a .clang-format file."
fi

# Process each directory
for dir in "${SEARCH_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        echo "Warning: Directory '$dir' does not exist or is not a directory. Skipping."
        continue
    fi
    
    echo "Applying clang-format to .cc, .cpp, .h, and .hpp files in '$dir'..."
    
    # Find and format all specified file types
    # The -i flag means "in-place" editing (modify files directly)
    find "$dir" -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;
    
    if [ $? -eq 0 ]; then
        echo "Completed formatting directory: $dir"
    else
        echo "Errors occurred while formatting directory: $dir"
    fi
done

echo "Formatting complete."