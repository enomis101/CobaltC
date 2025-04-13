#!/bin/bash

# Path to the test program and compiler
TEST_PROG="../writing-a-c-compiler-tests/test_compiler"
COMPILER="./build/compiler/cobaltc-compiler"

# Run the test program with the compiler and forward all arguments
$TEST_PROG $COMPILER "$@"