#pragma once

// Editor stub of lv/runtime.hpp: a screen built from a JSON document while the program runs.

#include <lv/detail/missing.hpp>
#include <lv/json.hpp>
#include <lv/json_writer.hpp>
#include <lv/props.hpp>
#include <lv/view.hpp>

#include <cstddef>
#include <expected>
#include <string_view>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/runtime.hpp"> = true;
}  // namespace lv::detail

namespace lv {

/// Why a document could not be loaded or a patch applied.
enum class load_error {
  malformed,         // not JSON, or not shaped like a node
  unknown_widget,    // "type" is not one of the widgets this runtime allows
  unknown_property,  // the widget has no such property, or it cannot be set from a document
  bad_value,         // the value does not fit the property
  out_of_memory,     // LVGL could not create a widget
  unknown_id,        // apply() names an id the document does not have, or its widget is gone
  duplicate_id,      // two nodes of one document have the same id
  too_many_ids,      // the document has more ids than a loaded screen can keep
  id_too_long,       // an id is longer than a loaded screen keeps (see id_length)
};

/// What went wrong, and the piece of the document it is about.
struct load_failure {
  load_error error;
  std::string_view at;
};

/// Builds screens from JSON documents, with the widgets it is given:
/// lv::runtime<"obj", "label", "button">::load(document, parent).
template <detail::fixed... Widgets>
  requires detail::known_widgets<Widgets...> && detail::included<"lv/runtime.hpp">
class runtime<Widgets...> {
 public:
  /// The most ids a loaded screen keeps, and the longest id.
  static constexpr std::size_t id_capacity = 32;
  static constexpr std::size_t id_length = 31;

  /// A screen that was loaded. Owns its widgets.
  class loaded {
   public:
    loaded() = default;
    loaded(loaded&&) noexcept = default;
    loaded& operator=(loaded&&) noexcept = default;

    /// The root widget.
    obj root() const;
    /// The widget with this id, or an empty view.
    obj find(std::string_view id) const;
    /// The id of a widget, or an empty text.
    std::string_view id_of(obj widget) const;
    /// Writes the screen as JSON into a sink.
    template <json::sink S>
    void dump(S&&) const {}
    /// Changes properties by id: {"speedo": {"value": 10}}.
    std::expected<void, load_failure> apply(std::string_view patch) const;
  };

  /// Builds the widgets of a JSON document under `parent`.
  static std::expected<loaded, load_failure> load(std::string_view document, obj parent);

  /// Writes the widgets under `root` as JSON into a sink.
  template <json::sink S>
  static void dump(obj, S&&, const loaded* = nullptr) {}
};

}  // namespace lv
