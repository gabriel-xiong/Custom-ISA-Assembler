#!/bin/bash

PASS=0
FAIL=0

run_test() {
    local desc="$1"
    local input="$2"
    local expected="$3"
    
    result=$(echo "$input" | ../../hw5-sim ../../fibonacci.tko 2>/dev/null)
    
    if [ "$result" = "$expected" ]; then
        echo "PASS: $desc"
        PASS=$((PASS+1))
    else
        echo "FAIL: $desc"
        echo "  Expected: $expected"
        echo "  Actual:      $result"
        FAIL=$((FAIL+1))
    fi
}

gcc -o ../../hw5-asm ../../hw5-asm.c -w && ../../hw5-asm ../../fibonacci.tk ../../fibonacci.tko
if [ $? -ne 0 ]; then echo "Assembly failed!"; exit 1; fi

echo "Fibonacci tests eeeeeee:
"

run_test "fib(1) = 0" "1" "0"
run_test "fib(3) = 1" "3" "1"
run_test "fib(5) = 3" "5" "3"
run_test "fib(7) = 8" "7" "8"
run_test "fib(10) = 34" "10" "34"

echo ""
echo "Results: $PASS passed, $FAIL failed"