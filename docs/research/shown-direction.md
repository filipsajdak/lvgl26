# Direction-aware page slides for lv::shown

Status: design, nothing built. Marks: `source` is read in `include/lv/shown.hpp`, `reasoned` is mine.

## The problem

`lv::shown(current, {.enter = slide_in, .leave = slide_out})` has one enter and one leave transition. Every change of the state plays them: a page always comes in from the right (`slide_in` = `{.x = {320 * px, 0 * px}}`), and the page that goes plays `slide_out`. Going back must come in from the left. `source`: `shown_options` and `shown_plan` hold one `transition_spec` each for enter and leave, and `switch_with_transition` / `start_enter` / `start_leave` choose between them by nothing but "entering" or "leaving".

## What the engine knows

- `show_arm(c, k)` knows the arm that was shown (`c.current`) and the one asked for (`k`), but arms are the unit of the engine, not values: an arm may show several values (`lv::is<settings, ink>(tree)`), and the direction between two **values** of one arm is not a switch at all (the page is kept).
- `start_leave(slot)` and `start_enter(slot, backwards)` read `plan.leave` and `plan.enter`. A page that turns round (asked for again while it leaves, a change during an enter) continues from where it stands when the two transitions move the same property (`turning_from`), so a pair of transitions for two directions that move the same property (`x`) keeps working: the page goes back where it stands. `source`
- `.on_shown` and `.on_front` belong to the arm and run when a page is built or comes to the front; they know nothing of the transition. `source`

## The API: what I propose, and the alternatives

**A (proposed): the direction is the order of the state's values; two more transitions for the way back.**

```cpp
constexpr auto slide_in   = lv::transition({.x = {320 * px, 0 * px}}, 220 * ms, lv::ease::out_quad);
constexpr auto slide_out  = lv::transition({.x = {0 * px, -320 * px}}, 220 * ms, lv::ease::out_quad);
constexpr auto back_in    = lv::transition({.x = {-320 * px, 0 * px}}, 220 * ms, lv::ease::out_quad);
constexpr auto back_out   = lv::transition({.x = {0 * px, 320 * px}}, 220 * ms, lv::ease::out_quad);

lv::shown(current, {.enter = slide_in, .leave = slide_out, .enter_back = back_in, .leave_back = back_out})[...]
```

A change from value `a` to value `b` is *forward* when the enumerator of `b` comes after the one of `a` in the enum, and *back* when it comes before. Forward plays `.enter` and `.leave`, back plays `.enter_back` and `.leave_back`. `.enter_back` and `.leave_back` left out are `.enter` and `.leave` (a program that sets none behaves as today). The enum is the order: `enum class page { welcome, drinks, size, brewing, enjoy }`, as the pages of a wizard are numbered. The user writes nothing else: `current = page::drinks;` slides forward, `current = page::welcome;` slides back; `lv::first`, `lv::match` and the state are used as before.

**B: a transition for an arm.** `lv::is<page::service>(service_ui, {.enter = from_top, .leave = to_top})`. The arriving arm's `.enter` and the leaving arm's `.leave` beat the options' (by direction). It covers a page that does not slide in a line with the others (the Service page comes down from the top, whichever page it was reached from) and it needs no order. It does not say "back" for the pages in a line, so it is not enough alone. Proposed **with A**: the arm's own transition, when it has one, beats the options'.

**C: a rule given by a function.** `.back = &is_back` with `bool is_back(page from, page to)`: any rule (a wizard that loops, a tab bar that goes by distance). The engine is no template: the function would take the two values as numbers (the position of the enumerator), and the user writes the comparison. More general and less written down; the order of an enum is the rule that nearly every pager follows, so A first and C when a case needs it (it costs two function pointers in the plan and nothing for a program that does not set it).

**D: the caller says it.** `lv::go(current, page::drinks, lv::forward)`: the state would no longer be the whole interface (a swipe handler, a task and C code write the state); rejected.

## Rules I would build

- The direction is read from the values, not the arms: the plan keeps a function that gives the position of the state's current value among the enumerators (`rank_now(source)`), and the core keeps the rank of the value that was shown. A bool state (`lv::when`) has no direction.
- Two arms with `lv::otherwise()`: its values rank by their enumerators like the others.
- A change while a page enters or leaves is judged from the value that was shown last, not from where the page stands. The page that turns round continues from where it stands as today: the spec of the new direction runs from `turning_from`.
- `.alive` above 2 and kept pages: a kept page that comes back uses the direction of the change like a built one.
- `lv::transition` is unchanged; there is no `lv::slide`. A helper that writes the four transitions of a slide from a width and a time (`lv::slides(320 * px, 220 * ms, ease)`) is a convenience for a later change.

## The finger-following page (gap 5, v0.2): not blocked

A page that follows the finger needs, for a drag, the transition of the direction of the neighbour that is being dragged to (the previous or the next value) and its distance. With A the plan already holds the four specs by direction and the engine knows the direction by the order of the values, so a drag picks `enter` or `enter_back` for the neighbour, sets the page's property along the finger, and on release runs the same spec from where the finger left it (`run_transition(slot, spec, from)`, which exists). The neighbour is "the next or the previous value in the enum", the same order. B and C do not get in its way: B names the spec of the arm; C would tell which neighbour is "back".

## Cost

- Run time: two `transition_spec` (about 32 bytes each) more in the options and in the plan of each `lv::shown`, in read-only memory; two function pointers and one rank in the core (no allocation).
- Compile time: `shown.hpp` is opt-in; the plan is a constant of the node. `lv::transition` is unchanged. A program with no back transitions pays the four more members of two structs.

## Questions for the team lead

1. Direction by the **enumerators' order** (A), or by the order in which the arms are written in `lv::shown(...)[...]`? I propose the enumerators: an arm may hold several values, so the order of arms is not the order of values.
2. Do you want B (the arm's own `.enter`/`.leave`) in this change? The coffee mock-up needs it for the Service page (down from the top). My proposal: yes, with A.
3. C (a function that says "back") now or later? My proposal: later.
4. Names: `.enter_back` and `.leave_back`. Fine, or `.back_enter` / `.back_leave`?
