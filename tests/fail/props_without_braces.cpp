#include <lv/lv.hpp>

// Without braces the members of p would hold whatever was in memory, and each
// member says whether it is set. `lv::props<"arc"> p{};` is the way.
lv::props<"arc"> p;
