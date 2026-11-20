// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

struct row {
  int a;
};
constexpr auto parts = lv::for_each_field<row>([]<auto F> { return lv::label(); });
