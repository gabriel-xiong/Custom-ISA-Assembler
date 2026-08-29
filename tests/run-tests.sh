#!/usr/bin/env sh
set -eu

temp_dir="$(mktemp -d 2>/dev/null || mktemp -d -t tinker-vm-tests)"
trap 'rm -rf "$temp_dir"' EXIT INT TERM

fibonacci_object="$temp_dir/fibonacci.tko"
binary_search_object="$temp_dir/binary-search.tko"
matrix_object="$temp_dir/matrix-multiplication.tko"

./tinker-asm examples/fibonacci.tk "$fibonacci_object"
./tinker-asm examples/binary-search.tk "$binary_search_object"
./tinker-asm examples/matrix-multiplication.tk "$matrix_object"

check_fibonacci() {
    input="$1"
    expected="$2"
    actual="$(printf '%s\n' "$input" | ./tinker-sim "$fibonacci_object")"

    if [ "$actual" != "$expected" ]; then
        printf 'FAIL: input %s produced %s; expected %s\n' "$input" "$actual" "$expected" >&2
        exit 1
    fi
}

check_fibonacci 1 0
check_fibonacci 2 1
check_fibonacci 5 2
check_fibonacci 10 21
check_fibonacci 15 233

actual="$(printf '5\n100\n105\n110\n115\n120\n110\n' | ./tinker-sim "$binary_search_object")"
if [ "$actual" != "found" ]; then
    printf 'FAIL: binary search expected found; got %s\n' "$actual" >&2
    exit 1
fi

actual="$(printf '5\n100\n105\n110\n115\n120\n99\n' | ./tinker-sim "$binary_search_object")"
if [ "$actual" != "not found" ]; then
    printf 'FAIL: binary search expected not found; got %s\n' "$actual" >&2
    exit 1
fi

actual="$(printf '1\n4617315517961601024\n4618441417868443648\n' | ./tinker-sim "$matrix_object")"
if [ "$actual" != "4629137466983448576" ]; then
    printf 'FAIL: 1x1 matrix product produced %s\n' "$actual" >&2
    exit 1
fi

if ./tinker-asm >/dev/null 2>&1; then
    printf 'FAIL: assembler accepted invalid CLI usage\n' >&2
    exit 1
fi

printf 'All Tinker VM tests passed.\n'
