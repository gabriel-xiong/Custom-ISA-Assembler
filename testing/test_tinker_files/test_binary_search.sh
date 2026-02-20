#!/bin/bash

PASS=0
FAIL=0

run_test() {
    local desc="$1"
    local input="$2"
    local expected="$3"
    
    result=$(echo "$input" | ../../hw5-sim ../../binary_search.tko 2>/dev/null)
    
    if [ "$result" = "$expected" ]; then
        echo "PASS: $desc"
        PASS=$((PASS+1))
    else
        echo "FAIL: $desc"
        echo "  Expected: $expected"
        echo "  Got:      $result"
        FAIL=$((FAIL+1))
    fi
}

gcc -o ../../hw5-asm ../../hw5-asm.c -w && ../../hw5-asm ../../binary_search.tk ../../binary_search.tko
if [ $? -ne 0 ]; then echo "Assembly failed!"; exit 1; fi

echo "Running binary tests eeee
" 

echo "Findable tests: "
run_test "Found 1" "$(printf '5\n100\n105\n110\n115\n120\n100')" "found"
run_test "Found 2" "$(printf '5\n100\n105\n110\n115\n120\n105')" "found"
run_test "Found 3" "$(printf '5\n100\n105\n110\n115\n120\n110')" "found"
run_test "Found 4" "$(printf '5\n100\n105\n110\n115\n120\n115')" "found"
run_test "Found 5" "$(printf '5\n100\n105\n110\n115\n120\n120')" "found"
run_test "Found 6" "$(printf '1\n100\n100')" "found"

echo "Unfindable tests: "

run_test "Not found 1" "$(printf '5\n100\n105\n110\n115\n120\n99')" "not found"
run_test "Not found 2" "$(printf '1\n100\n105')" "not found"
run_test "Not found 3" "$(printf '5\n100\n105\n110\n115\n120\n95')" "not found"
run_test "Not found 4" "$(printf '5\n100\n105\n110\n115\n120\n125')" "not found"


echo ""
echo "Results: $PASS passed, $FAIL failed"