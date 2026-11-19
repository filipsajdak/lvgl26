// lv::one_of: one slot for one of several trees, in one region of the largest size, one
// tree alive at a time; and its visit.
#include <lv/lv.hpp>

#include "check.hpp"
#include "full_heap.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <variant>
#include <type_traits>
#include <vector>

namespace {

// A node kind that counts its storages alive, so that the test sees each tree's storage end.
int alive = 0;

struct stamp {
  template <const auto& Tree, std::size_t... Path>
  struct storage {
    std::array<std::byte, 64> room{};
    storage() noexcept { ++alive; }
    ~storage() { --alive; }
  };

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state& st) {
    static_cast<void>(lv::detail::storage_of<Tree, Path...>(st));
    return parent;
  }
};

}  // namespace

template <>
inline constexpr bool lv::detail::is_structural<stamp> = true;

namespace {

using namespace lv::units;

void to_settings();

constexpr auto home = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Home"}),
    lv::button({.id = "next", .on_clicked = &to_settings, .size = {80 * px, 40 * px}})[lv::label({.text = "Next"})],
    stamp{}];
constexpr auto settings = lv::column[lv::label({.id = "heading", .text = "Settings"}), lv::row[stamp{}], lv::box[stamp{}]];
constexpr auto dialog = lv::column[lv::label({.id = "question", .text = "Discard?"})];

using slot_type = lv::one_of<home, settings, dialog>;

// The slot is the largest lv::mounted of its trees and the index of the one alive.
constexpr std::size_t largest =
    std::max({sizeof(lv::mounted<home>), sizeof(lv::mounted<settings>), sizeof(lv::mounted<dialog>)});
static_assert(sizeof(slot_type) <= largest + alignof(slot_type) + sizeof(std::size_t));
static_assert(sizeof(slot_type) >= largest);
static_assert(alignof(slot_type) >= alignof(lv::mounted<settings>));
static_assert(!std::is_move_constructible_v<slot_type> && !std::is_copy_constructible_v<slot_type>);

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

std::size_t heap_blocks() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.used_cnt;
}

struct heap_use {
  std::size_t bytes;
  std::size_t blocks;
  bool operator==(const heap_use&) const = default;
};

heap_use heap_now() { return {heap_used(), heap_blocks()}; }

template <class S>
bool is_empty(const S& s) {
  return !s && !s.has_value() && s.error() == lv::mount_error::not_mounted;
}

// Whether `p` points into the object `m`.
template <class M>
bool inside(const void* p, const M& m) {
  const auto at = reinterpret_cast<std::uintptr_t>(p);
  const auto first = reinterpret_cast<std::uintptr_t>(&m);
  return at >= first && at < first + sizeof(M);
}

constinit slot_type window;

// The handler of a button of home mounts settings in the same slot: it ends the tree whose
// click it runs in, and reuses its memory.
void to_settings() { static_cast<void>(window.mount<settings>(lv::screen_active())); }

// What visit sees of an arm, by its parameter type.
template <class A>
constexpr int pattern = lv::detail::arm_pattern<A, home, settings, dialog>();
void by_name(lv::held<dialog>) {}
constexpr auto by_const = [](const lv::held<settings>&) {};
constexpr auto no_throw = [](lv::held<home>) noexcept {};
constexpr auto two = [](lv::held<home>, int) {};
static_assert(pattern<decltype(by_name)> == 2 && pattern<decltype(&by_name)> == 2);
static_assert(pattern<decltype(by_const)> == 1 && pattern<decltype(no_throw)> == 0);
static_assert(pattern<decltype([n = 0](lv::held<dialog>) mutable { ++n; })> == 2);
static_assert(pattern<decltype([](auto) {})> == lv::detail::arm_any);
static_assert(pattern<decltype([](lv::not_mounted) {})> == lv::detail::arm_empty);
static_assert(pattern<decltype(two)> == lv::detail::arm_never);
static_assert(pattern<decltype([](int) {})> == lv::detail::arm_never);

namespace screens {
constexpr auto about = lv::column[lv::label({.text = "About"})];
}
static_assert(std::string_view{std::define_static_string(lv::detail::tree_name<screens::about>())}.ends_with("screens::about"));

void starts_empty() { check(is_empty(window) && !window.holds<home>() && !window.holds<settings>()); }

// Whether the slot holds the tree T, mounted in the slot's own memory.
template <const auto& T>
bool holds_inside() {
  return window.has_value() && window.holds<T>() && inside(&*window.get<T>(), window) && window.get<T>()->root();
}

// mount ends the tree that is there first; each tree lives in the slot's own memory.
void mounts_one_at_a_time(lv::obj screen) {
  const auto children = lv_obj_get_child_count(screen.raw());
  check(window.mount<home>(screen).has_value() && holds_inside<home>() && alive == 1);
  check(window.get<home>()->get<"title">().get<"text">() != nullptr);
  check(window.mount<settings>(screen).has_value() && holds_inside<settings>() && !window.holds<home>());
  check(alive == 2 && lv_obj_get_child_count(screen.raw()) == children + 1);
  check(static_cast<bool>(window));
  window.unmount();
  check(is_empty(window) && alive == 0 && lv_obj_get_child_count(screen.raw()) == children);
}

// The parent deletes the tree, or C code deletes its root: the slot is empty.
void ends_with_the_root(lv::obj screen) {
  auto box = screen.add<"obj">();
  check(window.mount<settings>(box).has_value());
  lv_obj_delete(box.raw());
  check(is_empty(window) && alive == 0);
  check(window.mount<home>(screen).has_value());
  lv_obj_delete(window.get<home>()->root().raw());  // C API
  check(is_empty(window) && alive == 0);
}

// Switching between the trees many times ends at the heap it started from: lvgl26
// allocates nothing, and every tree's mount costs what its first mount cost.
void switches_without_heap(lv::obj screen) {
  constexpr int rounds = 200;
  const heap_use start = heap_now();
  check(window.mount<home>(screen).has_value());
  const heap_use with_home = heap_now();
  check(window.mount<settings>(screen).has_value());
  const heap_use with_settings = heap_now();
  bool stable = true;
  for (int i = 0; i < rounds; ++i) {
    stable = window.mount<home>(screen).has_value() && heap_now() == with_home && stable;
    stable = window.mount<settings>(screen).has_value() && heap_now() == with_settings && stable;
  }
  window.unmount();
  check(stable && heap_now() == start);
  std::printf("home %zu bytes in %zu blocks, settings %zu bytes in %zu blocks, the same after %d switches\n",
              with_home.bytes - start.bytes, with_home.blocks - start.blocks, with_settings.bytes - start.bytes,
              with_settings.blocks - start.blocks, rounds);
}

// A handle kept after the visit holds while the slot holds its tree: false once the slot
// changed, and true again when the same tree is mounted again. An arm may switch the slot.
// A handle to home, taken in a visit.
lv::held<home> handle_of_home() {
  lv::held<home> kept{};
  window.visit([&](lv::held<home> h) { kept = h; }, [](auto) {});
  return kept;
}

// An arm that mounts settings into the slot it visits: allowed. Returns its own handle.
lv::held<home> remount_in_arm(lv::obj screen) {
  lv::held<home> kept{};
  window.visit([&](lv::held<home> h) { static_cast<void>(window.mount<settings>(screen)); kept = h; },
               [](auto) {});
  return kept;
}

// After the slot changed, kept handles no longer hold; mounting home again, they do.
void held_again(lv::obj screen, lv::held<home> kept, lv::held<home> in_arm) {
  check(!kept && !in_arm && window.holds<settings>() && window.get<settings>());
  window.unmount();
  check(!kept && !window.get<settings>());
  check(window.mount<home>(screen).has_value() && kept && in_arm);  // the same tree again
  window.unmount();
}

void handles_follow_the_slot(lv::obj screen) {
  check(!lv::held<home>{} && window.mount<home>(screen).has_value());
  const lv::held<home> kept = handle_of_home();
  check(kept.has_value() && kept->root() && (*kept).has_value());
  held_again(screen, kept, remount_in_arm(screen));
}

// visit<R> names the result: every arm's result converts to R; visit<void> discards.
void visit_with_a_named_result(lv::obj screen) {
  using word = std::variant<int, std::string_view>;
  const auto empty = window.visit<word>([](lv::held<home>) { return 1; }, [](auto) { return std::string_view{"other"}; });
  static_assert(std::is_same_v<decltype(empty), const std::optional<word>>);
  check(!empty);
  check(window.mount<settings>(screen).has_value());
  const word named = window.visit<word>([](lv::held<home>) { return 1; }, [](auto) { return std::string_view{"other"}; },
                                        [](lv::not_mounted) { return 0; });
  check(std::get<std::string_view>(named) == "other");
  static_assert(std::is_void_v<decltype(window.visit<void>([](auto) { return 1; }))>);
  window.visit<void>([](auto) { return 1; });
  window.unmount();
}

// A click on a button of home switches the slot to settings, from inside home.
void switches_from_inside(lv::obj screen) {
  lv_test_indev_create_all();
  check(window.mount<home>(screen).has_value());
  lv_refr_now(nullptr);
  lv_area_t at;
  lv_obj_get_coords(window.get<home>()->get<"next">().raw(), &at);
  lv_test_mouse_click_at((at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2);
  check(window.holds<settings>() && alive == 2);
  window.unmount();
  check(is_empty(window) && alive == 0);
}

// A failed mount over a tree that is mounted: the old tree is gone first, and the slot is
// empty with the error. settings frees less heap than home needs.
void fails_over_a_live_tree(lv::obj screen) {
  const std::size_t children = lv_obj_get_child_count(screen.raw());
  check(window.mount<settings>(screen).has_value());
  lv_log_register_print_cb([](lv_log_level_t, const char*) {});
  {
    full_heap no_room{0};
    check(!window.mount<home>(screen).has_value());
  }
  lv_log_register_print_cb(nullptr);
  check(!window && window.error() == lv::mount_error::out_of_memory && alive == 0);
  check(lv_obj_get_child_count(screen.raw()) == children && !window.holds<settings>());
}

// The arms of visit, as an application writes them.
int visits(slot_type& s) {
  return s.visit([](lv::held<home>) { return 1; }, [](lv::held<settings>) { return 2; },
                 [](auto) { return 3; }, [](lv::not_mounted) { return 0; });
}

std::optional<int> visits_without_empty(slot_type& s) {
  return s.visit([](lv::held<home> h) { return h->root() ? 10 : -1; }, [](auto) { return 20; });
}

int counted = 0;
void count_dialog(lv::held<dialog>) { ++counted; }

void visit_calls_the_arm_of_the_tree(lv::obj screen) {
  check(visits(window) == 0 && !visits_without_empty(window).has_value());
  check(window.mount<home>(screen).has_value() && visits(window) == 1 && visits_without_empty(window) == 10);
  check(window.mount<settings>(screen).has_value() && visits(window) == 2 && visits_without_empty(window) == 20);
  check(window.mount<dialog>(screen).has_value() && visits(window) == 3);
  // Arms that return nothing are skipped on an empty slot; a function is an arm too.
  window.visit(&count_dialog, [](auto) { counted += 100; });
  window.unmount();
  window.visit(&count_dialog, [](auto) { counted += 100; });
  check(counted == 1);
  static_assert(std::is_same_v<decltype(visits_without_empty(window)), std::optional<int>>);
  // An int arm and a double arm give their common type, double, as std::visit allows.
  check(window.mount<settings>(screen).has_value());
  const auto common = window.visit([](lv::held<settings>) { return 2; }, [](auto) { return 0.5; });
  static_assert(std::is_same_v<decltype(common), const std::optional<double>>);
  check(common == 2.0);
  window.unmount();
  static_assert(std::is_void_v<decltype(window.visit(&count_dialog, [](auto) {}))>);
}

// The heap runs out at every point of a mount: the slot is empty with the error, nothing
// is left behind, and it mounts again in the same memory.
constexpr std::size_t heap_step = 8;

void* take_block(std::size_t size) {
  void* p = nullptr;
  while (p == nullptr && size >= heap_step) {
    p = lv_malloc(size);
    if (p == nullptr) size -= size / 16 > heap_step ? size / 16 : heap_step;
  }
  return p;
}

std::vector<void*> take_heap_but(std::size_t budget) {
  std::vector<void*> taken;
  for (;;) {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    // Only the blocks larger than the budget: a fragmented heap keeps its small blocks, and the
    // largest that is left is the budget (lv::detail::add_style probes for one block).
    if (m.free_biggest_size <= budget) return taken;
    void* p = take_block(m.free_biggest_size - budget);
    if (p == nullptr) return taken;
    taken.push_back(p);
  }
}

// Mounts settings in the empty slot; a failure leaves it empty with its error.
bool switch_once(lv::obj screen) {
  if (window.mount<settings>(screen)) return true;
  check(!window && window.error() == lv::mount_error::out_of_memory && alive == 0);
  return false;
}

// One mount with only `budget` bytes of heap free, then the heap back. Whether it mounted.
bool squeezed_round(lv::obj screen, std::size_t budget) {
  const std::vector<void*> taken = take_heap_but(budget);
  const bool mounted = switch_once(screen);
  if (mounted) window.unmount();
  for (void* p : taken) lv_free(p);
  return mounted;
}

void out_of_memory_leaves_the_slot_empty(lv::obj screen) {
  constexpr int most_rounds = 2000;
  const std::size_t children = lv_obj_get_child_count(screen.raw());
  const std::size_t start_blocks = heap_blocks();
  lv_log_register_print_cb([](lv_log_level_t, const char*) {});  // a thousand expected failures
  int failed = 0;
  bool mounted = false;
  for (int round = 0; round < most_rounds && !mounted; ++round) {
    mounted = squeezed_round(screen, static_cast<std::size_t>(round) * heap_step);
    failed += mounted ? 0 : 1;
    check(alive == 0 && heap_blocks() == start_blocks && lv_obj_get_child_count(screen.raw()) == children);
  }
  lv_log_register_print_cb(nullptr);
  check(mounted && failed > 0);
  std::printf("out of memory at %d points\n", failed);
}

// A call that breaks a contract ends the program with a contract violation. Each case is
// run as a test of its own, which expects that.
struct violation {
  const char* name;
  int (*run)(lv::obj screen);
};

constinit slot_type misused;

constexpr violation violations[] = {
    {"unmount_empty", [](lv::obj) { return misused.unmount(), 0; }},
    {"get_other",
     [](lv::obj screen) {
       static_cast<void>(misused.mount<home>(screen));
       return misused.get<settings>()->root() ? 1 : 0;
     }},
    {"held_after_remount",
     [](lv::obj screen) {
       static_cast<void>(misused.mount<home>(screen));
       return misused.visit([&](lv::held<home> h) {
         static_cast<void>(misused.mount<settings>(screen));  // allowed: the slot changes
         return h->root() ? 1 : 0;                            // the handle no longer holds
       },
                            [](auto) { return 0; })
           .value_or(0);
     }},
    {"error_of_mounted",
     [](lv::obj screen) {
       static_cast<void>(misused.mount<home>(screen));
       return static_cast<int>(misused.error());
     }},
};

int violates(const char* which, lv::obj screen) {
  for (const violation& v : violations)
    if (std::strcmp(which, v.name) == 0) return v.run(screen);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  lv_display_t* display = lv_test_display_create(320, 240);
  lv_display_set_theme(display, nullptr);
  auto screen = lv::screen_active();
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1], screen));
    return 0;
  }

  starts_empty();
  mounts_one_at_a_time(screen);
  ends_with_the_root(screen);
  switches_without_heap(screen);
  visit_calls_the_arm_of_the_tree(screen);
  switches_from_inside(screen);
  handles_follow_the_slot(screen);
  visit_with_a_named_result(screen);
  fails_over_a_live_tree(screen);
  out_of_memory_leaves_the_slot_empty(screen);
  check(window.mount<dialog>(screen).has_value());  // the global outlives LVGL

  lv_deinit();
  check(alive == 0);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
