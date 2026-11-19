#!/usr/bin/env python3
"""Fails when a function of a -fstack-usage report needs more stack than its budget.

    python3 tools/stack_budget.py build/arm-m4/examples/mcu/CMakeFiles/set_stack_probe.dir/set_probe.cpp.su

The budget of a run-time view.set({...}) on a Cortex-M4 is 4 KB, decided by the owner on
2026-11-19 (docs/research/memory.md, "The stack of a run-time set"). Uses the standard library only.
"""

import pathlib
import re
import sys

# Stack that one run-time set may take on the Cortex-M4.
SET_STACK_BUDGET = 4096
# The functions of examples/mcu/set_probe.cpp, by the name -fstack-usage gives them.
PROBED = ("set_value",)


def frames(report: str) -> dict[str, int]:
    """The stack of each function of a -fstack-usage report: `file:line:col:name<TAB>bytes<TAB>kind`."""
    out = {}
    for line in report.splitlines():
        parts = line.split("\t")
        if len(parts) < 2 or not parts[1].isdigit():
            continue
        out[parts[0]] = int(parts[1])
    return out


def function_of(name: str) -> str:
    """The function of a report line, without its file, line and column."""
    return re.sub(r"^.*?:\d+:\d+:", "", name)


def probed(name: str) -> bool:
    """Whether a report line names a function of the probe: `... void set_value(...)`."""
    return any(f" {p}(" in name for p in PROBED)


def over_budget(report: str, budget: int = SET_STACK_BUDGET) -> list[str]:
    """The probed functions above the budget, and an error when one is missing."""
    found = {name: size for name, size in frames(report).items() if probed(name)}
    problems = [f"{name}: {size} bytes of stack, the budget is {budget}" for name, size in found.items() if size > budget]
    if not found:
        problems.append("the report names none of " + ", ".join(PROBED))
    return problems


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(__doc__.strip().splitlines()[0], file=sys.stderr)
        return 2
    report = pathlib.Path(argv[0]).read_text()
    problems = over_budget(report)
    for p in problems:
        print(f"stack_budget: {p}", file=sys.stderr)
    for name, size in frames(report).items():
        if probed(name):
            print(f"{function_of(name)}: {size} bytes (budget {SET_STACK_BUDGET})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
