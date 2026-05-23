#!/usr/bin/env python3
import glob
import os
import re
import sys
import subprocess
from pathlib import Path

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
RESET = "\033[0m"

CASE_SCORE = 20

# return: status
def run_one(input_path: str) -> int:
    filename = os.path.basename(input_path)

    pattern = r"^([0-9]+)_S([0-9]+)_W([0-9]+)_B([0-9]+)\.in$"
    match = re.match(pattern, filename)
    if match is None:
        print(f"{RED}[ERROR]{RESET} Invalid input filename format: {filename}", file=sys.stderr)
        return 2

    tc_id, s, w, b = match.groups()

    os.makedirs("output", exist_ok=True)
    output_name = os.path.splitext(filename)[0]

    output_path = os.path.join("output", output_name + ".out")
    answer_path = os.path.join("answer", output_name + ".ans")

    # generate testcase's output
    with open(output_path, "w", encoding="utf-8") as out_fp:
        result = subprocess.run(
            ["./csim_cpp", "-v", "-S", s, "-W", w, "-B", b, "-t", input_path],
            stdout=out_fp,
            stderr=subprocess.PIPE,
            text=True
        )
    if result.returncode != 0:
        print(f"{RED}[RE]{RESET} {input_path}, status={result.returncode}", file=sys.stderr)
        if result.stderr:
            print(result.stderr, file=sys.stderr, end="")
        return 2

    # correctness check
    diff_result = subprocess.run(
        ["git", "diff", "--no-index", "--quiet", "--", answer_path, output_path]
    )
    if diff_result.returncode == 0:
        print(f"{GREEN}[PASS]{RESET} {input_path}")
        return 0
    elif diff_result.returncode == 1:
        print(f"{RED}[WA]{RESET} {input_path}")
        print(f"run \"git diff --no-index -- {answer_path} {output_path}\" to check.")
        return 1
    else:
        print(f"{RED}[ERROR]{RESET} git diff failed for {input_path}", file=sys.stderr)
        return 2

# build executables
def compile():
    result = subprocess.run(
        ["make", "all"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.returncode != 0:
        print(f"{RED}[CE]{RESET} See message below")
        print(result.stderr)
        return False

    return True       

# summary of this test
def print_summary(passed: int, wrong: int, errors: int, score: int):
    print(f"\n{YELLOW}===== Summary ====={RESET}")
    print(f"PASS   : {passed}")
    print(f"WA     : {wrong}")
    print(f"ERR    : {errors}")
    print(f"{GREEN}SCORE{RESET}  : {score}")

def main():

    passed = 0
    wrong = 0
    errors = 0
    total_score_get = 0

    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} '<glob_pattern>'", file=sys.stderr)
        print(f"Example: {sys.argv[0]} 'input/0_*.in'", file=sys.stderr)
        errors += 1
        print_summary(passed, wrong, errors, total_score_get)
        return 0

    pattern = sys.argv[1]
    inputs = sorted(glob.glob(pattern))

    # ===== testcase existence check =====
    if not inputs:
        print(f"No input files matched: {pattern}", file=sys.stderr)
        errors += 1
        print_summary(passed, wrong, errors, total_score_get)
        return 0

    # ===== compile stage =====
    compile_success = compile()
    if not compile_success:
        errors += 1
        print_summary(passed, wrong, errors, total_score_get)
        return 0

    # ===== run stage =====
    for input_path in inputs:
        ret = run_one(input_path)
        if ret == 0:
            passed += 1
            total_score_get += CASE_SCORE
        elif ret == 1:
            wrong += 1
        else:
            errors += 1

    print_summary(passed, wrong, errors, total_score_get)


if __name__ == "__main__":
    main()