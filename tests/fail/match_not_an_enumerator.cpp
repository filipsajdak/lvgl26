#include <lv/lv.hpp>
enum class mode { off, eco };
constexpr int level = lv::match(static_cast<mode>(7))(lv::is<mode::off>.then([] { return 0; }),
                                                      lv::is<mode::eco>.then([] { return 1; }));
