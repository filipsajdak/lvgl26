#pragma once

// Typed application state over LVGL's observer.
//
//   lv::state<int> speed{0};                      // a constant-initialized global
//
//   lv::arc({.value = lv::bind(speed)})           // the arc follows speed, and writes back
//   lv::label({.text = lv::bind(speed, "%d km/h")})
//
//   speed = 120;                                  // every bound widget updates
//
// A state is an lv_subject_t underneath, created the first time something binds to
// it or asks for raw(). C code can use that subject like any other.
// Like all of LVGL, a state must only be touched from the UI thread.

#include <lv/values.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace lv {

namespace detail {

// What a state can hold, and which kind of subject carries it.
template <class T>
concept int_state = integer<T> || std::same_as<T, bool> || std::is_enum_v<T> ||
                    std::same_as<T, angle> || std::same_as<T, duration>;

template <class T>
concept state_value = int_state<T> || std::same_as<T, lv_color_t> || std::is_pointer_v<T>;

template <class T>
constexpr lv_subject_type_t subject_type() {
  if constexpr (std::same_as<T, lv_color_t>)
    return LV_SUBJECT_TYPE_COLOR;
  else if constexpr (std::is_pointer_v<T>)
    return LV_SUBJECT_TYPE_POINTER;
  else
    return LV_SUBJECT_TYPE_INT;
}

template <class T>
void write_subject(lv_subject_t* s, T v) {
  if constexpr (std::same_as<T, lv_color_t>)
    lv_subject_set_color(s, v);
  else if constexpr (std::is_pointer_v<T>)
    lv_subject_set_pointer(s, const_cast<void*>(static_cast<const void*>(v)));
  else if constexpr (requires { v.raw(); })
    lv_subject_set_int(s, static_cast<std::int32_t>(v.raw()));
  else
    lv_subject_set_int(s, static_cast<std::int32_t>(v));
}

template <class T>
T read_subject(lv_subject_t* s) {
  if constexpr (std::same_as<T, lv_color_t>)
    return lv_subject_get_color(s);
  else if constexpr (std::is_pointer_v<T>)
    return static_cast<T>(lv_subject_get_pointer(s));
  else if constexpr (std::same_as<T, angle>)
    return angle{lv_subject_get_int(s)};
  else if constexpr (std::same_as<T, duration>)
    return duration{static_cast<std::uint32_t>(lv_subject_get_int(s))};
  else if constexpr (std::same_as<T, bool>)
    return lv_subject_get_int(s) != 0;
  else
    return static_cast<T>(lv_subject_get_int(s));
}

}  // namespace detail

// A value that widgets can follow. See the top of this file.
template <class T>
class state {
  static_assert(detail::state_value<T>,
                "lv: a state holds an integer, a bool, an enum, an lv::angle, an lv::duration, "
                "an lv_color_t or a pointer (for text: const char*).");

  T value_{};
  mutable lv_subject_t* subject_ = nullptr;

 public:
  constexpr state() = default;
  constexpr explicit state(T initial) : value_(initial) {}
  state(const state&) = delete;
  state& operator=(const state&) = delete;
  ~state() { reset(); }

  // The current value. Reads the subject, so a change made by C code is seen.
  T get() const { return subject_ ? detail::read_subject<T>(subject_) : value_; }

  // Changes the value and updates everything bound to it.
  void set(T v) {
    value_ = v;
    if (subject_) detail::write_subject(subject_, v);
  }

  state& operator=(T v) {
    set(v);
    return *this;
  }

  // The LVGL subject, created on first use. Needs lv_init() to have run.
  // Null only if LVGL has no memory left.
  lv_subject_t* raw() const {
    if (subject_ == nullptr) {
      subject_ = lv_subject_create(detail::subject_type<T>());
      if (subject_) detail::write_subject(subject_, value_);
    }
    return subject_;
  }

  // Calls `f(value)` now and after every change. The handler may capture nothing,
  // or one pointer-sized trivially copyable value. Returns LVGL's observer, which
  // lv_observer_delete() removes.
  template <class F>
    requires std::invocable<F&, T>
  lv_observer_t* observe(F f) const {
    lv_subject_t* s = raw();
    if (s == nullptr) return nullptr;
    if constexpr (detail::stateless<F>) {
      return lv_subject_add_observer(
          s,
          [](lv_observer_t*, lv_subject_t* subject) {
            F h{};
            h(detail::read_subject<T>(subject));
          },
          nullptr);
    } else if constexpr (detail::pointer_sized<F>) {
      void* stored = nullptr;
      std::memcpy(&stored, &f, sizeof(F));
      return lv_subject_add_observer(
          s,
          [](lv_observer_t* observer, lv_subject_t* subject) {
            void* kept = lv_observer_get_user_data(observer);
            alignas(F) unsigned char buf[sizeof(F)];
            std::memcpy(buf, &kept, sizeof(F));
            (*reinterpret_cast<F*>(buf))(detail::read_subject<T>(subject));
          },
          stored);
    } else {
      static_assert(false,
                    "lv: an observer may capture nothing, or one pointer-sized trivially "
                    "copyable value. Capture a pointer to your state instead.");
    }
  }

  // Deletes the subject. Widgets bound to it stop following. Called by the destructor.
  void reset() {
    if (subject_ != nullptr && lv_is_initialized()) {
      value_ = detail::read_subject<T>(subject_);
      lv_subject_delete(subject_);
    }
    subject_ = nullptr;
  }
};

// Binds a property to a state: .value = lv::bind(speed).
template <class T>
constexpr binding<T> bind(const state<T>& s) {
  return {&s};
}

// Binds a text property to a state through a printf format: lv::bind(speed, "%d km/h").
template <class T>
constexpr formatted_binding<T> bind(const state<T>& s, const char* format) {
  return {&s, format};
}

}  // namespace lv
