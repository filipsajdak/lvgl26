#include <lv/lv.hpp>
#include <lv/shown.hpp>
enum class page { home, settings };
enum class look { light, dark };
constinit lv::state<page> current{page::home};
constexpr auto a = lv::label();
constexpr auto ui = lv::group()[lv::shown(current)[lv::is<look::light>(a), lv::otherwise()]];
