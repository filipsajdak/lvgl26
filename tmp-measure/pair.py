#!/usr/bin/env python3
"""Throwaway: the same counters before and after a commit. Prints one markdown row per commit pair."""
import os, re, shlex, subprocess, sys, tempfile
sys.path.insert(0, "tools")
import compile_report as cr
import measure as m

def go(root, build, name, tools):
    mode, command, cwd = m.cc1_for(build, name, root)
    command = [c.replace(sys.argv[1], root) for c in command]
    cwd = root
    res = {}
    if "perf" in tools:
        v, why = m.perf_runs(command, cwd, 3)
        res["perf"] = v[len(v)//2] if None not in v else None
    mem = m.sh(command + ["-fmem-report"], cwd=cwd).stderr
    t = re.search(r"^Total\s+(\d+)", mem, re.M)
    res["mem_total"] = int(t.group(1)) if t else None
    if "valgrind" in tools:
        r = m.sh(["valgrind", "--tool=callgrind", "--callgrind-out-file=/dev/null"] + command, cwd=cwd)
        k = re.search(r"Collected\s*:\s*(\d+)", r.stderr)
        res["callgrind"] = int(k.group(1)) if k else None
    res["wall_ms"] = round(m.wall_runs(command, cwd, 3)[1])
    ops = cr.ops_range(command, cwd)
    res["ops"] = ops
    return res

if __name__ == "__main__":
    repo, build, name, tools = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4].split(",")
    for label, root in [("before", sys.argv[5]), ("after", sys.argv[6])]:
        print(label, name, go(root, build, name, tools), flush=True)
