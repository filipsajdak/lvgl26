#pragma once

// Typed application state over LVGL's observer.
//
//   struct kmh { int value; };                    // a named value: says what the number is
//   constinit lv::state<kmh> speed{kmh{0}};       // a constant-initialized global
//
//   lv::arc({.value = lv::bind(speed)})           // the arc follows speed, and writes back
//   lv::label({.text = lv::bind(speed, "{} km/h")})
//
//   speed = kmh{120};                             // every bound widget updates
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
concept state_value = plain_state_value<T> || strong_value<T> || std::same_as<T, lv_color_t> ||
                      std::is_pointer_v<T>;

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
  else if constexpr (strong_value<T>)
    lv_subject_set_int(s, static_cast<std::int32_t>(v.[:strong_member<T>():]));
  else
    lv_subject_set_int(s, static_cast<std::int32_t>(v));
}

template <class T>
T read_subject(lv_subject_t* s) {
  if constexpr (std::same_as<T, lv_color_t>) {
    return lv_subject_get_color(s);
  } else if constexpr (std::is_pointer_v<T>) {
    return static_cast<T>(lv_subject_get_pointer(s));
  } else if constexpr (strong_value<T>) {
    using member = typename state_payload<T>::type;
    return T{static_cast<member>(lv_subject_get_int(s))};
  } else if constexpr (std::same_as<T, bool>) {
    return lv_subject_get_int(s) != 0;
  } else {
    return static_cast<T>(lv_subject_get_int(s));
  }
}

// The LVGL subject behind a state, created on first use.
template <class T, class O>
lv_subject_t* subject_of(const void* source) {
  return static_cast<const state<T, O>*>(source)->raw();
}

}  // namespace detail

// A value that widgets can follow. See the top of this file.
// O is the settings tag; leave it out.
template <class T, class O = override>
class state {
  static_assert(detail::state_value<T>,
                "lv: a state holds a named value (a struct with one integer, bool or enum "
                "member, such as `struct kmh { int value; };`), a bool, an enum, an "
                "lv_color_t or a pointer (for text: const char*).");
  static_assert(!detail::integer<T> || cfg<O>.raw_integer_state,
                "lv: a state of a plain integer does not say what the number is. Give it a "
                "named type: `struct kmh { int value; }; lv::state<kmh> speed;`. To allow "
                "plain integers, set lv::config::raw_integer_state.");

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
      return lv_subject_add_observer(s, &call_stateless<F>, nullptr);
    } else if constexpr (detail::pointer_sized<F>) {
      void* stored = nullptr;
      std::memcpy(&stored, &f, sizeof(F));
      return lv_subject_add_observer(s, &call_stored<F>, stored);
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

 private:
  template <class F>
  static void call_stateless(lv_observer_t*, lv_subject_t* subject) {
    F h{};
    h(detail::read_subject<T>(subject));
  }

  template <class F>
  static void call_stored(lv_observer_t* observer, lv_subject_t* subject) {
    void* kept = lv_observer_get_user_data(observer);
    alignas(F) unsigned char buf[sizeof(F)];
    std::memcpy(buf, &kept, sizeof(F));
    (*reinterpret_cast<F*>(buf))(detail::read_subject<T>(subject));
  }
};

// Binds a property to a state: .value = lv::bind(speed).
template <class T, class O>
constexpr binding<T> bind(const state<T, O>& s) {
  return {&s, &detail::subject_of<T, O>};
}

// Binds a text property to a state through a format: lv::bind(speed, "{} km/h").
template <class T, class O>
constexpr formatted_binding<T> bind(const state<T, O>& s, text_format format) {
  return {&s, &detail::subject_of<T, O>, format.printf};
}

}  // namespace lv
