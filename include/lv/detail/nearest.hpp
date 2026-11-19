#pragma once

// Names near a misspelled one, for the messages of compile errors: "Did you mean 'threads'?".

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

// How far apart two names are, counting a letter inserted, left out or changed.
consteval std::size_t distance_between(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t diagonal = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t above = row[j];
      const std::size_t change = diagonal + (a[i - 1] == b[j - 1] ? 0 : 1);
      row[j] = std::min({above + 1, row[j - 1] + 1, change});
      diagonal = above;
    }
  }
  return row[b.size()];
}

// "a, b, c"
consteval std::string joined_names(const std::vector<std::string>& names) {
  std::string out;
  for (const auto& n : names) out += (out.empty() ? "" : ", ") + n;
  return out;
}

// How many names a message offers for a name that is none of them.
inline constexpr std::size_t names_offered = 5;

// The names of `names` nearest to `wanted`, nearest first, at most `names_offered`.
consteval std::vector<std::string> nearest_list(std::string_view wanted, std::vector<std::string> names) {
  std::vector<std::size_t> far;
  for (const auto& n : names) far.push_back(distance_between(wanted, n));
  std::vector<std::string> picked;
  for (std::size_t k = 0; k < names_offered && k < names.size(); ++k) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < names.size(); ++i)
      if (far[i] < far[best]) best = i;
    picked.push_back(names[best]);
    far[best] = static_cast<std::size_t>(-1);
  }
  return picked;
}

consteval std::string nearest_names(std::string_view wanted, std::vector<std::string> names) {
  return joined_names(nearest_list(wanted, std::move(names)));
}

// A name this far from another (a letter more, less or different) is a likely typo for it.
inline constexpr std::size_t close_enough = 2;

// " Did you mean 'threads'?" when one of `names` is a likely typo for `wanted`, else nothing.
consteval std::string did_you_mean(std::string_view wanted, std::vector<std::string> names) {
  if (names.empty()) return {};
  const std::string closest = nearest_list(wanted, std::move(names)).front();
  return distance_between(wanted, closest) <= close_enough ? " Did you mean '" + closest + "'?" : std::string{};
}

}  // namespace lv::detail
