#pragma once

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>

#include <concepts>
#include <cstring>
#include <type_traits>
#include <utility>

namespace lv {

template <detail::fixed W>
class widget;

using obj = widget<"obj">;

// What an event handler receives.
class event {
  lv_event_t* e_;

 public:
  constexpr explicit event(lv_event_t* e) : e_(e) {}
  constexpr lv_event_t* raw() const { return e_; }
  lv_event_code_t code() const { return lv_event_get_code(e_); }

  // The widget the handler was added to.
  template <class V = obj>
  V target() const {
    return V{lv_event_get_current_target_obj(e_)};
  }
};

namespace detail {

template <class F>
concept event_handler = std::invocable<F&, event> || std::invocable<F&>;

template <class F>
concept stateless = std::is_empty_v<F> && std::default_initializable<F>;

// A handler whose state fits in LVGL's user_data pointer, so nothing is allocated.
template <class F>
concept pointer_sized = std::is_trivially_copyable_v<F> &&
                        std::is_trivially_destructible_v<F> && sizeof(F) <= sizeof(void*);

template <class F>
void call(F& f, lv_event_t* e) {
  if constexpr (std::invocable<F&, event>)
    f(event{e});
  else
    f();
}

}  // namespace detail

// A non-owning, pointer-sized view of an LVGL widget. Copy it freely.
// It never deletes the widget: see lv::owned for that.
template <detail::fixed W>
class widget {
  lv_obj_t* p_ = nullptr;

 public:
  static constexpr std::string_view kind = W.sv();

  constexpr widget() = default;
  constexpr explicit widget(lv_obj_t* p) : p_(p) {}

  // Every widget is an obj.
  constexpr operator widget<"obj">() const
    requires(W.sv() != "obj")
  {
    return widget<"obj">{p_};
  }

  constexpr lv_obj_t* raw() const { return p_; }
  constexpr explicit operator bool() const { return p_ != nullptr; }
  friend constexpr bool operator==(widget, widget) = default;

  // Creates the widget as a child of `parent`. The parent owns it.
  static widget create(widget<"obj"> parent)
    pre(parent.raw() != nullptr)
  {
    return widget{[:detail::resolve_create(W.sv()):](parent.raw())};
  }

  // Creates a child of this widget.
  template <detail::fixed C>
  widget<C> add() const {
    return widget<C>::create(widget<"obj">{p_});
  }

  // set<"text">("Speed") calls lv_label_set_text, or lv_obj_set_* as a fallback.
  template <detail::fixed P, class Self, class... A>
  Self set(this Self self, A&&... a)
    pre(self.raw() != nullptr)
  {
    [:detail::resolve(W.sv(), "set", P.sv()):](self.raw(), std::forward<A>(a)...);
    return self;
  }

  // get<"text">() calls lv_label_get_text, or lv_obj_get_* as a fallback.
  template <detail::fixed P, class... A>
  auto get(A&&... a) const {
    // A deduced return type with pre() crashes GCC 16.2, so the check is in the body.
    contract_assert(p_ != nullptr);
    return [:detail::resolve(W.sv(), "get", P.sv()):](p_, std::forward<A>(a)...);
  }

  // on<LV_EVENT_CLICKED>(handler). The handler takes lv::event or nothing.
  // It may capture nothing, or one pointer-sized trivially copyable value.
  template <lv_event_code_t Code, class Self, detail::event_handler F>
  Self on(this Self self, F f)
    pre(self.raw() != nullptr)
  {
    if constexpr (detail::stateless<F>) {
      lv_obj_add_event_cb(
          self.raw(),
          [](lv_event_t* e) {
            F h{};
            detail::call(h, e);
          },
          Code, nullptr);
    } else if constexpr (detail::pointer_sized<F>) {
      void* state = nullptr;
      std::memcpy(&state, &f, sizeof(F));
      lv_obj_add_event_cb(
          self.raw(),
          [](lv_event_t* e) {
            void* state = lv_event_get_user_data(e);
            alignas(F) unsigned char buf[sizeof(F)];
            std::memcpy(buf, &state, sizeof(F));
            detail::call(*reinterpret_cast<F*>(buf), e);
          },
          Code, state);
    } else {
      static_assert(false,
                    "lv: an event handler may capture nothing, or one pointer-sized trivially "
                    "copyable value (for example one pointer or one view). Capture a pointer "
                    "to your state instead.");
    }
    return self;
  }
};

using label = widget<"label">;
using button = widget<"button">;
using arc = widget<"arc">;

static_assert(sizeof(label) == sizeof(void*));

}  // namespace lv
