#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>
#include <type_traits>

namespace {

using label_view = lv::view<"label">;
using namespace lv::literals;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

int stateless_calls = 0;

void properties(lv::obj screen) {
  auto l = screen.add<"label">().set<"text">("Speed");
  CHECK(std::strcmp(l.get<"text">(), "Speed") == 0);
  static_assert(std::is_same_v<decltype(l), lv::view<"label">>);

  auto a = screen.add<"arc">().set<"range">(0, 240).set<"value">(120);
  CHECK(a.get<"value">() == 120);
  CHECK(a.get<"max_value">() == 240);

  // A property of obj, reached from a label.
  int tag = 7;
  l.set<"user_data">(&tag);
  CHECK(l.get<"user_data">() == &tag);

  // Typed properties: units and strict integers.
  a.set({.rotation = 90_deg, .value = 30});
  CHECK(a.get<"value">() == 30);
  CHECK(a.get<"rotation">() == 90);
  l.set({.text = "km/h", .width = 80_px});
  CHECK(std::strcmp(l.get<"text">(), "km/h") == 0);
  lv_obj_update_layout(l.raw());
  CHECK(l.get<"width">() == 80);

  // A view is a pointer: copies compare equal and convert to obj.
  lv::obj as_obj = l;
  CHECK(as_obj.raw() == l.raw());
  CHECK(lv::view<"label">{l.raw()} == l);
  CHECK(!lv::view<"label">{});
}

void events(lv::obj screen) {
  auto b = screen.add<"button">();
  stateless_calls = 0;

  b.on<LV_EVENT_CLICKED>([] { ++stateless_calls; });
  int captured_calls = 0;
  b.on<LV_EVENT_CLICKED>([&captured_calls](lv::event e) {
    if (e.code() == LV_EVENT_CLICKED && e.target<lv::view<"button">>()) ++captured_calls;
  });
  int other_calls = 0;
  b.on<LV_EVENT_PRESSED>([p = &other_calls] { ++*p; });

  lv_obj_send_event(b.raw(), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(b.raw(), LV_EVENT_CLICKED, nullptr);
  CHECK(stateless_calls == 2);
  CHECK(captured_calls == 2);
  CHECK(other_calls == 0);
}

void weak_views(lv::obj screen) {
  auto box = screen.add<"obj">();
  lv::weak<label_view> w{box.add<"label">()};
  CHECK(w && !w.expired());

  lv::weak<label_view> copy = w;
  lv::weak<label_view> moved = std::move(w);
  CHECK(w.expired());
  CHECK(copy && moved && copy.get() == moved.get());

  lv_obj_delete(box.raw());  // the parent deletes the label
  CHECK(copy.expired() && moved.expired());
  CHECK(!copy.get());
}

void owned_views(lv::obj screen) {
  const auto count = [&] { return lv_obj_get_child_count(screen.raw()); };
  const auto before = count();

  {
    lv::owned<label_view> o{screen.add<"label">()};
    CHECK(count() == before + 1);
    lv::owned<label_view> p = std::move(o);
    CHECK(!o && p);
    CHECK(count() == before + 1);
  }
  CHECK(count() == before);

  // The parent deletes first: the owner must not delete again.
  {
    auto box = screen.add<"obj">();
    lv::owned<label_view> o{box.add<"label">()};
    lv_obj_delete(box.raw());
    CHECK(!o);
  }
  CHECK(count() == before);

  // release() hands the widget back to its parent.
  label_view kept;
  {
    lv::owned<label_view> o{screen.add<"label">()};
    kept = o.release();
    CHECK(!o);
  }
  CHECK(count() == before + 1);
  lv_obj_delete(kept.raw());

  lv::owned<label_view> a{screen.add<"label">()};
  a = lv::owned<label_view>{screen.add<"label">()};  // the first label is deleted
  CHECK(count() == before + 1);
  a.reset();
  CHECK(count() == before);
}

}  // namespace

int main() {
  lv_init();
  lv_display_create(320, 240);
  auto screen = lv::screen_active();

  // LVGL allocates some things once, on first use (for example the screen's child
  // list). So the first pass warms up, and the next passes must leave nothing behind.
  std::size_t free_after[3];
  for (auto& f : free_after) {
    auto root = screen.add<"obj">();
    properties(root);
    events(root);
    weak_views(root);
    owned_views(root);
    lv_obj_delete(root.raw());
    f = free_bytes();
  }
  CHECK(free_after[1] == free_after[0]);
  CHECK(free_after[2] == free_after[0]);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
