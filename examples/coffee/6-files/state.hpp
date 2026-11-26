#pragma once

// What several screens share: where the machine is, and the drink chosen with its price. A state
// that only one screen uses lives in that screen's file.

#include <lv/lv.hpp>

#include <utility>

namespace coffee {

//! [state]
enum class drink { espresso, lungo, cappuccino, latte };
enum class step { drinks, cup, brewing, enjoy };

inline constinit lv::state chosen{drink::espresso};
inline constinit lv::state<step> current{step::drinks};

// The price of each drink, in the order of the enum: the drinks page shows it, and so does enjoy.
inline constexpr const char* prices[] = {"2.20 EUR", "2.40 EUR", "3.10 EUR", "3.30 EUR"};

inline constinit lv::computed price{[](drink d) { return prices[std::to_underlying(d)]; }, chosen};
//! [state]

}  // namespace coffee
