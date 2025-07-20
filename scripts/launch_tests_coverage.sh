#!/bin/bash

# Path to the test program and compiler
TEST_PROG="../../writing-a-c-compiler-tests/test_compiler"
COMPILER="./compiler/cobaltc-compiler"

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

# Exit with the test program's exit code
exit $TEST_RESULT