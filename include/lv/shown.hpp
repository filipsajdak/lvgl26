#pragma once

// lv::shown: a node that follows a state of an enum or a bool, and shows the tree of its value.
//
//   enum class page { home, settings, ink };
//   constinit lv::state<page> current{page::home};
//
//   constexpr auto window = lv::group({.size = {480 * px, 320 * px}})[
//       lv::shown(current)[
//           lv::is<page::home>(home_ui, {.on_shown = &start_home}),
//           lv::is<page::settings, page::ink>(settings_ui),  // one tree for two values: kept between them
//           lv::otherwise()]];                                // nothing for the other values
//
//   constexpr auto banner = lv::group()[lv::when(online)[online_ui]];  // a bool: the tree, or nothing
//
// The rules:
//
//   - An arm is lv::is<values...>(tree), lv::is<values...>() for nothing, or a last
//     lv::otherwise(tree) or lv::otherwise(). Every value has exactly one arm, as in lv::match.
//   - A tree of an arm is a constexpr variable of its own. Its ids are its own: a function
//     given to .on_shown gets an lv::held<tree> after the tree is built, and reaches them there.
//   - lv::shown is the only child of its widget.
//   - The old tree is deleted before the new one is built: the heap holds one page at a time.
//   - A change of the state is shown at the start of the display's next refresh, so several
//     changes in one handler build one tree, and code after `current = page::ink;` still sees
//     the old widgets. lv::settle() shows it at once.
//   - The trees live in the storage of the tree that holds lv::shown, inside its lv::mounted:
//     one region of the size of the largest. lvgl26 allocates nothing for a switch.
//   - If a widget of the old tree had the focus, the first widget of the new tree gets it.

#include <lv/detail/missing.hpp>
#include <lv/animate.hpp>
#include <lv/computed.hpp>
#include <lv/easing.hpp>
#include <lv/detail/deferred.hpp>
#include <lv/detail/watch.hpp>
#include <lv/match.hpp>
#include <lv/one_of.hpp>
#include <lv/state.hpp>
#include <lv/tree.hpp>

#include <lvgl/lvgl.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv::detail {
// This header is read: see lv/detail/missing.hpp.
template <>
inline constexpr bool included<"lv/shown.hpp"> = true;
}  // namespace lv::detail

namespace lv {

namespace detail {

// ---- the arms ----------------------------------------------------------------------

// The handle that a function of .on_shown takes: lv::held<T> of void (*)(lv::held<T>).
template <class F>
struct on_shown_handle;
template <class H>
struct on_shown_handle<void (*)(H)> {
  using type = H;
};

template <auto F>
void call_on_shown(const void* handle) {
  F(*static_cast<const typename on_shown_handle<decltype(F)>::type*>(handle));
}

// What .on_shown keeps of a function void f(lv::held<T>): a call that takes the handle, and
// the tree T, to check it against the arm.
struct shown_hook {
  void (*call)(const void* handle) = nullptr;
  const void* tree = nullptr;

  constexpr shown_hook() = default;

  template <const auto& T>
  consteval shown_hook(void (*f)(held<T>))
      : call(std::meta::extract<void (*)(const void*)>(
            std::meta::substitute(^^call_on_shown, {std::meta::reflect_constant(f)}))),
        tree(&T) {}

  // Any other function: the same signature without the constraint, to say what is wanted.
  template <class F>
  shown_hook(F) = delete("lv::shown: .on_shown and .on_front take a function void f(lv::held<tree>), "
                         "where tree is the tree of the arm.");
};

// What an arm takes beside its tree: lv::is<page::home>(home_ui, {.on_shown = &start, .on_front = &look}).
struct arm_options {
  // Called after the tree is built, with a handle to it: its ids, a task tied to its root.
  shown_hook on_shown;
  // Called each time the page comes to the front: when it was just built, after .on_shown, and
  // when a page that was still built comes back.
  shown_hook on_front;
};

// An arm: the pattern of lv::match, and the tree it shows (none for T = void).
template <class Pattern, class T>
  requires included<"lv/shown.hpp">
struct tree_arm<Pattern, T> {
  const T* tree;
  arm_options options;
};

template <class A>
inline constexpr bool arm_shows_tree = false;
template <class Pattern, class T>
inline constexpr bool arm_shows_tree<tree_arm<Pattern, T>> = !std::is_void_v<T>;

template <class A>
inline constexpr bool is_tree_arm = false;
template <class Pattern, class T>
inline constexpr bool is_tree_arm<tree_arm<Pattern, T>> = true;

template <class A>
struct tree_arm_pattern;
template <class Pattern, class T>
struct tree_arm_pattern<tree_arm<Pattern, T>> {
  using type = Pattern;
};

// ---- the checks of the arms ----------------------------------------------------------

template <class E, auto... Values>
consteval void add_arm_values(std::vector<E>& out, std::type_identity<one_of<Values...>>) {
  (out.push_back(static_cast<E>(Values)), ...);
}
template <class E>
consteval void add_arm_values(std::vector<E>&, std::type_identity<anything>) {}

template <class E, auto... Values>
consteval std::string_view arm_type_problem(std::type_identity<one_of<Values...>>) {
  std::string_view problem;
  ((problem.empty() && !value_of_subject<E, Values>
        ? problem = std::define_static_string(
              "lv::shown follows a state of " + str(std::meta::display_string_of(^^E)) +
              ", and an arm names a value of " + str(std::meta::display_string_of(^^decltype(Values))) +
              ". Name a value of the state's type: lv::is<" + str(std::meta::display_string_of(^^E)) + "::...>.")
        : problem),
   ...);
  return problem;
}
template <class E>
consteval std::string_view arm_type_problem(std::type_identity<anything>) {
  return {};
}

template <class Pattern>
inline constexpr bool is_otherwise = std::same_as<Pattern, anything>;

// Where the arms stand, as an error, or nothing.
template <class... A>
consteval std::string_view arm_order_problem() {
  if (sizeof...(A) == 0) return "lv::shown needs an arm for each value: lv::shown(state)[lv::is<value>(tree), ...].";
  if ((int{is_otherwise<typename tree_arm_pattern<A>::type>} + ...) > 1) return "lv::shown has more than one lv::otherwise arm.";
  if ((is_otherwise<typename tree_arm_pattern<A>::type> || ...) &&
      !is_otherwise<typename tree_arm_pattern<A...[sizeof...(A) - 1]>::type>)
    return "lv::shown: lv::otherwise must be the last arm; the arms after it can never be shown.";
  return {};
}

// The first value that two arms name, as an error, or nothing.
template <class E>
consteval std::string_view duplicate_arm_problem(const std::vector<E>& covered) {
  for (std::size_t i = 0; i < covered.size(); ++i)
    for (std::size_t j = i + 1; j < covered.size(); ++j)
      if (covered[i] == covered[j])
        return std::define_static_string("lv::shown has two arms for " + value_text(covered[i]) +
                                         ". Each value needs one; one arm may name several: lv::is<a, b>(tree).");
  return {};
}

// What is wrong with how the arms cover the values of E, or nothing.
template <class E>
consteval std::string_view arm_coverage_problem(const std::vector<E>& covered, bool wildcard) {
  if (const auto p = duplicate_arm_problem(covered); !p.empty()) return p;
  const auto missing = missing_values(covered);
  if (!wildcard && !missing.empty())
    return std::define_static_string("lv::shown does not cover: " + missing +
                                     ". Add an arm for each; lv::is<value>() shows nothing for it.");
  if (wildcard && missing.empty())
    return "lv::shown already covers every value, so its lv::otherwise arm can never be shown. Remove it.";
  return {};
}

// What is wrong with the arms A of a node that follows a value of E, or nothing.
template <class E, class... A>
consteval std::string_view shown_problem() {
  if constexpr (!(is_tree_arm<A> && ...)) {
    return "lv::shown takes arms: lv::is<value>(tree), lv::is<value>() for nothing, and a last "
           "lv::otherwise(tree). A tree is a constexpr variable: constexpr auto home = lv::column(...);";
  } else {
    if (const auto p = arm_order_problem<A...>(); !p.empty()) return p;
    std::string_view wrong;
    ((wrong = wrong.empty() ? arm_type_problem<E>(std::type_identity<typename tree_arm_pattern<A>::type>{}) : wrong), ...);
    if (!wrong.empty()) return wrong;
    std::vector<E> covered;
    (add_arm_values(covered, std::type_identity<typename tree_arm_pattern<A>::type>{}), ...);
    return arm_coverage_problem(covered, (is_otherwise<typename tree_arm_pattern<A>::type> || ...));
  }
}

// A state or a computed that lv::shown follows: of an enum or a bool.
template <class S>
concept shown_source = dependency<S> && (std::is_enum_v<value_of<S>> || std::same_as<value_of<S>, bool>);

// ---- the focus -----------------------------------------------------------------------

// Whether `o` lies below `holder`.
inline bool lies_below(const lv_obj_t* o, const lv_obj_t* holder) {
  for (const lv_obj_t* p = o != nullptr ? lv_obj_get_parent(o) : nullptr; p != nullptr; p = lv_obj_get_parent(p))
    if (p == holder) return true;
  return false;
}

// The group whose focused widget lies below `holder`, or null.
inline lv_group_t* focus_below(const lv_obj_t* holder) {
  lv_group_t* g = lv_group_get_default();
  return g != nullptr && lies_below(lv_group_get_focused(g), holder) ? g : nullptr;
}

// Focuses the first widget of `g` below `root`, the root of a new tree.
inline void focus_first(lv_group_t* g, lv_obj_t* root) {
  const std::uint32_t count = lv_group_get_obj_count(g);
  for (std::uint32_t i = 0; i < count; ++i) {
    lv_obj_t* o = lv_group_get_obj_by_index(g, i);
    if (o == root || lies_below(o, root)) {
      lv_group_focus_obj(o);
      return;
    }
  }
}

// ---- pages from other files ----------------------------------------------------------

inline constexpr std::size_t no_arm = static_cast<std::size_t>(-1);

struct shown_slot;

}  // namespace detail

// A page whose tree is in a file of its own: what lv::page_of<ui> gives. The file of the page
// defines it, and every other file declares it:
//
//   // pages.hpp
//   extern const lv::page home_page;
//   // home_page.cpp
//   constinit const lv::page home_page = lv::page_of<ui>;  // or lv::page_of<ui, {.on_shown = &start_home}>
//   // shell.cpp
//   lv::shown(current)[lv::is<page::home>(home_page), ...]
//
// It is constant data: the size and alignment of the page's lv::mounted, and the functions that
// build and end it in memory that lv::shown gives. No code of the page runs before it is shown.
struct page {
  std::size_t size;
  std::size_t align;
  const void* tree;                                // the page's tree: what a handle checks
  bool (*mount)(void* memory, lv_obj_t* parent);  // false: LVGL had no memory; nothing is alive
  void (*end)(void* memory);                      // deletes its widgets
  void (*abandon)(void* memory);                  // leaves its widgets to LVGL, which deletes them next
  lv_obj_t* (*root)(const void* memory);
  void (*on_shown)(detail::shown_slot& slot);     // after it is built, or null
  void (*on_front)(detail::shown_slot& slot);     // each time it comes to the front, or null
};

namespace detail {

// Whether an arm shows a page of another file.
template <class A>
inline constexpr bool arm_shows_page = false;
template <class Pattern>
inline constexpr bool arm_shows_page<tree_arm<Pattern, page>> = true;

// One place for a page: its memory, which page is there, and what it is doing.
// A kept page is built and hidden: it was shown, and may come back without being built again.
enum class slot_phase : unsigned char { empty, shown, entering, leaving, kept };

struct shown_core;

struct shown_slot {
  std::byte* memory = nullptr;
  const page* holds = nullptr;  // null: empty
  std::size_t arm = no_arm;
  slot_phase phase = slot_phase::empty;
  shown_core* owner = nullptr;
  lv_anim_exec_xcb_t moving = nullptr;  // the setter that a transition of the page runs
  std::int32_t at = 0;                 // where that transition stands
  std::uint32_t in_front_at = 0;       // when it was last at the front: the kept page shown longest ago goes first
};

template <const auto& Tree>
bool page_mount(void* memory, lv_obj_t* parent) {
  auto* tree = ::new (memory) mounted<Tree>(obj{parent});
  if (tree->has_value()) return true;
  std::destroy_at(tree);
  return false;
}

template <const auto& Tree>
void page_end(void* memory) {
  std::destroy_at(static_cast<mounted<Tree>*>(memory));
}

template <const auto& Tree>
void page_abandon(void* memory) {
  auto& tree = *static_cast<mounted<Tree>*>(memory);
  mounted_access::abandon(tree);
  std::destroy_at(&tree);
}

template <const auto& Tree>
lv_obj_t* page_root(const void* memory) {
  return static_cast<const mounted<Tree>*>(memory)->root().raw();
}

// The tree in `slot`, if it is Tree and alive: what a handle asks at each use.
template <const auto& Tree>
mounted<Tree>* page_find(void* slot) {
  auto& s = *static_cast<shown_slot*>(slot);
  if (s.holds == nullptr || s.holds->tree != static_cast<const void*>(&Tree)) return nullptr;
  auto* tree = reinterpret_cast<mounted<Tree>*>(s.memory);
  return tree->has_value() ? tree : nullptr;
}

// Calls the hook H (.on_shown or .on_front) of Hooks with a handle to the page in `slot`.
template <const auto& Tree, arm_options Hooks, std::meta::info H>
void page_call(shown_slot& slot) {
  const auto handle = held_access::make<Tree>(&slot, &page_find<Tree>);
  Hooks.[:H:].call(&handle);
}

// The function of a page that calls hook H of Hooks, or null.
template <const auto& Tree, arm_options Hooks, std::meta::info H>
consteval void (*page_hook_of())(shown_slot&) {
  if constexpr (is_null(Hooks.[:H:].tree)) {
    return nullptr;
  } else {
    if (std::meta::reflect_constant(Hooks.[:H:].tree) != std::meta::reflect_constant(static_cast<const void*>(&Tree)))
      fail("lv::page_of<ui, {.on_shown = ..., .on_front = ...}>: the function of ." +
           str(std::meta::identifier_of(H)) + " takes the lv::held of another tree than the page's. Write void f(lv::held<" +
           str(std::meta::display_string_of(std::meta::reflect_object(Tree))) + ">).");
    return &page_call<Tree, Hooks, H>;
  }
}

template <const auto& Tree, arm_options Hooks>
consteval page page_for() {
  return {sizeof(mounted<Tree>),
          alignof(mounted<Tree>),
          &Tree,
          &page_mount<Tree>,
          &page_end<Tree>,
          &page_abandon<Tree>,
          &page_root<Tree>,
          page_hook_of<Tree, Hooks, ^^arm_options::on_shown>(),
          page_hook_of<Tree, Hooks, ^^arm_options::on_front>()};
}

}  // namespace detail

// The page of the tree `Tree`, for lv::page: constinit const lv::page home_page = lv::page_of<ui>;
// Its hooks are named as those of an arm: lv::page_of<ui, {.on_shown = &start, .on_front = &look}>.
template <const auto& Tree, detail::arm_options Hooks = {}>
  requires tree_node<decltype(Tree)>
inline constexpr page page_of = detail::page_for<Tree, Hooks>();

namespace detail {

// ---- transitions ---------------------------------------------------------------------

// How a page enters or leaves: one property of its root, from one value to another.
struct transition_spec {
  lv_anim_exec_xcb_t step = nullptr;  // null: none
  std::int32_t from = 0;
  std::int32_t to = 0;
  std::uint32_t ms = 0;
  easing how{};
};

// What lv::shown takes beside its state: lv::shown(current, {.enter = slide_in, .leave = slide_out}).
struct shown_options {
  // How many pages may be built at once: the one shown and the one that leaves, and above 2 the
  // pages shown most recently, kept hidden so that they come back without being built again.
  std::size_t alive = 2;
  // How the new page comes in, and how the old one goes. Without them the old page is deleted
  // before the new one is built.
  transition_spec enter{};
  transition_spec leave{};
};

}  // namespace detail

namespace detail {

// The members of the animatable properties of a page's root, and their setters. Through D, so that
// the scan runs where a transition is written, not in every file that includes this header.
template <class D>
inline constexpr auto transition_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^lazy_animatable<"obj", D>, std::meta::access_context::unchecked()));
template <class D>
inline constexpr std::span<const std::meta::info> transition_setters =
    std::define_static_array(animatable_setters<D>("obj"));
// For each member: 1 when it is a style property of the main part (`.opa`, `.translate_x`), moved
// by its style setter, and 0 when it is a setter of the widget.
template <class D>
inline constexpr std::span<const int> transition_styles = std::define_static_array(animatable_styles<D>("obj"));

}  // namespace detail

// How a page of lv::shown enters or leaves: the arguments of lv::animate, for the root of the page.
//   constexpr auto slide_in = lv::transition({.x = {320 * px, 0 * px}}, tok.motion.slide_time, tok.motion.slide);
template <class D = void>
constexpr detail::transition_spec transition(const detail::lazy_animatable<"obj", D>& what, duration time,
                                             const easing& how = {}) {
  std::size_t properties = 0;
  detail::transition_spec t{.ms = static_cast<std::uint32_t>(time.ms), .how = how};
  template for (constexpr auto i : detail::indices<detail::transition_members<D>.size()>) {
    const auto& ends = what.[:detail::transition_members<D>[i]:];
    if (ends.is_set()) {
      ++properties;
      if constexpr (detail::transition_styles<D>[i] != 0)
        t.step = &detail::style_step<detail::transition_setters<D>[i], 0>;
      else
        t.step = &detail::animation_step<detail::transition_setters<D>[i]>;
      t.from = detail::number_as<std::int32_t>(detail::lower(ends.from()));
      t.to = detail::number_as<std::int32_t>(detail::lower(ends.to()));
    }
  }
  contract_assert(properties == 1);  // one property; GCC 16.2 refuses pre() with a fold here
  detail::keep_checked(properties == 1);
  return t;
}

namespace detail {

// ---- the engine: what every lv::shown runs, no template ---------------------------------

// What one lv::shown knows at compile time, for each arm: the page it shows, or null, and how
// .on_shown is called.
struct shown_plan {
  const page* const* pages;
  void (*const* shown_hooks)(shown_slot&);  // .on_shown of each arm, or null
  void (*const* front_hooks)(shown_slot&);  // .on_front of each arm, or null
  std::size_t arms;
  std::size_t slot_size;   // 0: the pages are in other files, and the size is found at mount
  std::size_t slot_align;
  transition_spec enter;
  transition_spec leave;
  std::size_t (*arm_now)(const void* source);  // the arm of the state's value
  const void* source;
};

// The run-time part of one lv::shown, inside its storage.
struct shown_core : watch_node, deferred_node {
  const shown_plan* plan = nullptr;
  lv_obj_t* holder = nullptr;
  lv_obj_t* glass = nullptr;  // over a leaving page, so that it takes no clicks
  std::span<shown_slot> slots;  // .alive of them, in the storage
  std::byte* block = nullptr;   // the memory of the slots from LVGL's heap, for pages of other files
  std::size_t slot_size = 0;
  std::size_t current = no_arm;  // the arm asked for last, also one that shows nothing
  std::uint32_t clock = 0;       // counts the pages that come to the front
};

// How many pages may stay built and hidden: the slots that the page in front and a leaving page
// do not need.
inline std::size_t keep_room(const shown_core& c) { return c.slots.size() > 2 ? c.slots.size() - 2 : 0; }

inline bool has_transitions(const shown_plan& p) { return p.enter.step != nullptr || p.leave.step != nullptr; }

// The slot of the page that is shown or comes in, or null.
inline shown_slot* front_slot(shown_core& c) {
  for (auto& s : c.slots)
    if (s.phase == slot_phase::shown || s.phase == slot_phase::entering) return &s;
  return nullptr;
}

inline shown_slot* slot_in(shown_core& c, slot_phase p) {
  for (auto& s : c.slots)
    if (s.phase == p) return &s;
  return nullptr;
}

// The kept page of arm `k`, or null.
inline shown_slot* kept_slot(shown_core& c, std::size_t k) {
  for (auto& s : c.slots)
    if (s.phase == slot_phase::kept && s.arm == k) return &s;
  return nullptr;
}

// The kept page that was at the front longest ago, or null.
inline shown_slot* oldest_kept(shown_core& c) {
  shown_slot* oldest = nullptr;
  for (auto& s : c.slots)
    if (s.phase == slot_phase::kept && (oldest == nullptr || s.in_front_at < oldest->in_front_at)) oldest = &s;
  return oldest;
}

inline std::size_t kept_count(const shown_core& c) {
  std::size_t n = 0;
  for (const auto& s : c.slots) n += s.phase == slot_phase::kept ? 1 : 0;
  return n;
}

// The glass lies over the leaving pages and under the others, while a page leaves.
inline void place_glass(shown_core& c) {
  if (c.glass == nullptr) return;
  shown_slot* leaving = slot_in(c, slot_phase::leaving);
  if (leaving == nullptr) {
    lv_obj_set_hidden(c.glass, true);
    return;
  }
  lv_obj_move_to_index(leaving->holds->root(leaving->memory), 0);
  lv_obj_move_to_index(c.glass, 1);
  lv_obj_set_hidden(c.glass, false);
}

// Each step of a transition: the property of the page's root, and where it stands now.
inline void transition_step(lv_anim_t* a, std::int32_t value) {
  auto& s = *static_cast<shown_slot*>(lv_anim_get_user_data(a));
  s.at = value;
  s.moving(s.holds->root(s.memory), value);
}

// What names a transition of a page to LVGL: LVGL finds an animation by its widget and its plain
// step, and the step of a transition is the custom one above. This one does nothing.
inline void transition_tag(void*, std::int32_t) {}

// Stops the transition of the page in `s`, where it stands.
inline void stop_transition(shown_slot& s) {
  lv_obj_t* root = s.holds != nullptr ? s.holds->root(s.memory) : nullptr;
  if (root != nullptr) lv_anim_delete(root, &transition_tag);
}

// Ends the page of a slot, deleting its widgets.
inline void end_page(shown_slot& s) {
  if (s.holds == nullptr) return;
  stop_transition(s);
  s.holds->end(s.memory);
  s.holds = nullptr;
  s.arm = no_arm;
  s.phase = slot_phase::empty;
}

// A page that is no longer at the front: kept and hidden while there is room for kept pages, the
// one at the front longest ago dropped when there is not; else its widgets go.
inline void retire(shown_slot& s) {
  shown_core& c = *s.owner;
  if (keep_room(c) == 0) {
    end_page(s);
    return;
  }
  stop_transition(s);
  lv_obj_set_hidden(s.holds->root(s.memory), true);
  s.phase = slot_phase::kept;
  while (kept_count(c) > keep_room(c)) end_page(*oldest_kept(c));
}

// A slot for a new page: an empty one, or the one of the kept page at the front longest ago.
inline shown_slot* free_slot(shown_core& c) {
  for (auto& s : c.slots)
    if (s.phase == slot_phase::empty) return &s;
  shown_slot* oldest = oldest_kept(c);
  if (oldest != nullptr) end_page(*oldest);
  return oldest;
}

void transition_done(lv_anim_t* a);

// The time of the part of `t` that is left when it starts at `from` instead of at its start.
inline std::uint32_t part_of(const transition_spec& t, std::int32_t from) {
  const std::int64_t whole = std::int64_t{t.to} - t.from;
  const std::int64_t left = std::int64_t{t.to} - from;
  if (whole == 0) return t.ms;
  if (left * whole <= 0) return 0;
  return static_cast<std::uint32_t>(std::min<std::int64_t>(t.ms, std::int64_t{t.ms} * left / whole));
}

// Runs `t` on the root of the page in `s`, from `from`: its own start, or where the page stands.
inline void run_transition(shown_slot& s, const transition_spec& t, std::int32_t from) {
  lv_obj_t* root = s.holds->root(s.memory);
  stop_transition(s);
  s.moving = t.step;
  s.at = from;
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, root);
  lv_anim_set_exec_cb(&a, &transition_tag);
  lv_anim_set_custom_exec_cb(&a, &transition_step);
  lv_anim_set_values(&a, from, t.to);
  lv_anim_set_duration(&a, part_of(t, from));
  t.how.apply(&a);
  lv_anim_set_completed_cb(&a, &transition_done);
  lv_anim_set_user_data(&a, &s);
  if (lv_anim_start(&a) != nullptr) return;
  // Without memory for the animation the page takes its end at once.
  t.step(root, t.to);
  if (s.phase == slot_phase::leaving)
    retire(s);
  else
    s.phase = slot_phase::shown;
}

// Where a page that turns round starts: where it stands if the two transitions move the same
// property, else at the start of the new one.
inline std::int32_t turning_from(const shown_slot& s, const transition_spec& next) {
  return s.moving == next.step ? s.at : next.from;
}

inline void start_leave(shown_slot& s) {
  const shown_plan& p = *s.owner->plan;
  const std::int32_t from = s.phase == slot_phase::entering ? turning_from(s, p.leave) : p.leave.from;
  s.phase = slot_phase::leaving;
  if (p.leave.step != nullptr)
    run_transition(s, p.leave, from);
  else
    retire(s);
}

inline void start_enter(shown_slot& s, bool backwards) {
  const shown_plan& p = *s.owner->plan;
  const std::int32_t from = backwards ? turning_from(s, p.enter) : p.enter.from;
  s.phase = slot_phase::entering;
  if (p.enter.step != nullptr)
    run_transition(s, p.enter, from);
  else
    s.phase = slot_phase::shown;
}

inline void transition_done(lv_anim_t* a) {
  auto& s = *static_cast<shown_slot*>(lv_anim_get_user_data(a));
  if (s.phase == slot_phase::entering) {
    s.phase = slot_phase::shown;
  } else if (s.phase == slot_phase::leaving) {
    retire(s);
    place_glass(*s.owner);
  }
}

// Builds the page of arm `k` in the empty slot `s`. False when LVGL had no memory.
inline bool build_page(shown_core& c, shown_slot& s, std::size_t k) {
  const page* p = c.plan->pages[k];
  if (!p->mount(s.memory, c.holder)) return false;
  s.holds = p;
  s.arm = k;
  s.owner = &c;
  return true;
}

// .on_shown and .on_front of a page of another file, which the page carries.
inline void page_shown_hook(shown_slot& s) {
  if (s.holds->on_shown != nullptr) s.holds->on_shown(s);
}
inline void page_front_hook(shown_slot& s) {
  if (s.holds->on_front != nullptr) s.holds->on_front(s);
}

// A page comes to the front: its .on_front.
inline void to_front(shown_core& c, shown_slot& s) {
  s.in_front_at = ++c.clock;
  if (c.plan->front_hooks[s.arm] != nullptr) c.plan->front_hooks[s.arm](s);
}

// A kept page comes back to the front: shown again, not built again.
inline void reveal(shown_core& c, shown_slot& s, lv_group_t* focus) {
  lv_obj_set_hidden(s.holds->root(s.memory), false);
  start_enter(s, false);
  if (focus != nullptr) focus_first(focus, s.holds->root(s.memory));
  to_front(c, s);
}

// After a page is built: the focus if the old page had it, its .on_shown, then its .on_front.
inline void welcome(shown_core& c, shown_slot& s, lv_group_t* focus) {
  if (focus != nullptr) focus_first(focus, s.holds->root(s.memory));
  if (c.plan->shown_hooks[s.arm] != nullptr) c.plan->shown_hooks[s.arm](s);
  to_front(c, s);
}

// The old page is deleted before the new one is built: the heap holds one page at a time.
inline bool switch_at_once(shown_core& c, std::size_t k, lv_group_t* focus) {
  shown_slot* old = front_slot(c);
  // A kept page comes back first, so that the old page, kept in its turn, cannot push it out.
  if (shown_slot* kept = kept_slot(c, k)) {
    reveal(c, *kept, focus);
    if (old != nullptr) retire(*old);
    return true;
  }
  if (old != nullptr) retire(*old);
  if (c.plan->pages[k] == nullptr) return true;
  shown_slot& s = *free_slot(c);
  if (!build_page(c, s, k)) return false;
  s.phase = slot_phase::shown;
  welcome(c, s, focus);
  return true;
}

// A page asked for again while it leaves plays its leave backwards, and the page that came in
// leaves in its place.
inline bool come_back(shown_core& c, shown_slot& back) {
  if (shown_slot* front = front_slot(c)) start_leave(*front);
  start_enter(back, true);
  place_glass(c);
  to_front(c, back);
  return true;
}

// Brings the page of arm `k` in: a kept one comes back, any other is built in a free slot.
inline bool bring_in(shown_core& c, std::size_t k, lv_group_t* focus) {
  if (shown_slot* kept = kept_slot(c, k)) {
    reveal(c, *kept, focus);
    return true;
  }
  shown_slot& s = *free_slot(c);
  if (!build_page(c, s, k)) return false;
  start_enter(s, false);
  welcome(c, s, focus);
  return true;
}

// The new page is built in a free slot before the old one leaves. A page that still leaves
// is cut, so that two pages at most are alive.
inline bool switch_with_transition(shown_core& c, std::size_t k, lv_group_t* focus) {
  if (shown_slot* leaving = slot_in(c, slot_phase::leaving)) {
    if (leaving->arm == k) return come_back(c, *leaving);
    // Cut: kept if there is room, without pushing out the page that is asked for.
    if (shown_slot* kept = kept_slot(c, k)) kept->in_front_at = ++c.clock;
    retire(*leaving);
  }
  shown_slot* front = front_slot(c);
  if (front != nullptr) start_leave(*front);
  const bool ok = c.plan->pages[k] == nullptr || bring_in(c, k, focus);
  place_glass(c);
  return ok;
}

// Shows arm `k`. False when LVGL had no memory for its page; then nothing new is shown.
inline bool show_arm(shown_core& c, std::size_t k) {
  if (c.current == k) return true;
  c.current = k;
  lv_group_t* focus = focus_below(c.holder);
  return has_transitions(*c.plan) ? switch_with_transition(c, k, focus) : switch_at_once(c, k, focus);
}

inline void shown_changed(watch_node& w) {
  auto& c = static_cast<shown_core&>(w);
  if (c.plan->arm_now(c.plan->source) != c.current) defer(c, c.holder);
}

inline void shown_run(deferred_node& d) {
  auto& c = static_cast<shown_core&>(d);
  static_cast<void>(show_arm(c, c.plan->arm_now(c.plan->source)));
}

// The memory of the slots: in the storage when the compiler knows every page's size, else one
// block of LVGL's heap of two slots of the largest page. False when LVGL had none.
inline bool give_slots(shown_core& c, std::span<shown_slot> slots, std::byte* room) {
  c.slots = slots;
  std::size_t size = c.plan->slot_size;
  std::size_t align = c.plan->slot_align;
  if (size == 0) {
    for (std::size_t k = 0; k < c.plan->arms; ++k)
      if (const page* p = c.plan->pages[k]; p != nullptr) {
        size = std::max(size, p->size);
        align = std::max(align, p->align);
      }
    size = (size + align - 1) / align * align;
    c.block = static_cast<std::byte*>(lv_malloc(c.slots.size() * size + align));
    if (c.block == nullptr) return false;
    const auto at = reinterpret_cast<std::uintptr_t>(c.block);
    room = c.block + (align - at % align) % align;
  }
  for (std::size_t i = 0; i < c.slots.size(); ++i) {
    c.slots[i].memory = room + i * size;
    c.slots[i].owner = &c;
  }
  c.slot_size = size;
  return true;
}

// A transparent widget that catches the clicks over a leaving page.
inline bool make_glass(shown_core& c) {
  if (c.plan->leave.step == nullptr) return true;
  c.glass = lv_obj_create(c.holder);
  if (c.glass == nullptr) return false;
  lv_obj_remove_style_all(c.glass);
  lv_obj_set_size(c.glass, lv_pct(100), lv_pct(100));
  lv_obj_set_hidden(c.glass, true);
  return true;
}

// Starts the node: its slots, its glass, the page of the state's value, and its watcher.
inline bool start_shown(shown_core& c, std::span<shown_slot> slots, std::byte* room, watch_list& watchers) {
  if (!give_slots(c, slots, room) || !make_glass(c)) return false;
  c.changed = &shown_changed;
  c.run = &shown_run;
  watchers.append(c);
  const std::size_t k = c.plan->arm_now(c.plan->source);
  c.current = k;
  if (c.plan->pages[k] == nullptr) return true;
  shown_slot& s = c.slots[0];
  if (!build_page(c, s, k)) return false;
  s.phase = slot_phase::shown;
  welcome(c, s, nullptr);
  return true;
}

// The node's storage ends while its widgets still exist; LVGL deletes them next, with the holder.
inline void stop_shown(shown_core& c) noexcept {
  watch_list::remove(c);
  deferred().remove(c);
  for (auto& s : c.slots) {
    if (s.holds == nullptr) continue;
    stop_transition(s);
    s.holds->abandon(s.memory);
    s.holds = nullptr;
  }
  lv_free(c.block);
  c.block = nullptr;
}

// ---- the node ------------------------------------------------------------------------

template <class S, class... Arms>
struct shown_node {
  using value_type = value_of<S>;
  static constexpr std::size_t arm_count = sizeof...(Arms);

  const S* source;
  shown_options options{};
  kids<Arms...> arms{};

  // The arms: lv::shown(state)[lv::is<a>(tree), lv::is<b>(), lv::otherwise(other)]. Variadic,
  // so that two arms are not read as one comma expression.
  template <class... A>
    requires(sizeof...(Arms) == 0)
  constexpr shown_node<S, A...> operator[](const A&... a) const {
    static_assert(shown_problem<value_type, A...>().empty(), shown_problem<value_type, A...>());
    return {source, options, {a...}};
  }

  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static constexpr const auto& arm() {
    return kid<K>(descend<Path...>(Tree).arms);
  }

  template <std::size_t K, const auto& Tree, std::size_t... Path>
  using arm_type = std::remove_cvref_t<decltype(arm<K, Tree, Path...>())>;

  // Whether arm K shows a tree of this file, or a page of another file.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval bool has_tree() {
    return arm_shows_tree<arm_type<K, Tree, Path...>> && !arm_shows_page<arm_type<K, Tree, Path...>>;
  }
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval bool has_page() {
    return arm_shows_page<arm_type<K, Tree, Path...>>;
  }

  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval std::meta::info tree_of() {
    return std::meta::reflect_object(*arm<K, Tree, Path...>().tree);
  }

  // A function of .on_shown or .on_front (Hook) that takes another tree than its arm shows, as an
  // error, or nothing.
  template <std::size_t K, auto Hook, const auto& Tree, std::size_t... Path>
  static consteval std::string one_hook_problem(std::string_view name) {
    constexpr const auto& hook = arm<K, Tree, Path...>().options.[:Hook:];
    if constexpr (!is_null(hook.tree)) {
      if constexpr (has_page<K, Tree, Path...>())
        return "lv::shown: a page of another file takes its " + str(name) +
               " in its own file: constinit const lv::page home_page = lv::page_of<ui, {" + str(name) + " = &f}>;";
      if (std::meta::reflect_constant(hook.tree) !=
          std::meta::reflect_constant(static_cast<const void*>(arm<K, Tree, Path...>().tree)))
        return "lv::shown: the function of " + str(name) + " takes the lv::held of another tree than its arm shows. " +
               "Write void f(lv::held<" + str(std::meta::display_string_of(tree_of<K, Tree, Path...>())) + ">).";
    }
    return {};
  }

  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval std::string hook_problem() {
    const auto shown = one_hook_problem<K, ^^arm_options::on_shown, Tree, Path...>(".on_shown");
    return shown.empty() ? one_hook_problem<K, ^^arm_options::on_front, Tree, Path...>(".on_front") : shown;
  }

  // A tree shown by two arms, or a function of .on_shown for another tree, as an error, or nothing.
  template <const auto& Tree, std::size_t... Path>
  static consteval std::string_view tree_problem() {
    std::vector<std::meta::info> seen;
    std::string problem;
    template for (constexpr auto k : indices<arm_count>) {
      if constexpr (has_tree<k, Tree, Path...>() || has_page<k, Tree, Path...>()) {
        constexpr auto t = tree_of<k, Tree, Path...>();
        if (problem.empty() && std::ranges::contains(seen, t))
          problem = "lv::shown shows the tree " + str(std::meta::display_string_of(t)) +
                    " in two arms. Name both values in one arm: lv::is<a, b>(" + str(std::meta::display_string_of(t)) + ").";
        seen.push_back(t);
        if (problem.empty()) problem = hook_problem<k, Tree, Path...>();
      }
    }
    return problem.empty() ? std::string_view{} : std::string_view{std::define_static_string(problem)};
  }

  template <const auto& Tree, std::size_t... Path>
  static consteval std::string_view options_problem() {
    constexpr const auto& o = descend<Path...>(Tree).options;
    if (o.alive == 0) return "lv::shown needs a slot for the page it shows: .alive = 1 or more.";
    if (o.alive == 1 && (!is_null(o.enter.step) || !is_null(o.leave.step)))
      return "lv::shown: a page that enters or leaves is alive beside the other one, so .enter and .leave need .alive = 2.";
    return {};
  }

  // The page of arm K: a tree of this file as a page, or the page of another file.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval const page* page_at() {
    if constexpr (has_page<K, Tree, Path...>())
      return arm<K, Tree, Path...>().tree;
    else if constexpr (has_tree<K, Tree, Path...>())
      return &page_of<*arm<K, Tree, Path...>().tree>;
    else
      return nullptr;
  }

  // .on_shown or .on_front (Hook) of a tree of this file, given in its arm.
  template <std::size_t K, auto Hook, const auto& Tree, std::size_t... Path>
  static void call_hook(shown_slot& s) {
    constexpr auto& T = *arm<K, Tree, Path...>().tree;
    const auto handle = held_access::make<T>(&s, &page_find<T>);
    arm<K, Tree, Path...>().options.[:Hook:].call(&handle);
  }

  // The function that calls Hook of arm K: the page's own for a page of another file, read when it
  // is shown since its value is in that file; the arm's for a tree of this file; or null.
  template <std::size_t K, auto Hook, const auto& Tree, std::size_t... Path>
  static consteval void (*hook_at())(shown_slot&) {
    if constexpr (has_page<K, Tree, Path...>())
      return Hook == ^^arm_options::on_shown ? &page_shown_hook : &page_front_hook;
    else if constexpr (has_tree<K, Tree, Path...>()) {
      if constexpr (!is_null(arm<K, Tree, Path...>().options.[:Hook:].tree)) return &call_hook<K, Hook, Tree, Path...>;
    }
    return nullptr;
  }

  // The largest page of this file, when every page with a tree is of this file; else 0.
  template <const auto& Tree, std::size_t... Path>
  static consteval std::array<std::size_t, 2> slot_of() {
    std::size_t size = 0;
    std::size_t align = 1;
    bool other_file = false;
    template for (constexpr auto k : indices<arm_count>) {
      if constexpr (has_page<k, Tree, Path...>()) other_file = true;
      if constexpr (has_tree<k, Tree, Path...>()) {
        size = std::max(size, page_of<*arm<k, Tree, Path...>().tree>.size);
        align = std::max(align, page_of<*arm<k, Tree, Path...>().tree>.align);
      }
    }
    if (other_file) return {0, 1};
    return {(size + align - 1) / align * align, align};
  }

  template <auto... Values>
  static bool matches(value_type v, std::type_identity<one_of<Values...>>) {
    return ((v == static_cast<value_type>(Values)) || ...);
  }
  static bool matches(value_type, std::type_identity<anything>) { return true; }

  // The arm of the value of the state.
  template <const auto& Tree, std::size_t... Path>
  static std::size_t arm_now(const void* source) {
    const value_type v = static_cast<const S*>(source)->get();
    std::size_t found = no_arm;
    template for (constexpr auto k : indices<arm_count>) {
      using pattern = typename tree_arm_pattern<arm_type<k, Tree, Path...>>::type;
      if (found == no_arm && matches(v, std::type_identity<pattern>{})) found = k;
    }
    return found;
  }

  template <const auto& Tree, std::size_t... Path>
  struct parts {
    static constexpr std::array<const page*, arm_count> pages = []<std::size_t... K>(std::index_sequence<K...>) {
      return std::array<const page*, arm_count>{page_at<K, Tree, Path...>()...};
    }(std::make_index_sequence<arm_count>{});
    static constexpr std::array<void (*)(shown_slot&), arm_count> shown_hooks = []<std::size_t... K>(std::index_sequence<K...>) {
      return std::array<void (*)(shown_slot&), arm_count>{hook_at<K, ^^arm_options::on_shown, Tree, Path...>()...};
    }(std::make_index_sequence<arm_count>{});
    static constexpr std::array<void (*)(shown_slot&), arm_count> front_hooks = []<std::size_t... K>(std::index_sequence<K...>) {
      return std::array<void (*)(shown_slot&), arm_count>{hook_at<K, ^^arm_options::on_front, Tree, Path...>()...};
    }(std::make_index_sequence<arm_count>{});
    static constexpr auto slot = slot_of<Tree, Path...>();
    static constexpr shown_plan plan{pages.data(),
                                     shown_hooks.data(),
                                     front_hooks.data(),
                                     arm_count,
                                     slot[0],
                                     slot[1],
                                     descend<Path...>(Tree).options.enter,
                                     descend<Path...>(Tree).options.leave,
                                     &arm_now<Tree, Path...>,
                                     descend<Path...>(Tree).source};
  };

  // What one lv::shown keeps while its widget exists: see shown_core. Its slots are here when
  // the compiler knows the size of every page; else they come from LVGL's heap at mount.
  template <const auto& Tree, std::size_t... Path>
  struct storage : shown_core {
    static constexpr std::size_t alive = descend<Path...>(Tree).options.alive;
    std::array<shown_slot, alive> places{};
    alignas(parts<Tree, Path...>::slot[1]) std::byte room[std::max<std::size_t>(1, parts<Tree, Path...>::slot[0] * alive)];

    storage() = default;
    storage(const storage&) = delete;
    storage& operator=(const storage&) = delete;
    ~storage() { stop_shown(*this); }
  };

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, build_state& st) {
    static_assert(options_problem<Tree, Path...>().empty(), options_problem<Tree, Path...>());
    static_assert(tree_problem<Tree, Path...>().empty(), tree_problem<Tree, Path...>());
    auto& s = storage_of<Tree, Path...>(st);
    s.plan = &parts<Tree, Path...>::plan;
    s.holder = parent;
    if (!start_shown(s, s.places, s.room, graph_access::watchers(*descend<Path...>(Tree).source))) st.ok = false;
    return parent;
  }
};

// lv::when(flag)[tree]: the tree while the flag is true, nothing while it is false.
template <class S>
struct when_node {
  const S* source;
  shown_options options{};

  template <class T>
    requires tree_node<T> || std::same_as<T, page>
  constexpr auto operator[](const T& tree) const {
    using shows = tree_arm<one_of<true>, T>;
    using hides = tree_arm<one_of<false>, void>;
    return shown_node<S, shows, hides>{source, options, {shows{&tree, {}}, hides{}}};
  }
};

}  // namespace detail

template <class S, class... Arms>
inline constexpr bool detail::is_structural<detail::shown_node<S, Arms...>> = true;
template <class S, class... Arms>
inline constexpr bool detail::sole_child<detail::shown_node<S, Arms...>> = true;

// Shows the arm of the value of `state`: lv::shown(current)[lv::is<page::home>(home_ui), ...].
// With transitions: lv::shown(current, {.enter = slide_in, .leave = slide_out})[...]. See the
// top of this file.
template <detail::shown_source S>
constexpr detail::shown_node<S> shown(const S& state, const detail::shown_options& options = {}) {
  return {&state, options};
}
// The arms as further arguments: lv::shown(current, lv::is<page::home>(home_ui), ...), with the
// transitions in braces before them: lv::shown(current, {.enter = slide_in}, lv::is<...>(...), ...).
template <detail::shown_source S, class... A>
  requires detail::children_given<A...>
constexpr auto shown(const S& state, const detail::shown_options& options, const A&... arms) {
  using value_type = detail::value_of<S>;
  static_assert(detail::shown_problem<value_type, A...>().empty(), detail::shown_problem<value_type, A...>());
  return detail::shown_node<S, A...>{&state, options, {arms...}};
}
template <detail::shown_source S, class... A>
  requires detail::children_given<A...>
constexpr auto shown(const S& state, const A&... arms) {
  using value_type = detail::value_of<S>;
  static_assert(detail::shown_problem<value_type, A...>().empty(), detail::shown_problem<value_type, A...>());
  return detail::shown_node<S, A...>{&state, {}, {arms...}};
}
template <class S>
void shown(const S&, const detail::shown_options& = {}) = delete(
    "lv::shown follows an lv::state or an lv::computed of an enum or a bool: lv::shown(current, ...). "
    "For a number or a text, make an lv::computed of an enum from it.");
template <class S, class... A>
  requires(detail::children_given<A...> && !detail::shown_source<S>)
void shown(const S&, const A&...) = delete(
    "lv::shown follows an lv::state or an lv::computed of an enum or a bool: lv::shown(current, ...). "
    "For a number or a text, make an lv::computed of an enum from it.");

// Shows `tree` while `flag` is true: lv::when(online)[online_ui].
template <detail::shown_source S>
  requires std::same_as<detail::value_of<S>, bool>
constexpr detail::when_node<S> when(const S& flag, const detail::shown_options& options = {}) {
  return {&flag, options};
}
// The tree as the second argument: lv::when(online, online_ui).
template <detail::shown_source S, class T>
  requires std::same_as<detail::value_of<S>, bool> && (tree_node<T> || std::same_as<T, page>)
constexpr auto when(const S& flag, const T& tree) {
  using shows = detail::tree_arm<detail::one_of<true>, T>;
  using hides = detail::tree_arm<detail::one_of<false>, void>;
  return detail::shown_node<S, shows, hides>{&flag, {}, {shows{&tree, {}}, hides{}}};
}
template <class S>
void when(const S&, const detail::shown_options& = {}) = delete(
    "lv::when follows an lv::state or an lv::computed of a bool: lv::when(online, tree).");
template <class S, class T>
void when(const S&, const T&) = delete(
    "lv::when follows an lv::state or an lv::computed of a bool, and shows one tree: lv::when(online, online_ui).");

}  // namespace lv
