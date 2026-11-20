#include <lv/lv.hpp>
enum class page { home, settings };
constexpr auto a = lv::label();
constexpr auto arm = lv::is<page::home>(a);
