#!/usr/bin/env python3
"""Throwaway: the same counters before and after a commit.
   pair.py <repo root> <build> <probe file in tests/> <file to compile> <worktree before> <worktree after>"""
import os, re, sys
sys.path.insert(0, "tools"); sys.path.insert(0, "tmp-measure")
import compile_report as cr
import measure as m

def with_root(command, repo, root, target):
    out = []
    for c in command:
        if c.endswith("/" + sys.argv[3]): c = target
        else: c = c.replace(f"{repo}/include", f"{root}/include")
        out.append(c)
    return out

def go(root, repo, build, probe, target):
    mode, command, cwd = m.cc1_for(build, "tests/" + probe, repo)
    command = with_root(command, repo, root, target)
    res = {}
    mem = m.sh(command + ["-fmem-report"], cwd=cwd).stderr
    t = re.search(r"^Total\S*\s+(\d+\S*)", mem, re.M)
    res["mem_total"] = t.group(1) if t else None
    r = m.sh(["valgrind", "--tool=callgrind", "--callgrind-out-file=/dev/null"] + command, cwd=cwd)
    k = re.search(r"Collected\s*:\s*(\d+)", r.stderr)
    res["callgrind_Ir"] = int(k.group(1)) if k else r.stderr[-200:]
    w = sorted(m.wall_runs(command, cwd, 3))
    res["wall_ms_min_med_max"] = [round(w[0]), round(w[2]), round(w[-1])]
    res["includes_from"] = sorted({c for c in command if "include" in c and root in c})[:2]
    return res

if __name__ == "__main__":
    repo, build, probe, target, before, after = sys.argv[1:7]
    for label, root in (("before", before), ("after", after)):
        print(f"{label}: {go(root, repo, build, probe, target)}", flush=True)
