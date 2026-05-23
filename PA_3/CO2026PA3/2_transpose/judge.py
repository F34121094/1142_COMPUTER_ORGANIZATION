#!/usr/bin/env python3
import glob
import os
import re
import sys
import subprocess
from pathlib import Path
import checker

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
RESET = "\033[0m"

# adjust these bounds to change grading
UPPER_MISS = {32: 500, 64: 1700}
LOWER_MISS = {32: 256, 64: 1024}
CASE_SCORE = 25

# mapping #misses with our grading criterion
def miss_score(n: int, misses: int, alpha: float = 0.5) -> float:
    low = LOWER_MISS[n]
    up = UPPER_MISS[n]

    if misses <= low:
        return CASE_SCORE
    if misses >= up:
        return 0.0

    # Normalize to [0, 1]
    x = (up - misses) / (up - low)

    # scoring curve
    return round(CASE_SCORE * (x ** alpha), 2)

# read stats from temporary file ".csim_results"
def read_misses(result_path: str) -> int:
    try:
        with open(result_path, "r", encoding="utf-8") as fp:
            content = fp.read().strip()
    except OSError as e:
        print(f"{RED}[ERROR]{RESET} failed to read {result_path}: {e}", file=sys.stderr)
        return -1

    parts = content.split()
    if len(parts) < 3:
        print(f"{RED}[ERROR]{RESET} invalid format in {result_path}: {content}", file=sys.stderr)
        return -1

    try:
        hits = int(parts[0])
        misses = int(parts[1])
        evicts = int(parts[2])
    except ValueError:
        print(f"{RED}[ERROR]{RESET} non-integer content in {result_path}: {content}", file=sys.stderr)
        return -1

    return misses

# return: status, score
def run_one(input_path: str) -> tuple[int, int]:
    filename = os.path.basename(input_path)

    pattern = r"^([1-9]\d*)_(0|1)\.in$"
    match = re.match(pattern, filename)
    if match is None:
        print(f"{RED}[ERROR]{RESET} Invalid input filename format: {filename}", file=sys.stderr)
        return 2, 0

    n = match.group(1)
    tc = match.group(2)

    os.makedirs("output", exist_ok=True)
    os.makedirs("answer", exist_ok=True)
    
    # simulate naive transpose
    answer_path = os.path.join("answer", f"{n}_{tc}.ans")
    result = subprocess.run(
        ["./evaluate", "-N", n, "-n", "-c", tc],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if result.returncode != 0:
        print(f"{RED}[RE]{RESET} {input_path}, status={result.returncode}", file=sys.stderr)
        if result.stderr:
            print(result.stderr, file=sys.stderr, end="")
        return 2, 0
    naive_misses = read_misses(".csim_results")
    if naive_misses < 0:
        return 2, 0

    # simulate optimized transpose
    output_path = os.path.join("output", f"{n}_{tc}.out")
    result = subprocess.run(
        ["./evaluate", "-N", n, "-c", tc],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if result.returncode != 0:
        print(f"{RED}[RE]{RESET} {input_path}, status={result.returncode}", file=sys.stderr)
        if result.stderr:
            print(result.stderr, file=sys.stderr, end="")
        return 2, 0
    misses = read_misses(".csim_results")
    if misses < 0:
        return 2, 0

    # correctness check
    diff_result = subprocess.run(
        ["git", "diff", "--no-index", "--quiet", "--", answer_path, output_path]
    )
    if diff_result.returncode == 0:
        print(f"{GREEN}[PASS]{RESET} {input_path}, ")
        score = miss_score(int(n), misses)
        print(f" [NAIVE] misses={naive_misses}, ", end="")
        print(f"{CYAN}[IMPROVED]{RESET} misses={misses}, score={score}")
        return 0, score
    elif diff_result.returncode == 1:
        print(f"{RED}[WA]{RESET} {input_path}")
        print(f'Run "git diff --no-index -- {answer_path} {output_path}" to check.')
        return 1, 0
    else:
        print(f"{RED}[ERROR]{RESET} git diff failed for {input_path}", file=sys.stderr)
        return 2, 0

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
        print(f"Example: {sys.argv[0]} 'input/32.in'", file=sys.stderr)
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

    # ===== check snippet stage =====
    snippet = "snippet.c"
    try:
        with open(snippet, "r", encoding="utf-8") as f:
            code = f.read()
    except Exception as e:
        print(f"{RED}[CE]{RESET} {e}")
        errors += 1
        print_summary(passed, wrong, errors, total_score_get)
        return 0

    matches = checker.find_blocked_types(code)
    if matches:
        print(f"{RED}[WA]{RESET} Blocked data type keyword(s) found:")
        for keyword, pos in matches:
            print(f"  {keyword} at offset {pos}")
        wrong += 1
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
        ret, score = run_one(input_path)
        if ret == 0:
            passed += 1
            total_score_get += score
        elif ret == 1:
            wrong += 1
        else:
            errors += 1

    print_summary(passed, wrong, errors, round(total_score_get, 2))


if __name__ == "__main__":
    main()