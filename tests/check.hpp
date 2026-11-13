#pragma once

#include <cstdio>

// A minimal test harness: CHECK records a failure and the test goes on.
inline int failures = 0;

#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      ++failures;                                                        \
      std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                    \
  } while (0)
