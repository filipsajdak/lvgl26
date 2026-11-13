#pragma once

// The events of a widget by name, generated from LVGL's lv_event_code_t:
//
//   co_await button.next().clicked;
//   co_await slider.next().value_changed;
//   button.next().gone      // the widget is deleted (LVGL's LV_EVENT_DELETE)
//
// view.next() returns a struct with one member per event. Each member names one
// event of that widget; lv/task.hpp makes it something a coroutine can wait for.

#include <lv/detail/api.hpp>
#include <lv/values.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv {

// One event of one widget: something to wait for.
template <lv_event_code_t Code>
struct event_ref {
  lv_obj_t* widget = nullptr;
};

namespace detail {

// "LV_EVENT_VALUE_CHANGED" -> "value_changed". LV_EVENT_DELETE is "gone", because
// `delete` is a keyword. Empty for the entries that are not events.
consteval std::string event_name(std::string_view enumerator) {
  constexpr std::string_view prefix = "LV_EVENT_";
  if (!enumerator.starts_with(prefix)) return {};
  const auto rest = enumerator.substr(prefix.size());
  if (is_one_of(rest, {"ALL", "LAST", "LAST_CUSTOM", "PREPROCESS", "MARKED_DELETING"})) return {};
  if (rest == "DELETE") return "gone";
  std::string out;
  for (char c : rest) out.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c);
  return out;
}

// The name of an event code: "clicked" for LV_EVENT_CLICKED.
consteval std::string_view event_name_of(lv_event_code_t code) {
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t)))
    if (std::meta::extract<lv_event_code_t>(e) == code)
      return std::define_static_string(event_name(std::meta::identifier_of(e)));
  throw unknown_name("lv: this event code has no name");
}

// A template on D only so that the struct is generated on first use.
template <class D>
consteval std::vector<std::meta::info> event_specs() {
  std::vector<std::meta::info> specs;
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t))) {
    const auto name = event_name(std::meta::identifier_of(e));
    if (name.empty()) continue;
    const auto type = std::meta::substitute(
        ^^event_ref, {std::meta::reflect_constant(std::meta::extract<lv_event_code_t>(e))});
    specs.push_back(std::meta::data_member_spec(type, {.name = name}));
  }
  return specs;
}

template <class D = void>
struct events_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, event_specs<D>()); }
};

// The events of the widget `w`.
template <class D = void>
constexpr typename events_of<D>::type make_events(lv_obj_t* w) {
  typename events_of<D>::type events{};
  template for (constexpr auto m :
                std::define_static_array(std::meta::nonstatic_data_members_of(
                    ^^typename events_of<D>::type, std::meta::access_context::unchecked()))) {
    events.[:m:].widget = w;
  }
  return events;
}

}  // namespace detail

}  // namespace lv
