#pragma once

// Editor stub of lv/detail/theme_styles.hpp.

namespace lv {

namespace stub {
struct theme_access;
}  // namespace stub

/// What lv::theme returns: the styles of a theme in read-only memory. Give it to an
/// lv::display as its last argument.
class theme_styles {
 public:
  constexpr theme_styles() = default;
  /// True for a theme that sets nothing.
  constexpr bool empty() const { return !set_; }

 private:
  friend struct stub::theme_access;
  constexpr explicit theme_styles(bool set) : set_(set) {}
  bool set_ = false;
};

namespace stub {

struct theme_access {
  // A theme that sets something: the stubs do not look at what.
  static constexpr theme_styles some() { return theme_styles{true}; }
};

}  // namespace stub

}  // namespace lv
