// `numerals` holds one file, Barlow Condensed Bold, with the characters of a reading.
// `body` is Barlow at weight 500: that font comes from lv::signal::type.
#include <lv/lv.hpp>
#include <lv/signal_type.hpp>

constexpr auto body = lv::signal::numerals(lv::signal::type_tokens.text.body);

int main() {}
