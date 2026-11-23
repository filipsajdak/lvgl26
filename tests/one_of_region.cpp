// detail::one_of_region: one of several objects in one place, built in place and ended by reset or by the destructor.
#include <lv/detail/one_of_region.hpp>

#include "check.hpp"

#include <cstddef>
#include <type_traits>

namespace {

using lv::detail::one_of_region;

int alive = 0;
int ended_first = 0;
int ended_second = 0;
int ended_third = 0;

struct first {
  int value = 1;
  first() { ++alive; }
  ~first() { --alive, ++ended_first; }
};
struct second {
  long value = 20;
  second() { ++alive; }
  ~second() { --alive, ++ended_second; }
};
struct third {
  char value = 3;
  third() { ++alive; }
  ~third() { --alive, ++ended_third; }
};

using region = one_of_region<first, second, third>;

// The region is not copied or moved, and a constinit one holds nothing.
static_assert(!std::is_copy_constructible_v<region> && !std::is_copy_assignable_v<region>);
static_assert(!std::is_move_constructible_v<region> && !std::is_move_assignable_v<region>);
static_assert(region::none == 3);
static_assert(sizeof(region) >= sizeof(second));
constinit region constant;
static_assert(one_of_region<>::none == 0);

void nothing_alive() {
  region r;
  check(r.index() == region::none);
  r.reset();  // nothing to end
  check(r.index() == region::none);
  check(alive == 0 && ended_first == 0 && ended_second == 0 && ended_third == 0);
}

void each_one() {
  region r;
  check(r.emplace<0>().value == 1);
  check(r.index() == 0 && alive == 1);
  check(r.get<0>().value == 1);
  r.reset();
  check(r.index() == region::none && alive == 0);
  check(ended_first == 1 && ended_second == 0 && ended_third == 0);

  check(r.emplace<1>().value == 20);
  check(r.index() == 1 && alive == 1);
  r.reset();
  check(ended_first == 1 && ended_second == 1 && ended_third == 0);

  check(r.emplace<2>().value == 3);
  check(r.index() == 2 && alive == 1);
  check(std::as_const(r).get<2>().value == 3);
  r.reset();
  check(ended_first == 1 && ended_second == 1 && ended_third == 1);
  check(alive == 0);
}

// Making another object ends the one that is alive, and only that one.
void replace() {
  ended_first = ended_second = ended_third = 0;
  region r;
  r.emplace<2>();
  r.emplace<0>();
  check(r.index() == 0 && alive == 1);
  check(ended_third == 1 && ended_first == 0 && ended_second == 0);
  r.emplace<1>();
  check(ended_first == 1 && ended_second == 0);
  check(r.index() == 1 && alive == 1);
}

void the_destructor_ends_it() {
  ended_first = ended_second = ended_third = 0;
  {
    region r;
    r.emplace<1>();
    check(alive == 1);
  }
  check(alive == 0 && ended_second == 1 && ended_first == 0 && ended_third == 0);
}

}  // namespace

int main() {
  nothing_alive();
  each_one();
  replace();
  the_destructor_ends_it();
  return failures;
}
