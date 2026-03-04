#!/bin/bash
# Run Unity tests in ESP32 QEMU emulator
#
# Usage: ./tools/run_qemu_tests.sh [timeout_seconds]
#
# Prerequisites:
#   - ESP-IDF environment sourced (. $IDF_PATH/export.sh)
#   - QEMU installed: python $IDF_PATH/tools/idf_tools.py install qemu-xtensa
#   - Test firmware built: make build-unity-tests-qemu
#
# Exit codes:
#   0 - All tests passed
#   1 - Test failures or build errors
#   124 - Timeout

set -e

TIMEOUT=${1:-120}  # Default 120 second timeout
BUILD_DIR="test/unity-hw-tests/build"
FLASH_IMAGE="$BUILD_DIR/flash_image.bin"
LOG_FILE="$BUILD_DIR/qemu_output.log"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

cd "$PROJECT_ROOT"

# Check prerequisites
if [ ! -f "$BUILD_DIR/unity_hw_tests.bin" ]; then
    echo "Error: Test firmware not built."
    echo "Run 'make build-unity-tests-qemu' first."
    exit 1
fi

if [ ! -f "$BUILD_DIR/bootloader/bootloader.bin" ]; then
    echo "Error: Bootloader not found in build directory."
    exit 1
fi

if [ ! -f "$BUILD_DIR/partition_table/partition-table.bin" ]; then
    echo "Error: Partition table not found in build directory."
    exit 1
fi

# Check if qemu-system-xtensa is available
if ! command -v qemu-system-xtensa &> /dev/null; then
    echo "Error: qemu-system-xtensa not found."
    echo "Install with: python \$IDF_PATH/tools/idf_tools.py install qemu-xtensa"
    echo "Then re-source: . \$IDF_PATH/export.sh"
    exit 1
fi

# Create merged flash image for QEMU
echo "=== Creating flash image for QEMU ==="
esptool.py --chip esp32 merge_bin \
    -o "$FLASH_IMAGE" \
    --flash_mode dio \
    --flash_freq 40m \
    --flash_size 4MB \
    0x1000 "$BUILD_DIR/bootloader/bootloader.bin" \
    0x8000 "$BUILD_DIR/partition_table/partition-table.bin" \
    0x10000 "$BUILD_DIR/unity_hw_tests.bin" \
    --fill-flash-size 4MB

echo "Flash image created: $FLASH_IMAGE"

# Clean previous log
rm -f "$LOG_FILE"

echo ""
echo "=== Running Unity tests in QEMU (timeout: ${TIMEOUT}s) ==="
echo "Waiting for test completion marker [QEMU_TEST_DONE]..."
echo ""

# Run QEMU with timeout, capture output
# Use timeout command to limit execution time
# -nographic: No graphical output
# -machine esp32: ESP32 target
# -drive: Flash image
# -serial mon:stdio: Route serial to stdout
timeout --preserve-status "$TIMEOUT" \
    qemu-system-xtensa \
    -nographic \
    -machine esp32 \
    -drive file="$FLASH_IMAGE",if=mtd,format=raw \
    -serial mon:stdio \
    2>&1 | tee "$LOG_FILE" &

QEMU_PID=$!

# Wait for test completion marker or timeout
RESULT=1
WAIT_COUNT=0
MAX_WAIT=$TIMEOUT

while [ $WAIT_COUNT -lt $MAX_WAIT ]; do
    # Check if QEMU is still running
    if ! kill -0 $QEMU_PID 2>/dev/null; then
        echo "QEMU process ended"
        break
    fi

    # Check for completion marker
    if grep -q "\[QEMU_TEST_DONE\]" "$LOG_FILE" 2>/dev/null; then
        echo ""
        echo "=== Test completion marker detected ==="
        kill $QEMU_PID 2>/dev/null || true
        RESULT=0
        break
    fi

    # Also check for Unity success pattern as backup
    if grep -q "Tests.*Failures.*Ignored" "$LOG_FILE" 2>/dev/null; then
        # Wait a bit more for full output
        sleep 2
        if grep -q "\[QEMU_TEST_DONE\]" "$LOG_FILE" 2>/dev/null; then
            kill $QEMU_PID 2>/dev/null || true
            RESULT=0
            break
        fi
    fi

    sleep 1
    WAIT_COUNT=$((WAIT_COUNT + 1))
done

# Clean up QEMU if still running
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

echo ""
echo "=== Test Results ==="

# Parse results from log
if [ -f "$LOG_FILE" ]; then
    # Extract Unity test summary
    if grep -E "^[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "$LOG_FILE"; then
        FAILURES=$(grep -oE "[0-9]+ Failures" "$LOG_FILE" | grep -oE "[0-9]+" | head -1)
        if [ "$FAILURES" != "0" ]; then
            echo ""
            echo "Test failures detected:"
            grep "FAIL" "$LOG_FILE" || true
            RESULT=1
        fi
    else
        echo "Warning: Could not find Unity test summary in output"
    fi

    # Check for crash/panic
    if grep -qi "panic" "$LOG_FILE" || grep -qi "guru meditation" "$LOG_FILE"; then
        echo ""
        echo "ERROR: Crash detected in QEMU output!"
        grep -i "panic\|guru meditation\|abort\|exception" "$LOG_FILE" || true
        RESULT=1
    fi
else
    echo "Error: No output log found"
    RESULT=1
fi

echo ""
if [ $RESULT -eq 0 ]; then
    echo "All tests passed!"
else
    echo "Tests failed or timed out"
fi

echo "Full log available at: $LOG_FILE"
exit $RESULT
