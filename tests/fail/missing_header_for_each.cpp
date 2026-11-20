// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

enum class mode { a, b };
constexpr auto parts = lv::for_each<mode>([]<mode M> { return lv::label(); });
