// The run-time storage of a tree: one object sized by the compiler from what each node
// kind declares, held inside the lv::mounted that owns the widgets. The object is built
// where the caller keeps it and never moves.
#include <lv/lv.hpp>

#include "check.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace {

// A node kind that keeps storage: it counts the storages alive, and writes where it was
// built into its storage. It adds no widget.
int alive = 0;
std::vector<const void*> built_at;

struct stamp {
  template <const auto& Tree, std::size_t... Path>
  struct storage {
    std::size_t depth = 0;
    std::array<std::size_t, 4> path{};
    storage() noexcept { ++alive; }
    ~storage() { --alive; }
  };

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state& st) {
    auto& s = lv::detail::storage_of<Tree, Path...>(st);
    s.depth = sizeof...(Path);
    s.path = {Path...};
    built_at.push_back(&s);
    return parent;
  }
};

// A kind of the same shape that keeps nothing.
struct plain_kind {
  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state&) {
    return parent;
  }
};

const void* scope_at = nullptr;

// A kind that opens a scope: its child keeps its storage inside the kind's own storage,
// as an arm of lv::shown will.
template <class Child>
struct scope_kind {
  lv::detail::kids<Child> kids{};

  template <const auto& Tree, std::size_t... Path>
  struct storage {
    bool opened = false;
    lv::detail::storage_t<Tree, Path..., 0> inner{};
  };

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, lv::detail::build_state& st) {
    auto& s = lv::detail::storage_of<Tree, Path...>(st);
    s.opened = true;
    scope_at = &s;
    void* own = st.storage;
    st.storage = &s.inner;
    lv::detail::build<Tree, Path..., 0>(parent, st);
    st.storage = own;
    return parent;
  }
};

// Kinds whose `storage` is not what lv::mounted can hold.
struct needs_an_argument {
  template <const auto& Tree, std::size_t... Path>
  struct storage {
    explicit storage(int) {}
  };
};
struct throws_at_the_end {
  template <const auto& Tree, std::size_t... Path>
  struct storage {
    ~storage() noexcept(false) {}
  };
};

}  // namespace

template <class Child>
inline constexpr bool lv::detail::is_structural<scope_kind<Child>> = true;
template <>
inline constexpr bool lv::detail::is_structural<stamp> = true;
template <>
inline constexpr bool lv::detail::is_structural<plain_kind> = true;

namespace {

using namespace lv::units;

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Storage"}),
    stamp{},
    lv::row[lv::label({.text = "a"}), stamp{}],
    lv::row[lv::box[stamp{}]]];

constexpr auto without = lv::column[lv::label({.id = "title", .text = "Plain"}), plain_kind{}];

using lv::detail::storage_t;

// What a kind's storage must be.
static_assert(lv::detail::storage_kind<stamp, ui, 1>);
static_assert(!lv::detail::storage_kind<plain_kind, ui, 1>);
static_assert(!lv::detail::storage_kind<needs_an_argument, ui, 1>);
static_assert(!lv::detail::storage_kind<throws_at_the_end, ui, 1>);

constexpr auto inner_row = lv::row[lv::label({.text = "inner"}), stamp{}];
constexpr auto scoped = lv::column[scope_kind<std::remove_cvref_t<decltype(inner_row)>>{{inner_row}}];

// The scope's child keeps its storage inside the kind's.
static_assert(std::is_same_v<decltype(storage_t<scoped>{}.s0.inner.s1), stamp::storage<scoped, 0, 0, 1>>);

// The storage is made from the kinds: a member s<k> for each child k with storage below.
static_assert(lv::detail::any_storage<std::remove_cvref_t<decltype(ui)>>);
static_assert(!lv::detail::any_storage<std::remove_cvref_t<decltype(without)>>);
static_assert(std::is_empty_v<lv::detail::tree_storage<without>>);
static_assert(std::is_same_v<storage_t<ui, 1>, stamp::storage<ui, 1>>);
static_assert(std::is_same_v<decltype(storage_t<ui>{}.s1), stamp::storage<ui, 1>>);
static_assert(std::is_same_v<decltype(storage_t<ui>{}.s2.s1), stamp::storage<ui, 2, 1>>);
static_assert(std::is_same_v<decltype(storage_t<ui>{}.s3.s0.s0), stamp::storage<ui, 3, 0, 0>>);
static_assert(sizeof(storage_t<ui>) == 3 * sizeof(stamp::storage<ui, 1>));

// The object holds the storage, and its size is a constant: no heap, any placement.
static_assert(sizeof(lv::mounted<ui>) >= sizeof(storage_t<ui>) + sizeof(lv::mounted<without>));
static_assert(lv::storage_size<ui> == sizeof(lv::mounted<ui>));
static_assert(!std::is_move_constructible_v<lv::mounted<ui>> && !std::is_copy_constructible_v<lv::mounted<ui>>);
static_assert(std::is_same_v<decltype(lv::mount<ui>(lv::obj{})), lv::mounted<ui>>);

template <class T>
concept can_detach = requires(T& t) { t.detach(); };
static_assert(can_detach<lv::mounted<without>> && !can_detach<lv::mounted<ui>>);

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

// Whether `p` points into the object `m`.
template <class M>
bool inside(const void* p, const M& m) {
  const auto at = reinterpret_cast<std::uintptr_t>(p);
  const auto first = reinterpret_cast<std::uintptr_t>(&m);
  return at >= first && at < first + sizeof(M);
}

// Whether the stamps were built at different places.
bool all_apart() {
  for (std::size_t i = 0; i < built_at.size(); ++i)
    for (std::size_t j = 0; j < i; ++j)
      if (built_at[i] == built_at[j]) return false;
  return true;
}

// Every stamp of `m` was built inside it, each at its own place, with its own path.
template <class M>
bool built_inside(const M& m) {
  if (built_at.size() != 3 || !all_apart()) return false;
  for (const void* at : built_at)
    if (!inside(at, m)) return false;
  const auto& last = *static_cast<const stamp::storage<ui, 3, 0, 0>*>(built_at[2]);
  return last.depth == 3 && last.path == std::array<std::size_t, 4>{3, 0, 0, 0};
}

void mounts_in_place(lv::obj screen) {
  built_at.clear();
  {
    auto root = lv::mount<ui>(screen);
    check(static_cast<bool>(root) && alive == 3 && built_inside(root));
    check(root->get<"title">().get<"text">() != nullptr);
    check(lv_obj_get_event_count(root.root().raw()) == 1);  // the one handler of the tree
  }
  check(alive == 0);  // the storage ends with the widgets
}

// A kind that opens a scope hands its child its own storage.
void opens_a_scope(lv::obj screen) {
  built_at.clear();
  {
    auto root = lv::mount<scoped>(screen);
    check(static_cast<bool>(root) && alive == 1 && built_at.size() == 1 && inside(built_at[0], root));
    const auto& scope = *static_cast<const storage_t<scoped, 0>*>(scope_at);
    check(inside(scope_at, root) && scope.opened && inside(built_at[0], scope));
  }
  check(alive == 0);
}

// The parent deletes the tree first, or C code deletes its root: the storage ends with
// the root, while the widgets still exist, and the lv::mounted deletes nothing twice.
void ends_with_the_root(lv::obj screen) {
  {
    auto box = screen.add<"obj">();
    auto root = lv::mount<ui>(box);
    check(alive == 3);
    lv_obj_delete(box.raw());
    check(alive == 0 && !root.root() && static_cast<bool>(root));
  }
  {
    auto root = lv::mount<ui>(screen);
    lv_obj_delete(root.root().raw());  // C API
    check(alive == 0 && !root.root());
  }
  check(alive == 0);
}

// A global made empty and mounted later.
constinit lv::mounted<ui> page;

void a_global_page(lv::obj screen) {
  check(!page && page.error() == lv::mount_error::not_mounted);
  built_at.clear();
  const auto mounted = page.mount(screen);
  check(mounted.has_value() && static_cast<bool>(page) && alive == 3 && built_inside(page));
}

// In memory of the caller's, with placement new behind a name.
void in_caller_memory(lv::obj screen) {
  alignas(lv::mounted<ui>) std::array<std::byte, lv::storage_size<ui>> memory;
  const auto children = lv_obj_get_child_count(screen.raw());
  built_at.clear();
  auto& root = lv::mount_in<ui>(memory, screen);
  check(static_cast<void*>(&root) == memory.data() && static_cast<bool>(root) && alive == 3 && built_inside(root));
  std::destroy_at(&root);
  check(alive == 0 && lv_obj_get_child_count(screen.raw()) == children);
}

// Code that keeps an lv::mounted keeps the prvalue: a returned call, a member, an
// optional. Each is built in its final place.
lv::mounted<ui> make(lv::obj parent) { return lv::mount<ui>(parent); }

struct two_screens {
  lv::mounted<ui> first;
  lv::mounted<ui> second;
};

void kept_where_built(lv::obj screen) {
  {
    built_at.clear();
    auto returned = make(screen);
    check(built_inside(returned));
  }
  {
    two_screens s{lv::mount<ui>(screen), lv::mount<ui>(screen)};
    check(alive == 6 && static_cast<bool>(s.first) && static_cast<bool>(s.second));
  }
  {
    std::optional<lv::mounted<ui>> later;
    built_at.clear();
    later.emplace(screen);
    check(later.has_value() && built_inside(*later));
    later.reset();
    check(alive == 0);
  }
  check(alive == 0);
}

// A tree that keeps nothing costs what it cost: the widgets and one handler. detach()
// and lv::place work for it as before.
void a_tree_without_storage(lv::obj screen) {
  lv::obj root;
  {
    auto m = lv::mount<without>(screen);
    root = m.detach();
    check(root && !m.root());
  }
  check(lv_obj_is_valid(root.raw()));
  lv_obj_delete(root.raw());
  const auto placed = lv::place<without>(screen);
  check(placed && lv_obj_get_event_count(placed.raw()) == 0);
  lv_obj_delete(placed.raw());
}

// The step by which the free heap grows from one round to the next.
constexpr std::size_t heap_step = 8;

// One block of at most `size` bytes, or null: the block's own header may not fit.
void* take_block(std::size_t size) {
  void* p = nullptr;
  while (p == nullptr && size >= heap_step) {
    p = lv_malloc(size);
    if (p == nullptr) size -= size / 16 > heap_step ? size / 16 : heap_step;
  }
  return p;
}

// Takes LVGL's heap until only `budget` bytes are free.
std::vector<void*> take_heap_but(std::size_t budget) {
  std::vector<void*> taken;
  for (;;) {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    if (m.free_size <= budget || m.free_biggest_size == 0) return taken;
    void* p = take_block(m.free_biggest_size > budget ? m.free_biggest_size - budget : m.free_biggest_size);
    if (p == nullptr) return taken;
    taken.push_back(p);
  }
}

// Mounts once; a failure must leave no storage and no widget. Whether it succeeded.
bool mount_once(lv::obj screen, std::size_t children) {
  auto root = lv::mount<ui>(screen);
  if (root) return true;
  check(root.error() == lv::mount_error::out_of_memory && alive == 0 &&
        lv_obj_get_child_count(screen.raw()) == children);
  return false;
}

// The heap runs out at every point of the mount: nothing is left behind, neither storage
// nor widgets nor heap. LVGL's theme is off: it crashes when its own realloc fails.
void out_of_memory_leaves_nothing(lv::obj screen) {
  constexpr int most_rounds = 2000;
  const std::size_t children = lv_obj_get_child_count(screen.raw());
  const std::size_t start = heap_used();
  const std::size_t start_blocks = heap_blocks();
  lv_log_register_print_cb([](lv_log_level_t, const char*) {});  // a thousand expected failures
  int failed = 0;
  bool mounted = false;
  for (int round = 0; round < most_rounds && !mounted; ++round) {
    const std::vector<void*> taken = take_heap_but(static_cast<std::size_t>(round) * heap_step);
    mounted = mount_once(screen, children);
    failed += mounted ? 0 : 1;
    for (void* p : taken) lv_free(p);
    // Counted in blocks: the allocator may keep a few bytes of a split block for a while.
    check(alive == 0 && heap_blocks() == start_blocks);
  }
  lv_log_register_print_cb(nullptr);
  check(mounted && failed > 0 && heap_used() == start);
  std::printf("out of memory at %d points\n", failed);
}

}  // namespace

int main() {
  lv_init();
  lv_display_t* display = lv_test_display_create(320, 240);
  lv_display_set_theme(display, nullptr);
  auto screen = lv::screen_active();

  mounts_in_place(screen);
  opens_a_scope(screen);
  ends_with_the_root(screen);
  in_caller_memory(screen);
  kept_where_built(screen);
  a_tree_without_storage(screen);
  out_of_memory_leaves_nothing(screen);
  a_global_page(screen);  // last: the global outlives LVGL

  lv_deinit();
  check(alive == 0);  // LVGL deleted the page's widgets, and with them its storage
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
