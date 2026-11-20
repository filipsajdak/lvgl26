#include <lv/lv.hpp>
enum class page { home, settings };
constinit lv::state<page> current{page::home};
auto node = lv::shown(current);
