#!/bin/bash

PASS=0
FAIL=0

run_test() {
    local desc="$1"
    local input="$2"
    local expected="$3"
    
    result=$(echo "$input" | ../../hw5-sim ../../matrix_multiplication.tko 2>/dev/null)
    
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

gcc -o ../../hw5-asm ../../hw5-asm.c -w && ../../hw5-asm ../../matrix_multiplication.tk ../../matrix_multiplication.tko
if [ $? -ne 0 ]; then echo "it failed bruh"; exit 1; fi

echo "matrix tests eeeeeee
" 

run_test "1x1: 5.0 * 6.0 = 30.0" "$(printf '1\n4617315517961601024\n4618441417868443648')" "4629137466983448576"

run_test "1x1: 7.0 * 8.0 = 56.0" "$(printf '1\n4619567317775286272\n4620693217682128896')" "4633078116657397760"

run_test "2x2 [[5,6],[7,8]] * [[5,6],[7,8]]" \
"$(printf '2\n4617315517961601024\n4618441417868443648\n4619567317775286272\n4620693217682128896\n4617315517961601024\n4618441417868443648\n4619567317775286272\n4620693217682128896')" \
"$(printf '4634415122796773376\n4635189178982727680\n4636103972657037312\n4637159503819702272')"

run_test "2x2 zeros * [[5,6],[7,8]] = zeros" \
"$(printf '2\n0\n0\n0\n0\n4617315517961601024\n4618441417868443648\n4619567317775286272\n4620693217682128896')" \
"$(printf '0\n0\n0\n0')"

echo ""
echo "Results: $PASS passed, $FAIL failed"