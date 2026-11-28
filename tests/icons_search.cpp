// The search of an icon's name halves the table: it reads about 11 of its 1530 lines. A search that
// reads them one by one needs far more operations than the limit this file compiles under.
#include <lv/icons.hpp>

static_assert(lv::detail::code_of_icon("acorn") == 0xeb9a);  // the first line
static_assert(lv::detail::code_of_icon("youtube-logo") == 0xe4fc);  // the last
static_assert(lv::detail::code_of_icon("lightning") == 0xe2de);
static_assert(lv::detail::code_of_icon("no-such-icon") == 0);

int main() {}
