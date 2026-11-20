#include <lv/lv.hpp>
#include <lv/repeat.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
struct row_type { [[=lv::caption{std::string_view{"queue.owner"}}]] const char* owner; };
void f() {}
