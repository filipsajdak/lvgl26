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

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv {

// One event of one widget: something to wait for.
template <lv_event_code_t Code>
struct event_ref {
  // No default value: the events of a widget are a union of these (see events_of),
  // and a union member must be trivial to construct.
  lv_obj_t* widget;

  // For lv::first: what to do when this event is the first to happen.
  //   button.next().clicked.then([] { ... })
  // Defined in lv/task.hpp.
  template <std::invocable F>
  auto then(F handler) const;
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

// The events of one widget, by name. It is a union: every member is an event_ref, a
// struct of one widget pointer, so all of them are the same pointer and the whole
// thing is as large as one. (A struct with a member for each event was 78 pointers,
// kept in the frame of every task that waits for one event.) Reading the widget
// through any member is defined: the members are standard-layout structs that start
// with the same member.
template <class D = void>
struct events_of {
  union type;
  consteval { std::meta::define_aggregate(^^type, event_specs<D>()); }
};

// The events of the widget `w`.
template <class D = void>
typename events_of<D>::type make_events(lv_obj_t* w) {
  using events = typename events_of<D>::type;
  constexpr auto first = std::meta::nonstatic_data_members_of(
      ^^events, std::meta::access_context::unchecked())[0];
  events all{};  // the first member is the one that is alive
  all.[:first:].widget = w;
  return all;
}

}  // namespace detail

}  // namespace lv
