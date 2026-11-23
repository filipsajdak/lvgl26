// detail::input_queue: samples in the order they came, a ring of a fixed size, a merge into the newest when it is full.
#include <lv/detail/input_queue.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstring>

namespace {

using lv::detail::input_queue;

// A merge that folds a sample into the newest one when both have the same kind, and refuses otherwise.
struct sample {
  int kind = 0;
  int value = 0;
};
inline int merges = 0;
struct fold_same_kind {
  static bool merge(sample& newest, sample s) {
    ++merges;
    if (newest.kind != s.kind) return false;
    newest.value = s.value;
    return true;
  }
};

template <std::size_t N>
concept queue_of = requires { typename input_queue<sample, N, fold_same_kind>; };
static_assert(queue_of<1> && queue_of<2> && queue_of<16>);
static_assert(!queue_of<0>);

void empty_queue() {
  input_queue<sample, 4, fold_same_kind> q;
  check(q.empty());
  check(q.size() == 0);
  check(!q.pop().has_value());
  check(q.push({1, 10}));
  check(!q.empty());
  check(q.size() == 1);
}

// A sample that finds room is not offered to the merge; a full queue folds into its newest, at the start of the ring too.
void merge_only_when_full() {
  input_queue<sample, 3, fold_same_kind> q;
  merges = 0;
  check(q.push({1, 1}) && q.push({2, 2}) && q.push({3, 3}));
  check(merges == 0);
  check(q.push({3, 9}));
  check(merges == 1);
  check(!q.push({1, 7}));
  check(merges == 2);
  check(q.pop()->value == 1);
  check(q.pop()->value == 2);
  check(q.pop()->value == 9);
}

void order_and_wrap() {
  input_queue<sample, 3, fold_same_kind> q;
  for (int round = 0; round < 4; ++round) {
    const int base = round * 10;
    check(q.push({1, base + 1}));
    check(q.push({2, base + 2}));
    check(q.push({3, base + 3}));
    check(q.size() == 3);
    check(q.back().value == base + 3);
    check(q.pop()->value == base + 1);
    check(q.pop()->value == base + 2);
    check(q.back().value == base + 3);
    check(q.pop()->value == base + 3);
    check(q.empty());
    check(!q.pop().has_value());
  }
}

// The newest sample after the ring has turned: the read of back() and the merge use the same place.
void merge_when_full_after_a_turn() {
  input_queue<sample, 3, fold_same_kind> q;
  q.push({1, 1});
  q.push({1, 2});
  q.pop();
  q.pop();
  q.push({2, 3});
  q.push({3, 4});
  q.push({4, 5});
  check(q.size() == 3);
  check(q.back().kind == 4 && q.back().value == 5);
  check(q.push({4, 6}));  // full: folded into the newest
  check(q.size() == 3);
  check(q.back().value == 6);
  check(!q.push({5, 7}));  // full, and another kind: refused
  check(q.size() == 3);
  check(q.back().value == 6);
  check(q.pop()->value == 3);
  check(q.pop()->value == 4);
  check(q.pop()->value == 6);
}

void one_place() {
  input_queue<sample, 1, fold_same_kind> q;
  check(q.push({1, 1}));
  check(q.push({1, 2}));
  check(!q.push({2, 3}));
  check(q.size() == 1);
  check(q.pop()->value == 2);
  check(q.empty());
  check(q.push({3, 4}));
  check(q.pop()->value == 4);
}

// back() of an empty queue ends the program.
void back_of_empty() {
  input_queue<sample, 2, fold_same_kind> q;
  [[maybe_unused]] const auto& s = q.back();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "back_of_empty") == 0) {
    back_of_empty();
    return 0;
  }
  empty_queue();
  merge_only_when_full();
  order_and_wrap();
  merge_when_full_after_a_turn();
  one_place();
  return failures;
}
