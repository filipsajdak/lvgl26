#pragma once

// One of several objects in one place, as std::variant keeps them: the size and the
// alignment are those of the largest, and one object is alive at a time. lv::one_of keeps
// its trees in one, and lv::shown will keep its arms in one.
//
//   one_of_region<mounted<home>, mounted<settings>> r;  // constinit: nothing alive
//   r.emplace<1>();         // ends the object that is alive first
//   r.get<1>().mount(parent);
//   r.reset();              // ends it: its destructor runs
//
// The objects are never moved: emplace builds them in place, and get gives a reference
// to the one that is alive. The region itself is not copied or moved.

#include <cstddef>
#include <memory>
#include <utility>

namespace lv::detail {

// No object: the member that is active while nothing is alive, so that a constinit
// region is a constant.
struct no_alternative {};

template <class... Ts>
union alternatives;

template <>
union alternatives<> {
  no_alternative none{};
};

// The objects as one union: `first`, or one of `rest`.
template <class T, class... Ts>
union alternatives<T, Ts...> {
  no_alternative none{};
  T first;
  alternatives<Ts...> rest;

  constexpr alternatives() {}
  constexpr ~alternatives() {}
};

// The object number I of the union `a`.
template <std::size_t I, class A>
constexpr auto& alternative(A& a) {
  if constexpr (I == 0)
    return a.first;
  else
    return alternative<I - 1>(a.rest);
}

// Starts the object number I of `a`, made from nothing; the members on its way become active.
template <std::size_t I, class A>
auto& start_alternative(A& a) {
  if constexpr (I == 0) {
    return *std::construct_at(&a.first);
  } else {
    std::construct_at(&a.rest);
    return start_alternative<I - 1>(a.rest);
  }
}

template <class... Ts>
class one_of_region {
  alternatives<Ts...> place_;
  std::size_t index_ = sizeof...(Ts);

 public:
  // The index of no object.
  static constexpr std::size_t none = sizeof...(Ts);

  constexpr one_of_region() = default;
  one_of_region(const one_of_region&) = delete;
  one_of_region& operator=(const one_of_region&) = delete;
  ~one_of_region() { reset(); }

  // Which object is alive, or `none`.
  std::size_t index() const noexcept { return index_; }

  // Ends the object that is alive, then makes object number I from nothing, in place.
  template <std::size_t I>
  auto& emplace() {
    reset();
    auto& made = start_alternative<I>(place_);
    index_ = I;
    return made;
  }

  // Ends the object that is alive, if one is.
  void reset() noexcept {
    [this]<std::size_t... I>(std::index_sequence<I...>) {
      ((index_ == I ? std::destroy_at(&alternative<I>(place_)) : void()), ...);
    }(std::index_sequence_for<Ts...>{});
    index_ = none;
  }

  // Object number I. It must be alive.
  template <std::size_t I>
  auto& get() noexcept {
    return alternative<I>(place_);
  }
  template <std::size_t I>
  const auto& get() const noexcept {
    return alternative<I>(place_);
  }
};

}  // namespace lv::detail
