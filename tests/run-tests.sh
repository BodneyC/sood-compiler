#!/usr/bin/env bash
#
# Runs each `tests/*.sood` file containing expectations, these are comments:
#
#   #> <line>   An expected line of output (stdout and stderr)
#   #< <line>   A line of input given on stdin
#   #! <text>   The compiler is expected to fail with an error containing <text>
#
# Each program is run both in the compiler's JIT (`-R`) and as a native binary.
#
# Usage: tests/run-tests.sh [path/to/sood]

set -uo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOOD="${1:-$TESTS_DIR/../build/src/sood}"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

export SPDLOG_LEVEL=error

pass=0
fail=0

expectation() { # <prefix> <file>
  sed -n "s/^#$1 \{0,1\}//p" "$2"
}

check() { # <name> <mode> <expected> <actual>
  if [[ "$3" == "$4" ]]; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    echo "FAIL: $1 ($2)"
    diff <(echo "$3") <(echo "$4") | sed 's/^/  /'
  fi
}

for test in "$TESTS_DIR"/*.sood; do
  name="$(basename "$test" .sood)"
  grep -qE '^#[<>!]' "$test" || continue

  input="$(expectation '<' "$test")"
  error="$(expectation '!' "$test")"

  if [[ -n "$error" ]]; then
    actual="$("$SOOD" -C -o "$TMP_DIR/$name.ll" "$test" 2>&1)"
    if [[ $? -ne 0 && "$actual" == *"$error"* ]]; then
      pass=$((pass + 1))
    else
      fail=$((fail + 1))
      echo "FAIL: $name (expected error containing '$error')"
      echo "$actual" | sed 's/^/  /'
    fi
    continue
  fi

  expected="$(expectation '>' "$test")"

  actual="$(echo "$input" | "$SOOD" -R "$test" 2>&1)"
  check "$name" jit "$expected" "$actual"

  if "$SOOD" -p 2 -o "$TMP_DIR/$name" "$test"; then
    actual="$(echo "$input" | "$TMP_DIR/$name" 2>&1)"
    check "$name" native "$expected" "$actual"
  else
    check "$name" native "$expected" "<compilation failed>"
  fi
done

echo "$pass passed, $fail failed"
[[ $fail -eq 0 ]]
