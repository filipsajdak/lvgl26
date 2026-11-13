#pragma once

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/props.hpp>

#include <concepts>
#include <cstring>
#include <type_traits>
#include <utility>

namespace lv {

template <detail::fixed W>
class view;

using obj = view<"obj">;

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
class view {
  lv_obj_t* p_ = nullptr;

 public:
  static constexpr std::string_view kind = W.sv();

  constexpr view() = default;
  constexpr explicit view(lv_obj_t* p) : p_(p) {}

  // Every widget is an obj.
  constexpr operator view<"obj">() const
    requires(W.sv() != "obj")
  {
    return view<"obj">{p_};
  }

  constexpr lv_obj_t* raw() const { return p_; }
  constexpr explicit operator bool() const { return p_ != nullptr; }
  friend constexpr bool operator==(view, view) = default;

  // Creates the widget as a child of `parent`. The parent owns it.
  static view create(view<"obj"> parent)
    pre(parent.raw() != nullptr)
  {
    return view{[:detail::resolve_create(W.sv()):](parent.raw())};
  }

  // Creates a child of this widget.
  template <detail::fixed C>
  view<C> add() const {
    return view<C>::create(view<"obj">{p_});
  }

  // set({.value = 0}) applies typed properties: see lv::props.
  template <class Self, class D = void>
  Self set(this Self self, const detail::lazy_props<W, D>& p)
    pre(self.raw() != nullptr)
  {
    detail::apply_props<W>(self.raw(), p);
    return self;
  }

  // The untyped escape hatch, for anything props does not cover:
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
            void* stored = lv_event_get_user_data(e);
            alignas(F) unsigned char buf[sizeof(F)];
            std::memcpy(buf, &stored, sizeof(F));
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

static_assert(sizeof(obj) == sizeof(void*));

}  // namespace lv
