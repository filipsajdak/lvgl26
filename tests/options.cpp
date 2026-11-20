// Lists of options for a dropdown and a roller: `.options = {"a", "b", "c"}` joined by the
// compiler, lv::options_of<E>, and `.selected` following a state of the enum.

#include <lv/lv.hpp>
#include <lv/runtime.hpp>

#include "check.hpp"

#include <cstring>
#include <string_view>

namespace {

enum class quality { best, medium, draft };

// An enum whose values are not 0, 1, 2: the place of an option is not its value.
enum class level : int { low = 10, mid = 20, high = 30 };

enum class sheet { a4, letter, legal, a5 };

}  // namespace

// The texts of an enum's options, given instead of the names of its enumerators.
template <>
struct lv::option_text<sheet> {
  static consteval const char* of(sheet s) {
    switch (s) {
      case sheet::a4: return "A4";
      case sheet::letter: return "Letter";
      case sheet::legal: return "Legal";
      case sheet::a5: return "A5";
    }
    return "";
  }
};

namespace {

using namespace std::string_view_literals;

// ---- what the compiler makes ----------------------------------------------------------

constexpr lv::option_list sizes{"100%", "75%", "50%"};
static_assert(std::string_view{sizes.text} == "100%\n75%\n50%");
static_assert(sizes.kept && sizes.enum_name == nullptr && sizes.count == 0);

// A single text stays what it was: LVGL copies it.
constexpr lv::option_list one_text{"a\nb"};
static_assert(std::string_view{one_text.text} == "a\nb" && !one_text.kept);

constexpr auto qualities = lv::options_of<quality>;
static_assert(std::string_view{qualities.text} == "best\nmedium\ndraft");
static_assert(qualities.count == 3 && qualities.kept);
static_assert(qualities.values[0] == 0 && qualities.values[2] == 2);
constexpr std::string_view tag{qualities.enum_name};
static_assert(tag.ends_with("quality"));

constexpr auto without_draft = lv::options_of<quality, lv::but<quality::draft>>;
static_assert(std::string_view{without_draft.text} == "best\nmedium" && without_draft.count == 2);
constexpr auto only_medium = lv::options_of<quality, lv::but<quality::best, quality::draft>>;
static_assert(std::string_view{only_medium.text} == "medium" && only_medium.values[0] == 1);

constexpr auto levels = lv::options_of<level>;
static_assert(levels.values[0] == 10 && levels.values[1] == 20 && levels.values[2] == 30);
static_assert(levels.index_of(20) == 1 && levels.index_of(30) == 2);
static_assert(levels.index_of(21) == levels.count);  // not in the list

// A place in the list is not a value, and an excluded value has no place.
constexpr auto without_mid = lv::options_of<level, lv::but<level::mid>>;
static_assert(without_mid.index_of(30) == 1 && without_mid.index_of(20) == without_mid.count);

// The texts of an enum come from lv::option_text.
constexpr auto sheets = lv::options_of<sheet, lv::but<sheet::a5>>;
static_assert(std::string_view{sheets.text} == "A4\nLetter\nLegal");

// Two enums of one name in different places are different enums to a binding.
namespace first_place {
enum class mode { a };
}
namespace second_place {
enum class mode { a };
}
static_assert(std::string_view{lv::options_of<first_place::mode>.enum_name} !=
              std::string_view{lv::options_of<second_place::mode>.enum_name});

// Which setters take a list is read from LVGL's doc comments, not written here.
static_assert(lv::detail::takes_option_list(^^lv_dropdown_set_options, "options"));
static_assert(!lv::detail::takes_option_list(^^lv_dropdown_set_options_static, "options"));  // the text a list is passed as
static_assert(lv::detail::takes_option_list(^^lv_roller_set_options, "options"));
static_assert(!lv::detail::takes_option_list(^^lv_dropdown_set_text, "txt"));
static_assert(!lv::detail::takes_option_list(^^lv_label_set_text, "text"));

// The property of a dropdown is a list; the one of a label is a text.
static_assert(std::same_as<decltype(lv::props<"dropdown">{}.options), lv::arg<lv::option_list>>);
static_assert(std::same_as<decltype(lv::props<"roller">{}.options), lv::arg<lv::option_list, lv_roller_mode_t>>);
static_assert(std::same_as<decltype(lv::props<"label">{}.text), lv::arg<const char*>>);

// ---- on a screen ---------------------------------------------------------------------

constinit lv::state<quality> chosen{quality::medium};
constinit lv::state<level> grade{level::high};
struct place {
  std::int32_t index;
};
constinit lv::state<place> where{place{1}};

constexpr auto ui = lv::group({})[
    lv::dropdown({.id = "size", .options = {"100%", "75%", "50%"}}),
    lv::dropdown({.id = "quality", .options = lv::options_of<quality>, .selected = lv::bind(chosen)}),
    lv::dropdown({.id = "reduced", .options = lv::options_of<quality, lv::but<quality::medium>>,
                  .selected = lv::bind(chosen)}),
    lv::roller({.id = "level", .options = {lv::options_of<level>, "normal"}, .selected = lv::bind(grade)}),
    lv::dropdown({.id = "where", .options = lv::options_of<quality>, .selected = lv::bind(where)}),
    lv::roller({.id = "week", .options = {{"Mon", "Tue", "Wed"}, "infinite"}}),
    lv::roller({.id = "days", .options = {"Mon\nTue", "normal"}}),
    lv::dropdown({.id = "plain", .options = "x\ny"})];

constinit lv::frame<320, 240> frame;

std::string_view options_of_widget(lv_obj_t* o) { return lv_dropdown_get_options(o); }

void a_list_is_the_text_of_lvgl(auto& root) {
  auto size = root.template get<"size">();
  check(options_of_widget(size.raw()) == "100%\n75%\n50%");
  check(lv_dropdown_get_option_count(size.raw()) == 3);
  char text[8]{};
  lv_dropdown_set_selected(size.raw(), 1);
  lv_dropdown_get_selected_str(size.raw(), text, sizeof text);
  check(std::string_view{text} == "75%");

  // An enum's options are the names of its enumerators; LVGL keeps the very text.
  auto quality_box = root.template get<"quality">();
  check(options_of_widget(quality_box.raw()) == "best\nmedium\ndraft");
  check(lv_dropdown_get_options(quality_box.raw()) == qualities.text);

  auto plain = root.template get<"plain">();
  check(options_of_widget(plain.raw()) == "x\ny");
  check(lv_dropdown_get_option_count(plain.raw()) == 2);
}

void a_roller_takes_a_list_and_a_mode(auto& root) {
  auto week = root.template get<"week">();
  check(lv_roller_get_option_count(week.raw()) == 3);
  // An infinite roller repeats its options; a normal one does not.
  check(std::string_view{lv_roller_get_options(week.raw())}.starts_with("Mon\nTue\nWed\nMon"));
  auto days = root.template get<"days">();
  check(lv_roller_get_option_count(days.raw()) == 2);
  check(std::string_view{lv_roller_get_options(days.raw())} == "Mon\nTue");
}

// The state shows in the widget, and the user's choice is written to the state.
void an_enum_state_follows_the_choice(auto& root) {
  auto box = root.template get<"quality">();
  check(lv_dropdown_get_selected(box.raw()) == 1);  // medium
  chosen = quality::draft;
  check(lv_dropdown_get_selected(box.raw()) == 2);
  chosen = quality::best;
  check(lv_dropdown_get_selected(box.raw()) == 0);

  lv_dropdown_set_selected(box.raw(), 2);
  lv_obj_send_event(box.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(chosen.get() == quality::draft);
  // Another widget on the same state follows it as well.
  check(lv_dropdown_get_selected(root.template get<"quality">().raw()) == 2);
}

// An enumerator that was left out has no place: the widget keeps what it showed. Places count
// the options that are there, not the enumerators.
void a_left_out_enumerator_is_not_shown(auto& root) {
  auto reduced = root.template get<"reduced">();  // best, draft
  check(options_of_widget(reduced.raw()) == "best\ndraft");
  chosen = quality::draft;
  check(lv_dropdown_get_selected(reduced.raw()) == 1);
  chosen = quality::medium;
  check(lv_dropdown_get_selected(reduced.raw()) == 1);  // unchanged
  chosen = quality::best;
  check(lv_dropdown_get_selected(reduced.raw()) == 0);

  lv_dropdown_set_selected(reduced.raw(), 1);
  lv_obj_send_event(reduced.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(chosen.get() == quality::draft);  // place 1 is draft, not medium
}

// The place of an option and the value of its enumerator are two things.
void values_are_not_places(auto& root) {
  auto level_roller = root.template get<"level">();
  check(lv_roller_get_selected(level_roller.raw()) == 2);  // high
  grade = level::low;
  check(lv_roller_get_selected(level_roller.raw()) == 0);
  lv_roller_set_selected(level_roller.raw(), 1, LV_ANIM_OFF);
  lv_obj_send_event(level_roller.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(grade.get() == level::mid);
}

// A state that holds an index keeps working with a list made from an enum.
void an_index_state_still_works(auto& root) {
  auto box = root.template get<"where">();
  check(lv_dropdown_get_selected(box.raw()) == 1);
  where = place{2};
  check(lv_dropdown_get_selected(box.raw()) == 2);
  lv_dropdown_set_selected(box.raw(), 0);
  lv_obj_send_event(box.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(where.get().index == 0);
}

// A list typed at run time works as before, and is copied by LVGL.
void a_text_made_later_is_copied(auto& root) {
  auto plain = root.template get<"plain">();
  char text[] = "p\nq\nr";
  plain.set({.options = text});
  text[0] = 'X';
  check(options_of_widget(plain.raw()) == "p\nq\nr");
  plain.set({.options = lv::options_of<sheet>});
  check(options_of_widget(plain.raw()) == "A4\nLetter\nLegal\nA5");
  plain.set({.options = {"one", "two"}});
  check(options_of_widget(plain.raw()) == "one\ntwo");
}

// A document gives the options as a text, which is copied as every document text is. (The
// reader keeps an escape such as \n as it is written, so the text here has no line break.)
void a_document_sets_options(lv::obj screen) {
  using doc = lv::runtime<"obj", "dropdown", "roller">;
  auto loaded = doc::load(R"({"type": "obj", "children": [
      {"type": "dropdown", "id": "d", "props": {"options": "Apple"}},
      {"type": "roller", "id": "r", "props": {"options": ["Mon", "infinite"]}}]})", screen);
  check(loaded.has_value());
  if (!loaded) return;
  check(options_of_widget(loaded->find("d").raw()) == "Apple");
  check(std::string_view{lv_roller_get_options(loaded->find("r").raw())}.starts_with("Mon\nMon"));
}

// ---- heap ----------------------------------------------------------------------------

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

template <const auto& Tree>
std::size_t mount_cost(lv::display& display) {
  const auto before = used_bytes();
  std::size_t cost = 0;
  {
    auto root = lv::mount<Tree>(display.screen());
    check(root.has_value());
    cost = used_bytes() - before;
  }
  check(used_bytes() == before);
  return cost;
}

constexpr auto long_list = lv::dropdown({.options = {"A rather long first option", "A rather long second option",
                                                     "A rather long third option", "A rather long fourth option",
                                                     "A rather long fifth option"}});
constexpr auto no_list = lv::dropdown({});
constexpr auto long_roller = lv::roller({.options = {{"A rather long first option", "A rather long second option",
                                                      "A rather long third option", "A rather long fourth option",
                                                      "A rather long fifth option"},
                                                     "normal"}});
constexpr auto no_roller_list = lv::roller({});

// A dropdown with a list takes no heap for it; a roller copies it into its label.
void a_static_list_adds_no_copy(lv::display& display) {
  const auto without = mount_cost<no_list>(display);
  const auto with = mount_cost<long_list>(display);
  check(with <= without);
  const auto roller_without = mount_cost<no_roller_list>(display);
  const auto roller_with = mount_cost<long_roller>(display);
  check(roller_with > roller_without);
}

}  // namespace

// A state of an enum follows the choice in a constexpr tree. Given to set() it is a contract
// violation: nothing in read-only memory holds what the observer needs.
int set_with_an_enum_state() {
  lv::application app;
  lv::display display{app, frame};
  auto box = display.screen().add<"dropdown">();
  box.set({.options = lv::options_of<quality>, .selected = lv::bind(chosen)});
  return 0;
}

int main(int argc, char**) {
  if (argc == 2) {
    std::printf("no contract violation: %d\n", set_with_an_enum_state());
    return 0;
  }
  lv::application app;
  lv::display display{app, frame};
  {
    auto root = lv::mount<ui>(display.screen());
    check(root.has_value());
    if (root) {
      a_list_is_the_text_of_lvgl(*root);
      a_roller_takes_a_list_and_a_mode(*root);
      an_enum_state_follows_the_choice(*root);
      a_left_out_enumerator_is_not_shown(*root);
      values_are_not_places(*root);
      an_index_state_still_works(*root);
      a_text_made_later_is_copied(*root);
    }
  }
  a_document_sets_options(display.screen());
  a_static_list_adds_no_copy(display);
  chosen.reset();
  grade.reset();
  where.reset();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
