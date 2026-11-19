#!/usr/bin/env python3
"""Writes script.json of examples/wasm as the C++ include that host_main.cpp reads."""
import json, sys
s = json.load(open(sys.argv[1]))
print("constexpr std::uint32_t script_end = %d;" % s["end"])
print("constexpr std::uint32_t script_step = %d;" % s["step"])
cap = s.get("capture", [s["end"]])
print("constexpr std::uint32_t script_capture[] = {%s};" % ", ".join(str(c) for c in cap))
print("constexpr event script[] = {{0, 0, 0, 0},  // keeps the array from being empty")
for at, x, y, down in s.get("events", []):
    print("    {%d, %d, %d, %d}," % (at, x, y, down))
print("};")
