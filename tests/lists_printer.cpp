// The print queue of the printer port: the column on the home page and the list of the Print
// page are lv::each over one lv::list (examples/ports/slint/printer/queue.hpp). A card has one
// tree for both looks, a tap opens it, and the Delete button removes the job of its own card.

#include "check.hpp"

#include <lv/lv.hpp>
#include <queue.hpp>
#include <scenes.hpp>
#include <shell.hpp>

#include <cstring>
#include <vector>

namespace {

using namespace printer;

constinit lv::frame<window_width, window_height> frame;

// The widgets under `w` for which `wanted` is true, in the order they are made.
template <class P>
void find(lv_obj_t* w, P wanted, std::vector<lv_obj_t*>& out) {
  if (wanted(w)) out.push_back(w);
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(w); ++i) find(lv_obj_get_child(w, static_cast<std::int32_t>(i)), wanted, out);
}

template <class P>
std::vector<lv_obj_t*> found(lv::obj root, P wanted) {
  std::vector<lv_obj_t*> out;
  find(root.raw(), wanted, out);
  return out;
}

// The cards of the home page are the checkable widgets of the panel.
std::vector<lv_obj_t*> home_cards(lv::obj screen) {
  return found(screen, [](lv_obj_t* w) { return lv_obj_is_checkable(w); });
}

lv_obj_t* dot_of(lv_obj_t* card) { return lv_obj_get_child(lv_obj_get_child(card, 0), 0); }
lv_obj_t* details_of(lv_obj_t* card) { return lv_obj_get_child(lv_obj_get_child(card, 1), 1); }

int details_height(lv_obj_t* card) {
  lv_obj_update_layout(card);
  return lv_obj_get_height(details_of(card));
}

// The printing card is checked in its dot, and a waiting one is not. Both are one template.
void one_template_has_both_looks(lv::obj screen) {
  const auto cards = home_cards(screen);
  check(cards.size() == queue.size() && cards.size() >= 2);
  check(lv_obj_has_state(dot_of(cards[0]), LV_STATE_CHECKED) && !lv_obj_has_state(dot_of(cards[1]), LV_STATE_CHECKED));
  check(!lv_color_eq(lv_obj_get_style_bg_color(dot_of(cards[0]), LV_PART_MAIN),
                     lv_obj_get_style_bg_color(dot_of(cards[1]), LV_PART_MAIN)));
  // The job starts printing: its dot turns, and its card does not open.
  queue.update(1, [](job& j) { j.printing = true; });
  check(lv_obj_has_state(dot_of(cards[1]), LV_STATE_CHECKED) && !lv_obj_has_state(cards[1], LV_STATE_CHECKED));
  queue.update(1, [](job& j) { j.printing = false; });
}

// A tap opens the details of that card alone, and a second tap closes them.
void a_tap_opens_one_card(lv::obj screen) {
  const auto cards = home_cards(screen);
  check(details_height(cards[1]) == 0 && details_height(cards[0]) == 0);
  lv_obj_send_event(cards[1], LV_EVENT_RELEASED, nullptr);
  check(lv_obj_has_state(cards[1], LV_STATE_CHECKED));
  check(details_height(cards[1]) > 0 && details_height(cards[0]) == 0);
  lv_obj_send_event(cards[1], LV_EVENT_RELEASED, nullptr);
  check(details_height(cards[1]) == 0);
}

// The Delete button of a card on the Print page removes the job of that card, and no other card is touched.
void delete_removes_its_own_job(lv::obj screen) {
  std::vector<lv_obj_t*> buttons;
  for (lv_obj_t* label : found(screen, [](lv_obj_t* w) {
         return lv_obj_check_type(w, &lv_label_class) && std::strcmp(lv_label_get_text(w), "Delete") == 0;
       }))
    buttons.push_back(lv_obj_get_parent(lv_obj_get_parent(label)));  // the label is in a row in the button
  check(buttons.size() == queue.size());
  const std::size_t before = queue.size();
  const char* second = queue[1].title;
  lv::weak<lv::obj> first_button{lv::obj{buttons[0]}};
  lv::weak<lv::obj> third_button{lv::obj{buttons[2]}};
  lv::weak<lv::obj> second_button{lv::obj{buttons[1]}};
  lv_obj_send_event(buttons[1], LV_EVENT_CLICKED, nullptr);
  check(queue.size() == before - 1 && queue[1].title != second);
  check(!first_button.expired() && !third_button.expired() && second_button.expired());
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  const lv::obj screen = display.screen();
  check(start(screen));
  play(app, scenes[0]);
  one_template_has_both_looks(screen);
  a_tap_opens_one_card(screen);
  play(app, scenes[3]);
  delete_removes_its_own_job(screen);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
