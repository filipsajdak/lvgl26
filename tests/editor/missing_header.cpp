// A template or a function of an opt-in header, used with lv/lv.hpp alone: the diagnostic names
// the header. Read by Clang with the editor stubs only (the test editor_missing_header); GCC gives
// the same text with lvgl26's own headers (tests/fail/missing_header_*.cpp).
#include <lv/lv.hpp>

namespace missing_header {

struct track {
  int id;
};

enum class mode { a, b };

constexpr int items[] = {1, 2};
constexpr unsigned char png[] = {1, 2, 3};
constexpr auto a = lv::label();
constexpr auto b = lv::button();

// expected-error@*:* {{lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>}}
lv::list<track, 4> tracks;

// expected-error@+1 {{lv::each is in <lv/list.hpp>: add #include <lv/list.hpp>}}
auto rows = lv::each(items, &track::id);

// expected-error@+1 {{lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>}}
auto by_mode = lv::for_each<mode>([]<mode M> { return lv::label(); });

// expected-error@+1 {{lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>}}
auto by_item = lv::for_each<items>([]<int I> { return lv::label(); });

// expected-error@+1 {{lv::for_each_field is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>}}
auto by_field = lv::for_each_field<track>([]<auto F> { return lv::label(); });

// expected-error@+1 {{lv::theme is in <lv/theme.hpp>: add #include <lv/theme.hpp>}}
auto styles = lv::theme({});

// expected-error@*:* {{lv::look is in <lv/look.hpp>: add #include <lv/look.hpp>}}
auto card = lv::look<"queue.card">;

// expected-error@*:* {{lv::image_file is in <lv/image.hpp>: add #include <lv/image.hpp>}}
auto picture = lv::image_file<png>;

// expected-error@*:* {{lv::encoded_image is in <lv/encoded_image.hpp>: add #include <lv/encoded_image.hpp>}}
auto encoded = lv::encoded_image<png>;

// expected-error@*:* {{lv::runtime is in <lv/runtime.hpp>: add #include <lv/runtime.hpp>}}
lv::runtime<"obj", "label"> document;

// expected-error@*:* {{lv::one_of is in <lv/one_of.hpp>: add #include <lv/one_of.hpp>}}
lv::one_of<a, b> slot;

// expected-error@*:* {{lv::pointer is in <lv/pointer.hpp>: add #include <lv/pointer.hpp>}}
void mouse(lv::display& display) { lv::pointer pointer{display}; }

constinit lv::state<mode> current_mode{mode::a};
constinit lv::state<bool> flag{false};

// expected-error@+1 {{lv::shown is in <lv/shown.hpp>: add #include <lv/shown.hpp>}}
auto by_mode_node = lv::shown(current_mode);

// expected-error@+1 {{lv::when is in <lv/shown.hpp>: add #include <lv/shown.hpp>}}
auto while_flag = lv::when(flag);

// expected-error@*:* {{lv::is<...>(tree) is an arm of lv::shown, in <lv/shown.hpp>: add #include <lv/shown.hpp>}}
auto an_arm = lv::is<mode::a>(a);

}  // namespace missing_header
