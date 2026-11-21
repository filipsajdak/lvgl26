#include <lv/lv.hpp>
#include <lv/shown.hpp>
using namespace lv::units;
enum class page { home, settings };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto b = lv::label();
constexpr auto slide = lv::transition({.x = {100 * px, 0 * px}, .y = {10 * px, 0 * px}}, 200 * ms);
