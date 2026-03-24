#!/bin/bash
# Test script to verify the simulator works independently

PIPE_DIR=$(mktemp -d /tmp/simtest_XXXXXX)
CMD_PIPE="$PIPE_DIR/cmd.pipe"
DATA_PIPE="$PIPE_DIR/data.pipe"

mkfifo "$CMD_PIPE" "$DATA_PIPE"

echo "=== Starting simulator ==="
./sim --cmd-pipe "$CMD_PIPE" --data-pipe "$DATA_PIPE" Worlds/tiny.world Bugs/simple.bug Bugs/simple.bug &
SIM_PID=$!
sleep 2

echo "=== Checking if sim is alive ==="
if kill -0 $SIM_PID 2>/dev/null; then
    echo "Simulator is running (pid=$SIM_PID)"
else
    echo "ERROR: Simulator died!"
    rm -rf "$PIPE_DIR"
    exit 1
fi

echo "=== Reading data pipe (5 sec timeout) ==="
timeout 5 cat "$DATA_PIPE" &
READ_PID=$!

sleep 1

echo "=== Sending STEP 1 ==="
echo "STEP 1" > "$CMD_PIPE"

# Wait for reader to finish
wait $READ_PID 2>/dev/null
echo ""
echo "=== Done ==="

kill $SIM_PID 2>/dev/null
rm -rf "$PIPE_DIR"
