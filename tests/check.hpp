#pragma once

#include <cstdio>
#include <source_location>

// A minimal test harness: check() records a failure and the test goes on.
inline int failures = 0;

inline void check(bool ok, std::source_location where = std::source_location::current()) {
  if (ok) return;
  ++failures;
  std::printf("%s:%lu: check failed\n", where.file_name(), static_cast<unsigned long>(where.line()));
}
