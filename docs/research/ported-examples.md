# LVGL's widget examples: what is ported, what can be, and what the others need

Researched on 2026-11-22, against LVGL v9.6.0 (`examples/widgets`) and `main` at `cb0974a`.
Marks follow [README.md](README.md). Every row below is `source`: the C file was read, with lvgl26's reference data and headers. Nothing was compiled.

## Short answers

| Question | Answer |
|---|---|
| How many widget examples has LVGL v9.6.0? | 163, in 34 folders |
| How many are ported? | 69, in `examples/lvgl/widgets/`, each compared pixel for pixel with its C original |
| How many more can be ported today? | 7: `checkbox_styling`, `roller_event`, `roller_styling`, `roller_visible_rows`, `slider_styling`, `spinner_basic`, `switch_styling` |
| What stops the other 87? | A form that lvgl26 does not have yet; the table "The blockers" counts them |
| Which one feature would open the most? | A child made by an `add_` function: 36 examples (charts, tab views, menus, message boxes, calendars, lists, spans, scale sections, a tile view, a window) |

## How the list was made

- Each `lv_example_<folder>_<name>.c` of LVGL v9.6.0 was matched with `examples/lvgl/widgets/<folder>_<name>.cpp`.
- For each one without a port, every call to LVGL was listed. A create or set function has a form in lvgl26; any other call needs one. What a set function takes is in `docs/reference/lvgl-v9.6.0.json`.
- Three facts of lvgl26's own code decide several rows:
  - A node sets its properties in the order of the members of its props struct, which is alphabetical (`apply_node` in `include/lv/tree.hpp`). LVGL needs some setters in another order.
  - A node sets a property once. A setter that LVGL calls several times with different arguments, such as a table's cells, has one value per node.
  - A property that takes a whole number cannot take a percentage, which LVGL writes `LV_PCT(80)`.
- The examples of `list`, `menu` and `win` are not in the v10 pin. A port is compared on both LVGL versions, so these would need a test on v9.6.0 only.

## The examples without a port

| Widget | Example | Can it be ported today? | What it needs |
|---|---|---|---|
| animimg | `animimg_play` | No: a call | `lv_animimg_start` starts the animation; it is no set function |
| arc | `arc_knob_widget` | No: a call | `lv_arc_rotate_obj_to_angle` places another widget at the knob's angle |
| arclabel | `arclabel_curved_text` | No: a feature of its own | `.radius` takes a whole number, and the example gives `LV_PCT(80)`: a percentage has no form there |
| buttonmatrix | `buttonmatrix_button_ctrl` | No: one setter, several calls | A different control for each button: `.ctrl_map` takes an array of C enumerators, and `.button_ctrl` is set once |
| buttonmatrix | `buttonmatrix_button_width` | No: one setter, several calls | A different width for three buttons: `.button_width` is set once |
| buttonmatrix | `buttonmatrix_one_checked` | No: the order of setters | `.button_ctrl_all` is set before `.map`, and a new map clears the controls |
| buttonmatrix | `buttonmatrix_popover` | No: the order of setters | The same: the controls must follow the map |
| buttonmatrix | `buttonmatrix_recolor` | No: the order of setters | The same: the controls must follow the map |
| buttonmatrix | `buttonmatrix_styling` | No: the order of setters | The same: the controls must follow the map |
| calendar | `calendar_basic` | No: an added child | The header is made by `lv_calendar_add_header_arrow` |
| calendar | `calendar_chinese` | No: an added child | `lv_calendar_add_header_dropdown`, and `LV_USE_CALENDAR_CHINESE`, off in the test build |
| calendar | `calendar_highlighted_days` | No: an added child | The header, and `.highlighted_dates`, which takes an array of C structs |
| canvas | `canvas_alpha` | No: drawing | `lv_canvas_fill_bg` and pixels set one by one |
| canvas | `canvas_arc` | No: drawing | `lv_draw_arc` on a canvas layer |
| canvas | `canvas_blur` | No: drawing | `lv_draw_blur`, `lv_draw_fill` and `lv_draw_label` on a canvas layer |
| canvas | `canvas_image` | No: drawing | `lv_draw_image` on a canvas layer |
| canvas | `canvas_label` | No: drawing | `lv_draw_label` on a canvas layer |
| canvas | `canvas_line` | No: drawing | `lv_draw_line` on a canvas layer |
| canvas | `canvas_rectangle` | No: drawing | `lv_draw_rect` on a canvas layer |
| canvas | `canvas_rotate` | No: drawing | `lv_draw_image` of a draw buffer, rotated |
| canvas | `canvas_triangle` | No: drawing | `lv_draw_triangle` on a canvas layer |
| canvas | `canvas_vector` | No: drawing | Vector paths (`lv_vector_path_*`, `lv_draw_vector`) |
| canvas | `canvas_wave_text` | No: drawing | `lv_draw_letter` from a timer |
| canvas | `canvas_windstorm_text` | No: drawing | `lv_draw_letter` from a timer |
| chart | `chart_area_gradient` | No: an added child | Series by `lv_chart_add_series`, and a draw-task event |
| chart | `chart_circular_gap` | No: an added child | Series by `lv_chart_add_series`, refreshed from a timer |
| chart | `chart_cursor` | No: an added child | Series and a cursor by `lv_chart_add_series` and `lv_chart_add_cursor` |
| chart | `chart_pressed_tooltip` | No: an added child | Series, and a tooltip drawn in `LV_EVENT_DRAW_POST_END` |
| chart | `chart_recolor_bars` | No: an added child | Series, and bars recolored in a draw-task event |
| chart | `chart_scatter` | No: an added child | Series, and points recolored in a draw-task event |
| chart | `chart_scrollable` | No: an added child | Series by `lv_chart_add_series` |
| chart | `chart_series` | No: an added child | Series by `lv_chart_add_series` |
| chart | `chart_ticks_labels` | No: an added child | Series by `lv_chart_add_series` |
| chart | `chart_types` | No: an added child | Series by `lv_chart_add_series` |
| checkbox | `checkbox_styling` | Now | The indicator in the checked state is `lv::on("indicator", "checked", {...})` |
| dropdown | `dropdown_styling` | No: a feature of its own | The style of the open list, a widget of its own reached by `lv_dropdown_get_list` |
| image | `image_recolor` | No: a feature of its own | A style property bound to a state (`lv_obj_bind_style_prop`) |
| image | `image_svg` | No: a build option | The SVG decoder, `LV_USE_SVG` |
| imagebutton | `imagebutton_transition` | No: a feature of its own | A style transition: `.transition` takes LVGL's descriptor, and a transition token sets only the duration |
| keyboard | `keyboard_custom_draw` | No: drawing | The keys are drawn again in a draw-task event |
| keyboard | `keyboard_custom_map` | No: a widget as a pointer | `.map` with an array of C control enumerators, and `.textarea`, which takes the text area as a pointer |
| label | `label_translation` | No: a feature of its own | LVGL's translation tables (`lv_translation_*`) |
| list | `list_reorder` | No: an added child | Buttons by `lv_list_add_button`; not in the v10 pin |
| list | `list_sections` | No: an added child | Texts and buttons by `lv_list_add_text` and `lv_list_add_button`; not in the v10 pin |
| lottie | `lottie_approve` | No: a build option | `LV_USE_LOTTIE`, which needs ThorVG |
| lottie | `lottie_from_data` | No: a build option | `LV_USE_LOTTIE`, which needs ThorVG |
| lottie | `lottie_from_file` | No: a build option | `LV_USE_LOTTIE`, which needs ThorVG |
| menu | `menu_custom_back_button` | No: an added child | Pages by `lv_menu_page_create(menu, title)`; not in the v10 pin |
| menu | `menu_floating_button` | No: an added child | Pages by `lv_menu_page_create(menu, title)`; not in the v10 pin |
| menu | `menu_navigation` | No: an added child | Pages by `lv_menu_page_create(menu, title)`; not in the v10 pin |
| menu | `menu_root_back_button` | No: an added child | Pages, and a message box; not in the v10 pin |
| menu | `menu_sidebar` | No: an added child | Pages, sections and separators; not in the v10 pin |
| menu | `menu_sub_page` | No: an added child | Pages by `lv_menu_page_create(menu, title)`; not in the v10 pin |
| msgbox | `msgbox_blur` | No: an added child | Title and text by `lv_msgbox_add_title` and `lv_msgbox_add_text` |
| msgbox | `msgbox_modal` | No: an added child | Title, text and buttons by `lv_msgbox_add_*` |
| msgbox | `msgbox_settings` | No: an added child | Title and buttons by `lv_msgbox_add_*` |
| obj | `obj_draggable` | No: a feature of its own | The motion of the input device (`lv_indev_get_vect`) |
| obj | `obj_transform` | No: a build option | A transform matrix (`lv_matrix_*`), with `LV_DRAW_TRANSFORM_USE_MATRIX` |
| roller | `roller_event` | Now | The label follows the selected day through a state of an enum and a computed text |
| roller | `roller_fade_mask` | No: drawing | A fading mask drawn on a canvas layer |
| roller | `roller_styling` | Now | The selected part's style; its font is the C font `font_example_large`, as `lv::raw(...)` |
| roller | `roller_visible_rows` | Now | Two rollers with a different count of visible rows |
| scale | `scale_bind_section` | No: an added child | A section by `lv_scale_add_section`, bound to sliders |
| scale | `scale_compass` | No: drawing | Labels recolored in a draw-task event, and an `lv_anim` |
| scale | `scale_heart_rate` | No: an added child | Sections by `lv_scale_add_section` |
| scale | `scale_label_recolor` | No: drawing | Labels recolored in a draw-task event |
| scale | `scale_needles` | No: a widget as a pointer | `.line_needle_value` takes the needle widget as a pointer, and an `lv_anim` moves it |
| scale | `scale_sunset_sunrise` | No: an added child | Sections by `lv_scale_add_section`, and a draw-task event |
| slider | `slider_styling` | Now | Styles of the track, the indicator and the knob, and of the knob while pressed |
| span | `span_hittest` | No: an added child | Spans by `lv_spangroup_add_span` |
| span | `span_styling` | No: an added child | Spans by `lv_spangroup_add_span` |
| spinbox | `spinbox_with_buttons` | No: a call | `lv_obj_align_to` and `lv_spinbox_increment` from the buttons' handlers |
| spinner | `spinner_basic` | Now | Two spinners with a duration and a sweep each |
| spinner | `spinner_styling` | No: one setter, several calls | In LVGL v9.6.0 this example builds a table, whose cells need `.cell_value` six times |
| switch | `switch_styling` | Now | Styles of the track, the knob, and the indicator while checked |
| table | `table_cells` | No: one setter, several calls | Several cells: `.cell_value` takes a row, a column and a text, and is set once |
| table | `table_file_browser` | No: one setter, several calls | Several cells, and files read with `lv_fs_*` |
| table | `table_merge_cells` | No: one setter, several calls | Several cells and cell controls |
| table | `table_rows_columns` | No: one setter, several calls | Several cells |
| table | `table_scroll` | No: one setter, several calls | Several cells |
| table | `table_scrollable_list` | No: one setter, several calls | Several cells, and a draw-task event |
| table | `table_styling` | No: one setter, several calls | Several cells |
| table | `table_width` | No: one setter, several calls | Several cells and column widths |
| tabview | `tabview_active_tab` | No: an added child | Tabs by `lv_tabview_add_tab` |
| tabview | `tabview_basic` | No: an added child | Tabs by `lv_tabview_add_tab` |
| tabview | `tabview_tab_bar_position` | No: an added child | Tabs by `lv_tabview_add_tab` |
| tabview | `tabview_tab_bar_style` | No: an added child | Tabs, and the tab bar reached by `lv_tabview_get_tab_bar` |
| tabview | `tabview_tab_button_content` | No: an added child | Tabs, and a tab's button reached by `lv_tabview_get_tab_button` |
| textarea | `textarea_buttonmatrix` | No: a call | `lv_textarea_add_text` and `lv_textarea_delete_char` from a button matrix |
| textarea | `textarea_cursor` | No: the order of setters | `.cursor_pos` is set before `.text`, and setting the text moves the cursor to its end |
| textarea | `textarea_format` | No: a call | `lv_textarea_add_char` from an event handler |
| textarea | `textarea_password` | No: a widget as a pointer | A keyboard joined to the text area: `.textarea` takes it as a pointer |
| tileview | `tileview_l_shape` | No: an added child | Tiles by `lv_tileview_add_tile` |
| win | `win_toolbar` | No: an added child | Title and buttons by `lv_win_add_title` and `lv_win_add_button`; not in the v10 pin |

## The blockers

| Blocker | Examples | Which |
|---|---|---|
| A child made by an `add_` function, or by a create function that takes more than a parent | 36 | `calendar_basic`, `calendar_chinese`, `calendar_highlighted_days`, `chart_area_gradient`, `chart_circular_gap`, `chart_cursor`, `chart_pressed_tooltip`, `chart_recolor_bars`, `chart_scatter`, `chart_scrollable`, `chart_series`, `chart_ticks_labels`, `chart_types`, `list_reorder`, `list_sections`, `menu_custom_back_button`, `menu_floating_button`, `menu_navigation`, `menu_root_back_button`, `menu_sidebar`, `menu_sub_page`, `msgbox_blur`, `msgbox_modal`, `msgbox_settings`, `scale_bind_section`, `scale_heart_rate`, `scale_sunset_sunrise`, `span_hittest`, `span_styling`, `tabview_active_tab`, `tabview_basic`, `tabview_tab_bar_position`, `tabview_tab_bar_style`, `tabview_tab_button_content`, `tileview_l_shape`, `win_toolbar` |
| Drawing: a canvas layer (`lv_canvas_init_layer`, `lv_draw_*`) or a draw-task event | 16 | `canvas_alpha`, `canvas_arc`, `canvas_blur`, `canvas_image`, `canvas_label`, `canvas_line`, `canvas_rectangle`, `canvas_rotate`, `canvas_triangle`, `canvas_vector`, `canvas_wave_text`, `canvas_windstorm_text`, `keyboard_custom_draw`, `roller_fade_mask`, `scale_compass`, `scale_label_recolor` |
| One setter called several times with different arguments: a node sets a property once | 11 | `buttonmatrix_button_ctrl`, `buttonmatrix_button_width`, `spinner_styling`, `table_cells`, `table_file_browser`, `table_merge_cells`, `table_rows_columns`, `table_scroll`, `table_scrollable_list`, `table_styling`, `table_width` |
| The order of the setters: properties are set in alphabetical order, and LVGL needs another | 5 | `buttonmatrix_one_checked`, `buttonmatrix_popover`, `buttonmatrix_recolor`, `buttonmatrix_styling`, `textarea_cursor` |
| A call that is neither a create nor a set function | 5 | `animimg_play`, `arc_knob_widget`, `spinbox_with_buttons`, `textarea_buttonmatrix`, `textarea_format` |
| An LVGL build option that the test build leaves off | 5 | `image_svg`, `lottie_approve`, `lottie_from_data`, `lottie_from_file`, `obj_transform` |
| A property that takes another widget as a pointer | 3 | `keyboard_custom_map`, `scale_needles`, `textarea_password` |
| One feature of its own | 6 | `arclabel_curved_text`, `dropdown_styling`, `image_recolor`, `imagebutton_transition`, `label_translation`, `obj_draggable` |

## What it changes in `examples/README.md`

- Its table "Not ported yet" says that `roller_fade_mask`, `buttonmatrix_styling` and `textarea_cursor` can be ported now. They cannot: the first draws on a canvas layer, and the other two need their setters in another order.
- It says the same of the styling examples of the slider, the switch and the checkbox, of the roller examples and of `spinner_basic`. They can, and the next pull request ports them.
- The table is brought up to date with the ports, and the counts of `README.md` and the site with the last batch.
