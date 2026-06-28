#!/bin/bash

set -euo pipefail

# Define test directories
UNIT_TEST_DIR="tests/server/unit"
STRESS_TEST_DIR="tests/server/stress"
FUNCTIONAL_TEST_DIR="tests/server/functional"

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <unit_test_script:0 or 1> <stress_test_script:0 or 1> <functional_test_script:0 or 1>"
    exit 1
fi

# Define which test to run based on the argument provided (0 or 1)
UNIT_TEST=$1
STRESS_TEST=$2
FUNCTIONAL_TEST=$3

# Variable to hold the server process ID
SERVER_PID=""

# Run unit tests if the argument is set to 1
if [ "$UNIT_TEST" -ne 0 ]; then

    # Compile unit tests
    echo "Compiling unit tests..."

    make -C "${UNIT_TEST_DIR}" || { echo "Compilation of unit tests failed."; exit 1; }

    # Run unit tests
    echo "Running unit tests..."
    if [ -f "${UNIT_TEST_DIR}/zappy_server_tests" ]; then
        make -C "${UNIT_TEST_DIR}" tests_run || { echo "Unit tests failed."; exit 1; }
    else
        echo "Unit test script zappy_server_tests not found in ${UNIT_TEST_DIR}."
        exit 1
    fi
    make -C "${UNIT_TEST_DIR}" fclean || { echo "Failed to clean unit test build files."; exit 1; }
    echo
    echo "Unit tests completed."
fi

echo

if [ "$STRESS_TEST" -ne 0 ] || [ "$FUNCTIONAL_TEST" -ne 0 ]; then
    echo "Compile the zappy_server binary for stress / functional tests..."
    echo
    make zappy_server || { echo "Compilation of zappy_server failed."; exit 1; }
    ./zappy_server -c 5 -n team1 team2 -p 4242 -x 10 -y 10 -f 100 &
    SERVER_PID=$!
    sleep 1
fi

echo

# Run stress tests if the argument is set to 1
if [ "$STRESS_TEST" -ne 0 ]; then
    for stress_test_script in "${STRESS_TEST_DIR}"/*.sh; do
        if [ -f "$stress_test_script" ]; then
            echo "Running stress test: $stress_test_script"
            ./"$stress_test_script"
        else
            echo "No stress test scripts found in ${STRESS_TEST_DIR}."
            exit 1
        fi
    done
fi

echo

# Run functional tests if the argument is set to 1
if [ "$FUNCTIONAL_TEST" -ne 0 ]; then
    for functional_test_script in "${FUNCTIONAL_TEST_DIR}"/*.sh; do
        if [ -f "$functional_test_script" ]; then
            echo "Running functional test: $functional_test_script"
            ./"$functional_test_script"
        else
            echo "No functional test scripts found in ${FUNCTIONAL_TEST_DIR}."
            exit 1
        fi
    done
fi

echo
echo "All selected tests completed successfully."

if [ -n "$SERVER_PID" ]; then
    echo
    echo "Terminating the server process with PID $SERVER_PID..."
    kill "$SERVER_PID" || { echo "Failed to terminate the server process."; exit 1; }
fi
