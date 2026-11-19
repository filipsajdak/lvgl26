#!/usr/bin/env python3
"""Throwaway: which compile-cost measures work on a GitHub runner. Prints markdown."""
import json, os, re, shlex, shutil, statistics, subprocess, sys, tempfile, time
sys.path.insert(0, "tools")
import compile_report as cr

def sh(cmd, **kw):
    kw.setdefault("capture_output", True); kw.setdefault("text", True)
    return subprocess.run(cmd, **kw)

def cc1_for(build, name, root):
    entries = cr.load_entries(build)
    entry = cr.select(entries, name)
    mode, command = cr.prepared(entry)
    return mode, command, entry["directory"]

def stats(vals):
    vals = [v for v in vals if v is not None]
    if not vals: return "n/a"
    m = statistics.median(vals)
    spread = (max(vals) - min(vals)) / m * 100 if m else 0
    return f"median {m:,.0f}, min {min(vals):,.0f}, max {max(vals):,.0f}, range {spread:.2f}%"

def perf_runs(command, cwd, n):
    out = []
    for _ in range(n):
        r = sh(["perf", "stat", "-e", "instructions:u", "-x,", "--"] + command, cwd=cwd)
        m = re.search(r"^(\d+|<not counted>|<not supported>),,instructions", r.stderr, re.M)
        out.append(int(m.group(1)) if m and m.group(1).isdigit() else None)
        if out[-1] is None:
            return out, r.stderr[-400:]
    return out, ""

def wall_runs(command, cwd, n):
    out = []
    for _ in range(n):
        t = time.perf_counter(); sh(command, cwd=cwd); out.append((time.perf_counter() - t) * 1000)
    return out

def timed(label, fn):
    t = time.perf_counter()
    try:
        res = fn()
    except Exception as e:
        res = f"failed: {e}"
    return res, time.perf_counter() - t

if __name__ == "__main__":
    build, name, repeats = sys.argv[1], sys.argv[2], int(sys.argv[3])
    mode, command, cwd = cc1_for(build, name, ".")
    print(f"## {name} ({mode})\n")
    base_wall = wall_runs(command, cwd, 1)[0] / 1000
    print(f"native one run: {base_wall:.1f} s\n")
    rows = []
    # hardware counter
    if shutil.which("perf"):
        vals, why = perf_runs(command, cwd, repeats)
        rows.append(("perf stat instructions:u", stats(vals) if None not in vals else f"not available: {why.strip()[:200]}", "1x"))
    else:
        rows.append(("perf stat instructions:u", "perf not installed", "-"))
    w = wall_runs(command, cwd, repeats)
    rows.append((f"wall time, ms ({repeats} runs)", stats(w), "1x"))
    # valgrind
    if shutil.which("valgrind"):
        def vg():
            r = sh(["valgrind", "--tool=callgrind", "--callgrind-out-file=/dev/null"] + command, cwd=cwd)
            m = re.search(r"Collected\s*:\s*(\d+)", r.stderr)
            return int(m.group(1)) if m else r.stderr[-300:]
        res, t = timed("vg", vg)
        rows.append(("valgrind callgrind total Ir (1 run)", res if isinstance(res, str) else f"{res:,}", f"{t/base_wall:.0f}x ({t:.0f} s)"))
    # qemu user mode plugin
    q = shutil.which("qemu-x86_64")
    if q:
        plug = [p for p in subprocess.run("find / -name 'libinsn*.so' 2>/dev/null", shell=True, capture_output=True, text=True).stdout.split()]
        if plug:
            def qm():
                r = sh([q, "-plugin", plug[0], "-d", "plugin", "-L", "/"] + command, cwd=cwd)
                return (r.stderr + r.stdout)[-300:]
            res, t = timed("qemu", qm)
            rows.append(("qemu-user plugin libinsn", res.strip().replace("\n", " "), f"{t/base_wall:.0f}x"))
        else:
            rows.append(("qemu-user plugin libinsn", "qemu-user has no libinsn plugin in this package", "-"))
    else:
        rows.append(("qemu-user plugin libinsn", "qemu-x86_64 not installed", "-"))
    # dynamorio
    dr = os.environ.get("DRRUN")
    if dr and os.path.exists(dr):
        def drm():
            r = sh([dr, "-t", "inscount", "--"] + command, cwd=cwd)
            return (r.stderr + r.stdout)[-300:]
        res, t = timed("dr", drm)
        rows.append(("DynamoRIO inscount (1 run)", res.strip().replace("\n", " "), f"{t/base_wall:.0f}x ({t:.0f} s)"))
    print("| measure | result | slowdown |\n|---|---|---|")
    for r in rows: print("| " + " | ".join(str(c).replace("|", "/") for c in r) + " |")
    # gcc counters
    print("\n### GCC's own counters (deterministic)\n")
    def run(extra):
        return sh(command + extra, cwd=cwd).stderr
    mem = run(["-fmem-report"])
    tot = re.findall(r"^(Total\S*(?: Allocated)?)\s+(\d+\S*)", mem, re.M)
    print("-fmem-report:", "; ".join(f"{a} {b}" for a, b in tot[:6]) or mem[-300:].replace("\n", " | "))
    st = run(["-fstats"])
    print("\n-fstats lines:", len(st.splitlines()), "; sample:", " | ".join(st.splitlines()[:6]))
    pre = sh([c for c in command if c not in ("-fpreprocessed",)] + ["-E"], cwd=cwd)
    print("\n-E is not usable with a cc1plus command:", pre.returncode)
