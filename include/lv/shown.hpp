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
#include <lv/computed.hpp>
#include <lv/detail/deferred.hpp>
#include <lv/detail/one_of_region.hpp>
#include <lv/detail/watch.hpp>
#include <lv/match.hpp>
#include <lv/one_of.hpp>
#include <lv/state.hpp>
#include <lv/tree.hpp>

#include <lvgl/lvgl.h>

#include <algorithm>
#include <concepts>
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
};

// What an arm takes beside its tree: lv::is<page::home>(home_ui, {.on_shown = &start}).
struct arm_options {
  // Called after the tree is built, with a handle to it: its ids, a task tied to its root.
  shown_hook on_shown;
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

// ---- the node ------------------------------------------------------------------------

inline constexpr std::size_t no_arm = static_cast<std::size_t>(-1);

template <class S, class... Arms>
struct shown_node {
  using value_type = value_of<S>;
  static constexpr std::size_t arm_count = sizeof...(Arms);

  const S* source;
  kids<Arms...> arms{};

  // The arms: lv::shown(state)[lv::is<a>(tree), lv::is<b>(), lv::otherwise(other)]. Variadic,
  // so that two arms are not read as one comma expression.
  template <class... A>
    requires(sizeof...(Arms) == 0)
  constexpr shown_node<S, A...> operator[](const A&... a) const {
    static_assert(shown_problem<value_type, A...>().empty(), shown_problem<value_type, A...>());
    return {source, {a...}};
  }

  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static constexpr const auto& arm() {
    return kid<K>(descend<Path...>(Tree).arms);
  }

  // Whether arm K shows a tree.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval bool has_tree() {
    return arm_shows_tree<std::remove_cvref_t<decltype(arm<K, Tree, Path...>())>>;
  }

  // The lv::mounted of arm K, which must show a tree.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval std::meta::info mounted_of() {
    return std::meta::substitute(^^mounted, {std::meta::reflect_object(*arm<K, Tree, Path...>().tree)});
  }

  // The place of arm K in the region: the arms that show a tree, in order.
  template <const auto& Tree, std::size_t... Path>
  static consteval std::array<std::size_t, arm_count> places() {
    std::array<std::size_t, arm_count> out{};
    std::size_t next = 0;
    template for (constexpr auto k : indices<arm_count>) {
      out[k] = has_tree<k, Tree, Path...>() ? next++ : no_arm;
    }
    return out;
  }

  template <const auto& Tree, std::size_t... Path>
  static consteval std::meta::info region_of() {
    std::vector<std::meta::info> trees;
    template for (constexpr auto k : indices<arm_count>) {
      if constexpr (has_tree<k, Tree, Path...>()) trees.push_back(mounted_of<k, Tree, Path...>());
    }
    return std::meta::substitute(^^one_of_region, trees);
  }

  // The tree of arm K, as a reflection.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval std::meta::info tree_of() {
    return std::meta::reflect_object(*arm<K, Tree, Path...>().tree);
  }

  // A function of .on_shown that takes another tree than its arm shows, as an error, or nothing.
  template <std::size_t K, const auto& Tree, std::size_t... Path>
  static consteval std::string hook_problem() {
    constexpr const auto& hook = arm<K, Tree, Path...>().options.on_shown;
    if constexpr (!is_null(hook.tree)) {
      if (std::meta::reflect_constant(hook.tree) !=
          std::meta::reflect_constant(static_cast<const void*>(arm<K, Tree, Path...>().tree)))
        return "lv::shown: the function of .on_shown takes the lv::held of another tree than its arm shows. "
               "Write void f(lv::held<" + str(std::meta::display_string_of(tree_of<K, Tree, Path...>())) + ">).";
    }
    return {};
  }

  // A tree shown by two arms, or a function of .on_shown for another tree, as an error, or nothing.
  template <const auto& Tree, std::size_t... Path>
  static consteval std::string_view tree_problem() {
    std::vector<std::meta::info> seen;
    std::string problem;
    template for (constexpr auto k : indices<arm_count>) {
      if constexpr (has_tree<k, Tree, Path...>()) {
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

  // What one lv::shown keeps while its widget exists: its place in the state's list and in the
  // queue of waiting work, its widget, the arm it shows, and the tree of that arm.
  template <const auto& Tree, std::size_t... Path>
  struct storage : watch_node, deferred_node {
    static constexpr auto place = places<Tree, Path...>();

    lv_obj_t* holder = nullptr;
    std::size_t shown = no_arm;
    typename[:region_of<Tree, Path...>():] region;

    storage() = default;
    storage(const storage&) = delete;
    storage& operator=(const storage&) = delete;

    // The widgets are still there; LVGL deletes them next, with the holder. The tree of the arm
    // lets go of them without deleting them.
    ~storage() {
      watch_list::remove(*this);
      deferred().remove(*this);
      template for (constexpr auto k : indices<arm_count>) {
        if constexpr (place[k] != no_arm)
          if (region.index() == place[k]) mounted_access::abandon(region.template get<place[k]>());
      }
    }

    // The tree of arm K, if it is mounted: what a handle asks at each use.
    template <std::size_t K>
    static auto find(void* self) noexcept {
      auto& s = *static_cast<storage*>(self);
      auto& tree = s.region.template get<place[K]>();
      return s.region.index() == place[K] && tree.has_value() ? &tree : nullptr;
    }
  };

  // The arm of the value `v`.
  template <const auto& Tree, std::size_t... Path>
  static std::size_t arm_of(value_type v) {
    std::size_t found = no_arm;
    template for (constexpr auto k : indices<arm_count>) {
      using pattern = typename tree_arm_pattern<std::remove_cvref_t<decltype(arm<k, Tree, Path...>())>>::type;
      if (found == no_arm && matches(v, std::type_identity<pattern>{})) found = k;
    }
    return found;
  }

  template <auto... Values>
  static bool matches(value_type v, std::type_identity<one_of<Values...>>) {
    return ((v == static_cast<value_type>(Values)) || ...);
  }
  static bool matches(value_type, std::type_identity<anything>) { return true; }

  // Shows arm K in `s`: the old tree goes first, then the new one is built. False when LVGL had
  // no memory for it; then nothing is shown.
  template <const auto& Tree, std::size_t... Path>
  static bool show(storage<Tree, Path...>& s, std::size_t k) {
    if (k == s.shown) return true;
    lv_group_t* focus = focus_below(s.holder);
    s.region.reset();
    s.shown = no_arm;
    bool ok = true;
    template for (constexpr auto i : indices<arm_count>) {
      if (i == k) ok = show_arm<i, Tree, Path...>(s, focus);
    }
    return ok;
  }

  // Builds arm I in the empty region of `s`. False when LVGL had no memory; then nothing is shown.
  template <std::size_t I, const auto& Tree, std::size_t... Path>
  static bool show_arm(storage<Tree, Path...>& s, lv_group_t* focus) {
    if constexpr (has_tree<I, Tree, Path...>()) {
      auto& tree = s.region.template emplace<storage<Tree, Path...>::place[I]>();
      if (!tree.mount(obj{s.holder}).has_value()) {
        s.region.reset();
        return false;
      }
      s.shown = I;
      if (focus != nullptr) focus_first(focus, tree.root().raw());
      welcome<I, Tree, Path...>(s);
    } else {
      s.shown = I;
    }
    return true;
  }

  // .on_shown of arm I, with a handle to its tree.
  template <std::size_t I, const auto& Tree, std::size_t... Path>
  static void welcome(storage<Tree, Path...>& s) {
    constexpr const auto& hook = arm<I, Tree, Path...>().options.on_shown;
    if constexpr (!is_null(hook.tree)) {
      const auto handle =
          held_access::make<*arm<I, Tree, Path...>().tree>(&s, &storage<Tree, Path...>::template find<I>);
      hook.call(&handle);
    }
  }

  template <const auto& Tree, std::size_t... Path>
  static void changed(watch_node& w) {
    auto& s = static_cast<storage<Tree, Path...>&>(w);
    if (arm_of<Tree, Path...>(descend<Path...>(Tree).source->get()) != s.shown) defer(s, s.holder);
  }

  template <const auto& Tree, std::size_t... Path>
  static void run(deferred_node& d) {
    auto& s = static_cast<storage<Tree, Path...>&>(d);
    static_cast<void>(show<Tree, Path...>(s, arm_of<Tree, Path...>(descend<Path...>(Tree).source->get())));
  }

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, build_state& st) {
    static_assert(tree_problem<Tree, Path...>().empty(), tree_problem<Tree, Path...>());
    auto& s = storage_of<Tree, Path...>(st);
    const S& source = *descend<Path...>(Tree).source;
    s.holder = parent;
    s.changed = &changed<Tree, Path...>;
    s.run = &run<Tree, Path...>;
    graph_access::watchers(source).append(s);
    if (!show<Tree, Path...>(s, arm_of<Tree, Path...>(source.get()))) st.ok = false;
    return parent;
  }
};

// lv::when(flag)[tree]: the tree while the flag is true, nothing while it is false.
template <class S>
struct when_node {
  const S* source;

  template <class T>
    requires tree_node<T>
  constexpr auto operator[](const T& tree) const {
    using shows = tree_arm<one_of<true>, T>;
    using hides = tree_arm<one_of<false>, void>;
    return shown_node<S, shows, hides>{source, {shows{&tree, {}}, hides{}}};
  }
};

}  // namespace detail

template <class S, class... Arms>
inline constexpr bool detail::is_structural<detail::shown_node<S, Arms...>> = true;
template <class S, class... Arms>
inline constexpr bool detail::sole_child<detail::shown_node<S, Arms...>> = true;

// Shows the arm of the value of `state`: lv::shown(current)[lv::is<page::home>(home_ui), ...].
// See the top of this file.
template <detail::shown_source S>
constexpr detail::shown_node<S> shown(const S& state) {
  return {&state};
}
// The arms as further arguments: lv::shown(current, lv::is<page::home>(home_ui), ...).
template <detail::shown_source S, class... A>
  requires detail::children_given<A...>
constexpr auto shown(const S& state, const A&... arms) {
  using value_type = detail::value_of<S>;
  static_assert(detail::shown_problem<value_type, A...>().empty(), detail::shown_problem<value_type, A...>());
  return detail::shown_node<S, A...>{&state, {arms...}};
}
template <class S>
void shown(const S&) = delete(
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
constexpr detail::when_node<S> when(const S& flag) {
  return {&flag};
}
// The tree as the second argument: lv::when(online, online_ui).
template <detail::shown_source S, class T>
  requires std::same_as<detail::value_of<S>, bool> && tree_node<T>
constexpr auto when(const S& flag, const T& tree) {
  using shows = detail::tree_arm<detail::one_of<true>, T>;
  using hides = detail::tree_arm<detail::one_of<false>, void>;
  return detail::shown_node<S, shows, hides>{&flag, {shows{&tree, {}}, hides{}}};
}
template <class S>
void when(const S&) = delete("lv::when follows an lv::state or an lv::computed of a bool: lv::when(online, tree).");
template <class S, class T>
void when(const S&, const T&) = delete(
    "lv::when follows an lv::state or an lv::computed of a bool, and shows one tree: lv::when(online, online_ui).");

}  // namespace lv
