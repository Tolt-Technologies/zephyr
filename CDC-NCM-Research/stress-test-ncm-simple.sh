#!/bin/bash
#
# Simple CDC-NCM enumeration stress test
#

ITERATIONS=20
WAIT_SECONDS=10
IFACE="${IFACE:-en16}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

SUCCESSES=0
FAILURES=0

git_ref() {
    local dir="$1"
    local hash branch
    hash=$(git -C "$dir" rev-parse --short HEAD 2>/dev/null)
    branch=$(git -C "$dir" symbolic-ref --short HEAD 2>/dev/null)
    if [ -n "$branch" ]; then
        echo "$hash ($branch)"
    else
        echo "$hash"
    fi
}

echo "adt-25-firmware: $(git_ref "$SCRIPT_DIR/..")"
echo "zephyr: $(git_ref "$SCRIPT_DIR/../../zephyr")"
echo "Interface: $IFACE, Iterations: $ITERATIONS, Wait: ${WAIT_SECONDS}s"
echo ""
printf "Results: "

for i in $(seq 1 $ITERATIONS); do
    nrfutil device reset >/dev/null 2>&1
    sleep "$WAIT_SECONDS"

    if ifconfig "$IFACE" 2>/dev/null | grep -q "status: active"; then
        printf "S"
        SUCCESSES=$((SUCCESSES + 1))
    else
        printf "F"
        FAILURES=$((FAILURES + 1))
    fi
done

echo ""
echo "$SUCCESSES/$ITERATIONS success ($((SUCCESSES * 100 / ITERATIONS))%)"
