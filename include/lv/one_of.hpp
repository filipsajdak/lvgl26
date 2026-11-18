#pragma once

// lv::one_of: one slot for one of several trees, under the rules of std::variant. Its size
// and alignment are those of the largest lv::mounted of its trees, and one tree is alive
// at a time. A slot is for a root, such as a window that shows one of several dialogs;
// pages inside a tree are switched by lv::shown.
//
//   constinit lv::one_of<home, settings> window;
//   window.mount<settings>(display.screen());    // ends the tree that is there first
//   window.visit([](lv::mounted<settings>& s) { s->get<"title">().set({.text = "Settings"}); },
//                [](auto&) {});                  // every other tree
//   window.unmount();
//
// It carries its own failure, as lv::mounted does: if (!window), window.has_value(),
// window.error(), which is mount_error::not_mounted while the slot holds no tree.

#include <lv/detail/one_of_region.hpp>
#include <lv/tree.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace lv {

// The arm of visit for a slot that holds no tree: [](lv::not_mounted) { ... }.
struct not_mounted {};

namespace detail {

// The name of a tree, as it is written: "home".
template <const auto& T>
consteval std::string tree_name() {
  return str(std::meta::display_string_of(std::meta::reflect_object(T)));
}

// The trees of a slot, as a list for a message: "home, settings, dialog".
template <const auto&... Trees>
consteval std::string tree_list() {
  std::string out;
  ((out += (out.empty() ? "" : ", ") + tree_name<Trees>()), ...);
  return out;
}

// The index of the tree T among Trees, or a compile error that lists them.
template <const auto& T, const auto&... Trees>
consteval std::size_t tree_index() {
  std::size_t at = 0;
  const bool found = ((std::same_as<mounted<T>, mounted<Trees>> || (++at, false)) || ...);
  if (!found)
    fail("lv: " + tree_name<T>() + " is not a tree of this lv::one_of. Its trees: " + tree_list<Trees...>() +
         ".");
  return at;
}

// What an arm of visit takes, told by its parameter type. A number is the index of a tree.
inline constexpr int arm_any = -1;    // auto&: every tree that no other arm names
inline constexpr int arm_empty = -2;  // lv::not_mounted: the slot holds no tree
inline constexpr int arm_never = -3;  // anything else: it can never be called

// The one parameter of a function type, or none.
template <class F>
struct one_parameter {
  static constexpr bool one = false;
  using type = void;
};
template <class R, class P>
struct one_parameter<R(P)> {
  static constexpr bool one = true;
  using type = P;
};
template <class R, class P>
struct one_parameter<R(P) noexcept> : one_parameter<R(P)> {};
template <class R, class C, class P>
struct one_parameter<R (C::*)(P)> : one_parameter<R(P)> {};
template <class R, class C, class P>
struct one_parameter<R (C::*)(P) const> : one_parameter<R(P)> {};
template <class R, class C, class P>
struct one_parameter<R (C::*)(P) noexcept> : one_parameter<R(P)> {};
template <class R, class C, class P>
struct one_parameter<R (C::*)(P) const noexcept> : one_parameter<R(P)> {};

// The function type of an arm that is not a template: a function, a pointer to one, or a
// lambda whose parameter is not auto. void for a generic lambda.
template <class F>
consteval std::meta::info arm_signature() {
  if constexpr (std::is_function_v<F>)
    return ^^F;
  else if constexpr (std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>)
    return ^^std::remove_pointer_t<F>;
  else if constexpr (requires { &F::operator(); })
    return ^^decltype(&F::operator());
  else
    return ^^void;
}

// The pattern of an arm whose one parameter is P.
template <class P, const auto&... Trees>
consteval int pattern_of() {
  using bare = std::remove_cvref_t<P>;
  if constexpr (std::same_as<bare, not_mounted>) {
    return arm_empty;
  } else {
    int at = 0;
    const bool found = ((std::same_as<bare, mounted<Trees>> || (++at, false)) || ...);
    return found ? at : arm_never;
  }
}

// The pattern of the arm A.
template <class A, const auto&... Trees>
consteval int arm_pattern() {
  using F = std::remove_cvref_t<A>;
  constexpr std::meta::info signature = arm_signature<F>();
  if constexpr (signature == ^^void) {
    return arm_any;
  } else {
    using parameter = one_parameter<typename[:signature:]>;
    if constexpr (parameter::one)
      return pattern_of<typename parameter::type, Trees...>();
    else
      return arm_never;
  }
}

// Which arm visit calls for each tree, and for the empty slot (-1: none).
template <std::size_t Trees, std::size_t Arms>
struct visit_plan {
  std::array<int, Trees> arm_of_tree{};
  int empty_arm = -1;
};

// The first arm with the pattern `pattern`, or -1.
template <std::size_t Arms>
consteval int first_arm(const std::array<int, Arms>& patterns, int pattern) {
  for (std::size_t a = 0; a < Arms; ++a)
    if (patterns[a] == pattern) return static_cast<int>(a);
  return -1;
}

// Fails for an arm that no tree and no empty slot reaches.
template <std::size_t Trees, std::size_t Arms>
consteval void every_arm_called(const visit_plan<Trees, Arms>& plan, std::string_view trees) {
  for (std::size_t a = 0; a < Arms; ++a) {
    bool called = plan.empty_arm == static_cast<int>(a);
    for (int chosen : plan.arm_of_tree) called = called || chosen == static_cast<int>(a);
    if (!called)
      fail("lv: arm " + decimal(a + 1) +
           " of visit can never be called. An arm takes lv::mounted<tree>& for one tree of the "
           "slot, auto& for every tree that no other arm names, or lv::not_mounted for the empty "
           "slot; each tree once. The slot's trees: " +
           str(trees) + ".");
  }
}

// The arm visit calls for each tree: the one that names the tree, else the auto& arm.
template <std::size_t Trees, std::size_t Arms>
consteval visit_plan<Trees, Arms> plan_visit(const std::array<int, Arms>& patterns,
                                             const std::array<std::string_view, Trees>& names,
                                             std::string_view trees) {
  visit_plan<Trees, Arms> plan;
  for (std::size_t t = 0; t < Trees; ++t) {
    const int named = first_arm(patterns, static_cast<int>(t));
    plan.arm_of_tree[t] = named >= 0 ? named : first_arm(patterns, arm_any);
    if (plan.arm_of_tree[t] < 0)
      fail("lv: no arm of visit takes the tree " + str(names[t]) + ". Add an arm that takes lv::mounted<" +
           str(names[t]) + ">&, or one that takes auto&.");
  }
  plan.empty_arm = first_arm(patterns, arm_empty);
  every_arm_called(plan, trees);
  return plan;
}

// Fails unless every arm returns the same type.
consteval std::meta::info one_result(const std::vector<std::meta::info>& results) {
  for (auto r : results)
    if (std::meta::dealias(r) != std::meta::dealias(results.front()))
      fail("lv: the arms of visit return different types: " +
           str(std::meta::display_string_of(std::meta::dealias(results.front()))) + " and " +
           str(std::meta::display_string_of(std::meta::dealias(r))) + ". Make every arm return one type.");
  return results.front();
}

}  // namespace detail

// One slot for one of the trees Trees, alive one at a time. See the top of this file.
template <const auto&... Trees>
  requires(tree_node<decltype(Trees)> && ...)
class one_of {
  detail::one_of_region<mounted<Trees>...> region_;

  static constexpr std::size_t count = sizeof...(Trees);
  static constexpr std::size_t none = count;

  static constexpr std::array<std::string_view, count> names{
      std::string_view{std::define_static_string(detail::tree_name<Trees>())}...};
  static constexpr std::string_view list{std::define_static_string(detail::tree_list<Trees...>())};

  // The index of the tree that is mounted, or `none`.
  std::size_t mounted_index() const noexcept {
    std::size_t at = none;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      ((region_.index() == I && region_.template get<I>().has_value() ? at = I : at), ...);
    }(std::make_index_sequence<count>{});
    return at;
  }

  // Why the tree that is there is not mounted: not_mounted, or the failure of its mount.
  mount_error error_of_alive() const noexcept {
    mount_error e = mount_error::not_mounted;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      ((region_.index() == I ? e = region_.template get<I>().error() : e), ...);
    }(std::make_index_sequence<count>{});
    return e;
  }

  template <auto Plan, class... Arms>
  static consteval std::meta::info result_of_arms() {
    std::vector<std::meta::info> results;
    template for (constexpr auto t : detail::indices<count>) {
      using arm = Arms...[static_cast<std::size_t>(Plan.arm_of_tree[t])];
      results.push_back(^^std::invoke_result_t<arm&, mounted<Trees...[t]>&>);
    }
    if constexpr (Plan.empty_arm >= 0)
      results.push_back(^^std::invoke_result_t<Arms...[static_cast<std::size_t>(Plan.empty_arm)]&, not_mounted>);
    return detail::one_result(results);
  }

  // What visit returns: R with an arm for the empty slot or when the arms return nothing,
  // std::optional<R> without one.
  template <auto Plan, class R>
  using visit_result = std::conditional_t<Plan.empty_arm >= 0 || std::is_void_v<R>, R, std::optional<R>>;

  template <class Out, auto Plan, class... Arms>
  Out on_empty(Arms&... arms) {
    if constexpr (Plan.empty_arm >= 0)
      return arms...[static_cast<std::size_t>(Plan.empty_arm)](not_mounted{});
    else if constexpr (!std::is_void_v<Out>)
      return std::nullopt;
  }

  template <class Out, auto Plan, std::size_t T, class... Arms>
  Out on_tree(Arms&... arms) {
    return arms...[static_cast<std::size_t>(Plan.arm_of_tree[T])](region_.template get<T>());
  }

 public:
  // Holds no tree: a constinit global, or a member.
  constexpr one_of() = default;
  one_of(const one_of&) = delete;
  one_of& operator=(const one_of&) = delete;

  // Ends the tree that is there, then builds the widgets of T under `parent`, in the slot's
  // memory. A failure leaves the slot empty, with the error. lvgl26 allocates nothing.
  template <const auto& T>
    requires tree_node<decltype(T)>
  std::expected<void, mount_error> mount(obj parent)
    pre(parent.raw() != nullptr)
  {
    return region_.template emplace<detail::tree_index<T, Trees...>()>().mount(parent);
  }

  // Deletes the widgets of the tree that is mounted: the slot is empty.
  void unmount()
    pre(has_value())
  {
    region_.reset();
  }

  // Whether a tree is mounted: built, and not deleted since.
  bool has_value() const noexcept { return mounted_index() != none; }
  explicit operator bool() const noexcept { return has_value(); }

  // Why no tree is mounted: not_mounted, or the failure of the last mount.
  mount_error error() const
    pre(!has_value())
  {
    return error_of_alive();
  }

  // Whether the tree T is the one mounted.
  template <const auto& T>
    requires tree_node<decltype(T)>
  bool holds() const noexcept {
    return mounted_index() == detail::tree_index<T, Trees...>();
  }

  // The tree T, which must be the one mounted.
  template <const auto& T>
    requires tree_node<decltype(T)>
  mounted<T>& get()
    pre(holds<T>())
  {
    return region_.template get<detail::tree_index<T, Trees...>()>();
  }

  // Calls the arm whose parameter names the tree that is mounted: lv::mounted<home>& for
  // home, auto& for every tree no other arm names, lv::not_mounted for the empty slot.
  // Without that arm an empty slot calls nothing, and the result is std::optional<R>.
  template <class... Arms>
  auto visit(Arms&&... arms) {
    static constexpr auto plan = detail::plan_visit<count, sizeof...(Arms)>(
        std::array<int, sizeof...(Arms)>{detail::arm_pattern<Arms, Trees...>()...}, names, list);
    using R = typename[:result_of_arms<plan, std::remove_cvref_t<Arms>...>():];
    using Out = visit_result<plan, R>;
    const std::size_t at = mounted_index();
    template for (constexpr auto t : detail::indices<count>) {
      if (at == t) return on_tree<Out, plan, t>(arms...);
    }
    return on_empty<Out, plan>(arms...);
  }
};

}  // namespace lv
