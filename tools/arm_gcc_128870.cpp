// The reproducer of GCC bug 128870, run by tools/arm-run.sh: GCC 16.2 rejects this at random
// with "taking address of an immediate function" when -fdeclone-ctor-dtor is on (-Os, -Oz).
#include <optional>
#include <string>
struct Name {
  template<class T> requires std::constructible_from<std::string, T>
  consteval Name(T&& n) : s((T&&) n) {}
  std::string s;
};
struct Options { std::optional<Name> name; };
consteval int f(Options o) { return o.name ? 1 : 0; }
consteval int g() { return f({.name = "title"}); }
constexpr int i = g();
int main() { return i; }
