#!/bin/bash

# Path to the test program and compiler
TEST_PROG="../writing-a-c-compiler-tests/test_compiler"
COMPILER="./build/compiler/cobaltc-compiler"

# Create logs and debug directories if they don't exist
mkdir -p ./logs
mkdir -p ./debug

# Delete previous logs and debug files
rm -f ./logs/*
rm -f ./debug/*

# Check if compiler exists
if [ ! -f "$COMPILER" ]; then
    echo "Error: Compiler not found at $COMPILER"
    exit 1
fi

# Check if test program exists
if [ ! -f "$TEST_PROG" ]; then
    echo "Error: Test program not found at $TEST_PROG"
    exit 1
fi

# Run the test program with the compiler and forward all arguments
echo "Running tests with $COMPILER..."
$TEST_PROG $COMPILER "$@"
TEST_RESULT=$?

# Generate graph only if tests passed and debug files exist
# if [ "$(ls -A ./debug 2>/dev/null)" ]; then
#     echo "Tests passed. Generating graph from debug files..."
#     ./scripts/generate_graph.sh ./debug/* > /dev/null 2>&1
#     GRAPH_RESULT=$?
    
#     if [ $GRAPH_RESULT -ne 0 ]; then
#         echo "Warning: Graph generation failed with exit code $GRAPH_RESULT"
#     else
#         echo "Graph generation completed successfully"
#     fi
# else
#     if [ $TEST_RESULT -ne 0 ]; then
#         echo "Tests failed with exit code $TEST_RESULT. Skipping graph generation."
#     elif [ ! "$(ls -A ./debug 2>/dev/null)" ]; then
#         echo "No debug files found. Skipping graph generation."
#     fi
# fi

# Exit with the test program's exit code
exit $TEST_RESULT