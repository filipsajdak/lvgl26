# Match

`lv::match` chooses by value and is checked at compile time: every value has exactly one arm.

Header: `<lv/lv.hpp>`, the core.

## A choice by value

The shape is that of the pattern matching proposed for C++29 (P2688):

```cpp
enum class mode { off, eco, sport };

const char* name = lv::match(m)(
  lv::is<mode::off>.then([] { return "Off"; }),
  lv::is<mode::eco>.then([] { return "Eco"; }),
  lv::is<mode::sport>.then([] { return "Sport"; })
);

int level = lv::match(n)(lv::is<1, 2>.then([] { return 0; }), lv::otherwise.then([] { return 1; }));
```

## What the compiler checks

For an enum, reflection checks that every enumerator has exactly one arm. Leave one out, and the build stops:

<!-- message: match_missing_case -->
```text
error: static assertion failed: lv::match does not cover: sport. Add an arm for each, or a last arm `lv::otherwise.then(...)`.
```

These stop the build too, each with a test of its own in `tests/fail/`:

- two arms for one value (`match_duplicate_case`);
- an arm with a value of another enum (`match_other_enum`);
- an `lv::otherwise` that can never match, because the arms cover every value (`match_redundant_otherwise`);
- no `lv::otherwise` on a type whose values cannot be listed, such as `int` (`match_needs_otherwise`);
- an `lv::otherwise` that is not the last arm, or two of them (`match_otherwise_not_last`, `match_two_otherwise`);
- a matcher kept in a variable: `lv::match(value)(arms...)` is one expression (`match_stored`).

## Where else it is used

The arms of `lv::shown`, which shows a tree for each value of a state, are the same patterns: [Pages that follow a state](mounting.md#pages-that-follow-a-state).
