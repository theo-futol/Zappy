BINARY_PATH="${PWD}/../../zappy_server"

COMMANDS_TO_RUN=(
    # empty parameters
    "$BINARY_PATH"
    # missing parameters
    "$BINARY_PATH -p 1234 -x -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 -f"
    # invalid parameters
    "$BINARY_PATH -p abc -x 10 -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x abc -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y abc -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c abc -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 -f abc"
    # extra parameters
    "$BINARY_PATH -p 1234 extra_param -x 10 -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 extra_param -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 extra_param -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 extra_param -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 -f 100 extra_param"
    # empty field values
    "$BINARY_PATH -p  -x 10 -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p '' -x 10 -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x  -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x '' -y 10 -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y  -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y '' -n team1 team2 -c 5 -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n  -c 5 -f 100"
    # "$BINARY_PATH -p 1234 -x 10 -y 10 -n '' -c 5 -f 100" TO DO FIX
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c  -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c '' -f 100"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 -f"
    "$BINARY_PATH -p 1234 -x 10 -y 10 -n team1 team2 -c 5 -f ''"
)
PASSED_TESTS=0
NUMBER_OF_TESTS=${#COMMANDS_TO_RUN[@]}

echo "Running $NUMBER_OF_TESTS tests for zappy_server with invalid parameters (all commands should fail and exit with code 84)..."
echo
for command in "${COMMANDS_TO_RUN[@]}"; do
    echo "Running command: $command"
    EXIT_CODE=$($command >/dev/null 2>&1; echo $?)
    if [ $EXIT_CODE -eq 84 ]; then
        echo "Test passed."
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo "Test failed with exit code $EXIT_CODE."
    fi
done
echo    
echo "Passed $PASSED_TESTS out of $NUMBER_OF_TESTS tests."

exit $((NUMBER_OF_TESTS - PASSED_TESTS))
