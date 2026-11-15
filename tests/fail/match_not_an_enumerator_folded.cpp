#include <lv/lv.hpp>
enum class mode { off, eco };
static_assert(lv::match(static_cast<mode>(7))(lv::is<mode::off>.then([] { return 0; }),
                                              lv::is<mode::eco>.then([] { return 1; })) == 0);
