#include <lv/lv.hpp>
using namespace lv::units;
constinit lv::state turn{90 * deg};
constexpr auto ui = lv::widget<"image">({.rotation = lv::bind(turn)});
