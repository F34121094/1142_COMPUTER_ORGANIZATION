#!/usr/bin/env python3

import os
import re
import sys
import subprocess
from helper import load_f32


NUM_TEST = 300
OUTPUT_DIM = 10
OUTPUT_SIZE = NUM_TEST * OUTPUT_DIM

ATOL = 1e-3
RTOL = 1e-3

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
RESET = "\033[0m"

CACHE_MISS_PENALTY = 100

# criterion
UPPER = 28
LOWER = 4
CASE_SCORE = {0: 10, 1: 90}

RISCV = os.environ.get("RISCV")
if not RISCV:
    print('Please set your RISCV variable before running judge.', file=sys.stderr)
    sys.exit(0)

# summary of this test
def print_summary(passed: int, wrong: int, errors: int, score: int):
    print(f"\n{YELLOW}===== Summary ====={RESET}")
    print(f"PASS   : {passed}")
    print(f"WA     : {wrong}")
    print(f"ERR    : {errors}")
    print(f"{GREEN}SCORE{RESET}  : {score}")

class Testbench:
    def __init__(self, testbench: str, lib: str, execfile: str, outputfile: str, datafile: str, weightfile: str):
        self.testbench = testbench
        self.lib = lib
        self.execfile = execfile
        self.outputfile = outputfile
        self.datafile = datafile
        self.weightfile = weightfile

        self.r_access = 0
        self.w_access = 0
        self.r_miss = 0
        self.w_miss = 0
        self.inst_cycles = 0
        self.mem_cycles = 0

        self.exec_stdout = ""
        self.exec_stderr = ""

    def compile(self) -> bool:
        result = subprocess.run(
            [
                "riscv64-unknown-linux-gnu-gcc",
                "-march=rv64gcv",
                "-static",
                "-O1",
                self.testbench,
                self.lib,
                self.datafile,
                self.weightfile,
                "-o",
                self.execfile,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        if result.returncode != 0:
            print(f"{RED}[CE]{RESET} See message below")
            print(result.stderr)
            return False

        return True

    def run(self, sets: int=16, ways: int=4, bytes: int=64) -> bool:
        pk_path = os.path.join(RISCV, "riscv64-unknown-linux-gnu/bin", "pk")

        # First run: generate output file
        result_output = subprocess.run(
            [
                "spike",
                "--isa=RV64GCV_Zicntr",
                pk_path,
                self.execfile,
                self.outputfile,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        if result_output.returncode != 0:
            print(f"{RED}[RE]{RESET} runtime failed while generating output: {self.lib}, status={result_output.returncode}", file=sys.stderr)
            if result_output.stderr:
                print(result_output.stderr, file=sys.stderr, end="")
            return False

        # Second run: collect cache statistics
        result_stat = subprocess.run(
            [
                "spike",
                "--isa=RV64GCV_Zicntr",
                f"--dc={sets}:{ways}:{bytes}",
                pk_path,
                self.execfile,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        if result_stat.returncode != 0:
            print(f"{RED}[RE]{RESET} runtime failed while collecting stats: {self.lib}, status={result_stat.returncode}", file=sys.stderr)
            if result_stat.stderr:
                print(result_stat.stderr, file=sys.stderr, end="")
            return False

        self.exec_stdout = result_stat.stdout
        self.exec_stderr = result_stat.stderr

        return self._parse_stats()

    def _parse_stats(self) -> bool:
        for line in self.exec_stdout.splitlines():
            if re.search(r"Read Accesses", line):
                self.r_access = int(re.search(r"[0-9]+", line)[0])
            elif re.search(r"Write Accesses", line):
                self.w_access = int(re.search(r"[0-9]+", line)[0])
            elif re.search(r"Read Misses", line):
                self.r_miss = int(re.search(r"[0-9]+", line)[0])
            elif re.search(r"Write Misses", line):
                self.w_miss = int(re.search(r"[0-9]+", line)[0])
            elif re.search(r"cycles", line):
                self.inst_cycles = int(re.search(r"[0-9]+", line)[0])

        self.mem_cycles = (
            (self.r_access - self.r_miss)
            + (self.w_access - self.w_miss)
            + CACHE_MISS_PENALTY * self.r_miss
            + CACHE_MISS_PENALTY * self.w_miss
        )

        return True

    def print_report(self, title: str) -> None:
        print(f"{CYAN}===== {title} ====={RESET}")
        print(self.exec_stdout, end="" if self.exec_stdout.endswith("\n") else "\n")
        print(f"Memory subsystem access overhead = {self.mem_cycles} (cpu cycle)")
        print(f"Instruction cycles = {self.inst_cycles} (cpu cycle)")


def check_correctness(answer_path: str, output_path: str) -> bool:
    ref = load_f32(answer_path, OUTPUT_SIZE)
    out = load_f32(output_path, OUTPUT_SIZE)

    max_abs_err = 0.0
    first_bad = None

    for idx, (r, s) in enumerate(zip(ref, out)):
        abs_err = abs(s - r)

        if abs_err > max_abs_err:
            max_abs_err = abs_err

        if abs_err > ATOL + RTOL * abs(r):
            first_bad = (idx, r, s, abs_err)
            break

    if first_bad is None:
        print(f"Output Correctness: {GREEN}Pass{RESET}")
        print(f"max_abs_err = {max_abs_err:.3e}")
        return True

    idx, r, s, abs_err = first_bad
    row = idx // OUTPUT_DIM
    col = idx % OUTPUT_DIM
    print(f"Output Correctness: {RED}Fail{RESET}")
    print(f"first mismatch at sample={row}, class={col}")
    print(f"ref={r:.3f}, out={s:.3f}, abs_err={abs_err:.3e}")
    return False

# status, ratio
def run_one(i: int) -> tuple[int, int]:
    original = Testbench(
        testbench="mlp_inference.c",
        lib="matmul_naive.c",
        execfile="mlp_original",
        datafile=f"input/data{i}.c",
        weightfile=f"input/weights{i}.c",
        outputfile=f"answer/mlp{i}.ans",
    )

    improved = Testbench(
        testbench="mlp_inference.c",
        lib="matmul_improved.c",
        execfile="mlp_improved",
        datafile=f"input/data{i}.c",
        weightfile=f"input/weights{i}.c",
        outputfile=f"output/mlp{i}.out",
    )

    # ===== run orginal =====
    if not original.compile():
        return 2, 0
    if not original.run():
        return 2, 0
    original.print_report("Original version")
    print("-----------------------------------")

    # ===== run improved =====
    if not improved.compile():
        return 2, 0
    try:
        import dc_config
    except ImportError:
        dc_config = None
    SETS = getattr(dc_config, "SETS", 16)
    WAYS = getattr(dc_config, "WAYS", 4)
    BYTES = getattr(dc_config, "BYTES", 64)
    if SETS == 1 or SETS*WAYS*BYTES > 4096:
        print(f"{YELLOW}[WARN]{RESET} Invalid Configuration. Use Default Cache: --dc=16:4:64")
        if not improved.run():
            return 2, 0
    else:
        if not improved.run(sets=SETS, ways=WAYS, bytes=BYTES):
            return 2, 0
    improved.print_report("Improved version")
    print("-----------------------------------")

    # abs(student - ref) <= atol + rtol * abs(ref)
    correct = check_correctness(f"answer/mlp{i}.ans", f"output/mlp{i}.out")

    if correct:
        memory_ratio = original.mem_cycles / improved.mem_cycles
        print(
        "memory access overhead improved ratio: ",
        memory_ratio if improved.mem_cycles != 0 else "INF",
        )
        improved_ratio = ((original.mem_cycles + original.inst_cycles) / (improved.mem_cycles + improved.inst_cycles))
        print(
            "total overhead improved ratio: ",
            improved_ratio
            if (improved.mem_cycles + improved.inst_cycles) != 0
            else "INF",
        )
        return 0, improved_ratio
    return 1, 0

# grading
def ratio_score(tc: int, ratio: float, alpha: float = 0.5) -> float:
    
    if ratio >= UPPER:
        return CASE_SCORE[tc]
    if ratio <= LOWER:
        return 0
    
    x = (ratio - LOWER)/(UPPER - LOWER)

    return round(CASE_SCORE[tc]*(x**alpha), 2)

def main():
    os.makedirs("output", exist_ok=True)
    os.makedirs("answer", exist_ok=True)

    passed = 0
    wrong = 0
    errors = 0
    total_score_get = 0

    for i in [0]:
        ret, ratio = run_one(i)
        if ret == 0:
            passed += 1
            total_score_get += ratio_score(i, ratio)
        elif ret == 1:
            wrong += 1
        else:
            errors += 1
    print_summary(passed, wrong, errors, round(total_score_get, 2))


if __name__ == "__main__":
    main()