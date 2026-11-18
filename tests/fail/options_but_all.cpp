#include <lv/lv.hpp>
enum class quality { best, medium };
constexpr auto o = lv::options_of<quality, lv::but<quality::best, quality::medium>>;
