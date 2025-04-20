#!/bin/bash

# Check if arguments were provided
if [ $# -eq 0 ]; then
    echo "Error: No input files specified."
    echo "Usage: $0 path/to/file.dot [path/to/other/files.dot ...]"
    echo "Example: $0 ./debug/*.dot"
    exit 1
fi

# Process each file
for dot_file in "$@"; do
    # Check if the file exists
    if [ ! -f "$dot_file" ]; then
        echo "Warning: File '$dot_file' not found. Skipping."
        continue
    fi
    
    # Check if it's a .dot file
    if [[ ! "$dot_file" == *.dot ]]; then
        echo "Warning: '$dot_file' is not a .dot file. Skipping."
        continue
    fi
    
    # Get the base name without the .dot extension
    base_name="${dot_file%.dot}"
    
    # Run dot command
    echo "Generating PNG from $dot_file..."
    dot -Tpng "$dot_file" -o "${base_name}.png"
    
    # Check if the command was successful
    if [ $? -eq 0 ]; then
        echo "Success! Created ${base_name}.png"
    else
        echo "Error: Failed to generate PNG for $dot_file."
    fi
done

echo "All processing complete."