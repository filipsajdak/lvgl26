// The names near a misspelled one (lv/detail/nearest.hpp): the distance between two names, the nearest
// few of a list and the "Did you mean" sentence of a compile error. All of it is constant evaluation,
// so the checks are static_asserts.
#include <lv/detail/nearest.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace {

using lv::detail::distance_between;
using lv::detail::did_you_mean;
using lv::detail::joined_names;
using lv::detail::nearest_list;
using lv::detail::nearest_names;
using names = std::vector<std::string>;

// Letters inserted, left out or changed; a swapped pair counts as two changes.
static_assert(distance_between("", "") == 0);
static_assert(distance_between("abc", "abc") == 0);
static_assert(distance_between("abc", "") == 3);
static_assert(distance_between("", "abc") == 3);
static_assert(distance_between("abc", "abcd") == 1);
static_assert(distance_between("abcd", "abc") == 1);
static_assert(distance_between("abc", "xbc") == 1);
static_assert(distance_between("abc", "axc") == 1);
static_assert(distance_between("abc", "abx") == 1);
static_assert(distance_between("ab", "ba") == 2);
static_assert(distance_between("kitten", "sitting") == 3);
static_assert(distance_between("sitting", "kitten") == 3);
static_assert(distance_between("saturday", "sunday") == 3);
static_assert(distance_between("flaw", "lawn") == 2);
static_assert(distance_between("thread", "threads") == 1);
static_assert(distance_between("allocation", "alocation") == 1);
static_assert(distance_between("abc", "xyz") == 3);
static_assert(distance_between("a", "bcdefg") == 6);

consteval bool joins() {
  return joined_names({}) == "" && joined_names({"a"}) == "a" && joined_names({"a", "b", "c"}) == "a, b, c";
}
static_assert(joins());

// The nearest first, at most five, ties in the order of the list, the names that are none the better left out.
consteval bool picks() {
  const names all{"threads", "files", "allocation", "floating_point", "thread", "threadz", "other"};
  const auto picked = nearest_list("thread", all);
  if (picked != names{"thread", "threads", "threadz", "other", "files"}) return false;
  if (nearest_list("x", {"b", "a"}) != names{"b", "a"}) return false;  // equal distance: list order
  if (nearest_list("abc", {}) != names{}) return false;
  if (nearest_list("abc", {"abd"}) != names{"abd"}) return false;
  if (nearest_list("abc", {"zzzzzz", "abd", "abcd", "zzz", "q", "abc"}) != names{"abc", "abd", "abcd", "zzz", "q"}) return false;
  return nearest_names("thread", all) == "thread, threads, threadz, other, files";
}
static_assert(picks());

// A name at most two letters from another is a typo for it; farther is none.
consteval bool suggests() {
  const names keys{"allocation", "threads", "files"};
  return did_you_mean("thread", keys) == " Did you mean 'threads'?" && did_you_mean("trheads", keys) == " Did you mean 'threads'?" &&
         did_you_mean("threads", keys) == " Did you mean 'threads'?" && did_you_mean("thr", keys) == "" && did_you_mean("thre", keys) == "" && did_you_mean("xthreadsx", keys) == " Did you mean 'threads'?" &&
         did_you_mean("xyz", keys) == "" && did_you_mean("fil", keys) == " Did you mean 'files'?" &&
         did_you_mean("anything", {}) == "" && did_you_mean("allocatio", {"allocation"}) == " Did you mean 'allocation'?";
}
static_assert(suggests());

}  // namespace

int main() { return 0; }
