#!/bin/bash

#
# Determines the top-tree directory (ROOT_DIRECTORY)
#
__SETUP_SCRIPT_FULL_PATH=$(realpath $0)
__SETUP_SCRIPT_DIRECTORY=$(dirname $__SETUP_SCRIPT_FULL_PATH)
ROOT_DIRECTORY=$(realpath ${__SETUP_SCRIPT_DIRECTORY})

#
# The default build type in use is
#
BUILD_TYPE=Debug
ENABLE_TESTING_PARAMETERS="-DENABLE_TESTING=OFF"
ENABLE_COVERAGE_PARAMETERS="-DENABLE_COVERAGE=OFF"

#
# Parses arguments
#
print_usage() {

    echo "Usage:"
    echo
    echo "$0 [--help][--build-type=<BUILD_TYPE>][--enable-testing]"
    echo
    echo "Options:"
    echo "      --help                    Displays this help"
    echo "      --build-type              The build type to pass to cmake"
    echo
    exit 1
}


while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            print_usage
            ;;
        --build-type)
            shift 1
            BUILD_TYPE=$1
            shift 1
            ;;
        --enable-testing)
            ENABLE_TESTING_PARAMETERS="-DENABLE_TESTING=ON"
            shift 1
            ;;
        --enable-coverage)
            ENABLE_COVERAGE_PARAMETERS="-DENABLE_COVERAGE=ON"
            shift 1
            ;;
        *)
            echo "Unknown option $1"
            exit 1
            ;;
    esac
done

if [[ "$BUILD_TYPE" != "Debug" &&  "$BUILD_TYPE" != "Release" ]]; then
    echo "FATAL: Build type must be either Debug or Release"
    exit 1
fi

#
# The build directory
#
BUILD_DIRECTORY=$ROOT_DIRECTORY/build
mkdir -p $BUILD_DIRECTORY

#
# Starts cmake
#
cd $BUILD_DIRECTORY && cmake $ROOT_DIRECTORY -DCMAKE_BUILD_TYPE=$BUILD_TYPE $ENABLE_TESTING_PARAMETERS $ENABLE_COVERAGE_PARAMETERS