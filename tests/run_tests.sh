#!/usr/bin/env bash
# tests/run_tests.sh — Mini Compiler test suite
# Run from project root: bash tests/run_tests.sh

set -euo pipefail

COMPILER="./compiler"
PASS=0
FAIL=0
TOTAL=0

RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
BOLD='\033[1m'
RESET='\033[0m'

run_test() {
    local name="$1"
    local cmd="$2"
    local expect_success="${3:-true}"

    TOTAL=$((TOTAL + 1))
    echo -n "  [${TOTAL}] ${name} ... "

    if eval "$cmd" > /tmp/compiler_test_out.txt 2>&1; then
        if [ "$expect_success" = "true" ]; then
            echo -e "${GREEN}PASS${RESET}"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}FAIL (expected failure, got success)${RESET}"
            FAIL=$((FAIL + 1))
        fi
    else
        if [ "$expect_success" = "false" ]; then
            echo -e "${GREEN}PASS (correctly rejected)${RESET}"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}FAIL${RESET}"
            cat /tmp/compiler_test_out.txt | sed 's/^/      /'
            FAIL=$((FAIL + 1))
        fi
    fi
}

echo ""
echo -e "${CYAN}${BOLD}══════ Mini Compiler Test Suite ══════${RESET}"
echo ""

# ── Build first ──────────────────────────────────────────────
echo -e "${BOLD}Building...${RESET}"
make -s all 2>&1
echo -e "${GREEN}  ✔ Build OK${RESET}"
echo ""

# ── Positive tests ───────────────────────────────────────────
echo -e "${BOLD}Positive Tests (should compile successfully)${RESET}"
run_test "hello.toy compiles"             "$COMPILER examples/hello.toy"
run_test "arithmetic.toy compiles"        "$COMPILER examples/arithmetic.toy"
run_test "floats.toy compiles"            "$COMPILER examples/floats.toy"
run_test "--tokens flag"                  "$COMPILER examples/hello.toy --tokens"
run_test "--ast flag"                     "$COMPILER examples/hello.toy --ast"
run_test "--ir flag"                      "$COMPILER examples/hello.toy --ir"
run_test "--asm flag"                     "$COMPILER examples/hello.toy --asm"
run_test "--symbols flag"                 "$COMPILER examples/hello.toy --symbols"
run_test "--ir --ast combined"            "$COMPILER examples/hello.toy --ir --ast"
run_test "link two files"                 "$COMPILER examples/main.toy examples/math.toy --link"
run_test "link three files"               "$COMPILER examples/main.toy examples/math.toy examples/utils.toy --link"
run_test "--no-opt flag disables fold"    "$COMPILER examples/arithmetic.toy --ir --no-opt"
run_test "output files written to disk"   "test -f output/hello.ir"

echo ""

# ── Negative tests ───────────────────────────────────────────
echo -e "${BOLD}Negative Tests (should be rejected)${RESET}"
run_test "no args → usage error"          "$COMPILER" false
run_test "missing file → error"           "$COMPILER nonexistent.toy" false
run_test "errors.toy → semantic errors"   "$COMPILER examples/errors.toy" false

echo ""

# ── Summary ──────────────────────────────────────────────────
echo -e "${CYAN}${BOLD}══════ Results ══════${RESET}"
echo -e "  Total : ${BOLD}${TOTAL}${RESET}"
echo -e "  Pass  : ${GREEN}${BOLD}${PASS}${RESET}"
echo -e "  Fail  : ${RED}${BOLD}${FAIL}${RESET}"
echo ""

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}${BOLD}  All tests passed!${RESET}"
    exit 0
else
    echo -e "${RED}${BOLD}  ${FAIL} test(s) failed.${RESET}"
    exit 1
fi
