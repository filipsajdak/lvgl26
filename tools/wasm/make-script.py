#!/usr/bin/env python3
"""Writes script.json of examples/wasm as the C++ include that host_main.cpp reads."""
import json, sys
s = json.load(open(sys.argv[1]))
print("constexpr std::uint32_t script_end = %d;" % s["end"])
print("constexpr std::uint32_t script_step = %d;" % s["step"])
print("constexpr event script[] = {")
for at, x, y, down in s["events"]:
    print("    {%d, %d, %d, %d}," % (at, x, y, down))
print("};")
