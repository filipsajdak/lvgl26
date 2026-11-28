/* The screen of examples/mcu/slice.cpp, written as plain LVGL C: no lvgl26 header. It is
   the baseline that the slice is measured against (docs/targets.md), built by the preset
   arm-m4 with the same LVGL and flags and run in the same QEMU board by tools/arm-run.sh.
   It is no example: examples call no C API. It makes the same widgets, binds the label to
   an integer subject, and runs an lv_timer where the slice runs a task: three times, every
   100 ms, it counts and moves the dial. It ends LVGL as the slice does. */
#include <lvgl.h>

#include <stdint.h>
#include <stdio.h>

/* The state: a subject, as LVGL's observer gives it. */
static lv_subject_t ticks;

/* Ten lines of the display at a time. They go nowhere: there is no panel in QEMU. */
static uint8_t band[LV_DRAW_BUF_SIZE(320, 10, LV_COLOR_FORMAT_NATIVE)] __attribute__((aligned(LV_DRAW_BUF_ALIGN)));

static void discard(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
  (void)area;
  (void)pixels;
  lv_display_flush_ready(display);
}

/* The task: count and move the dial. The timer ends itself after three runs. */
static void run(lv_timer_t* timer) {
  lv_obj_t* dial = lv_timer_get_user_data(timer);
  const int32_t count = lv_subject_get_int(&ticks) + 1;
  lv_subject_set_int(&ticks, count);
  lv_arc_set_value(dial, 60 * count);
}

static size_t heap_used(void) {
  lv_mem_monitor_t now;
  lv_mem_monitor(&now);
  return now.total_size - now.free_size;
}

int main(void) {
  lv_init();
  lv_display_t* display = lv_display_create(320, 240);
  lv_display_set_buffers(display, band, NULL, sizeof band, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, discard);

  const size_t before = heap_used();
  lv_subject_init_int(&ticks, 0);
  lv_obj_t* column = lv_obj_create(lv_display_get_screen_active(display));
  lv_obj_set_size(column, 320, 240);
  lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
  lv_obj_t* title = lv_label_create(column);
  lv_label_set_text(title, "Speed");
  lv_obj_t* dial = lv_arc_create(column);
  lv_arc_set_range(dial, 0, 240);
  lv_obj_set_size(dial, 110, 110);
  lv_arc_set_value(dial, 120);
  lv_obj_t* count = lv_label_create(column);
  if (lv_label_bind_text(count, &ticks, "%d ticks") == NULL) {
    printf("mount failed\n");
    return 1;
  }
  const size_t with_screen = heap_used();
  lv_timer_t* task = lv_timer_create(run, 100, dial);
  if (task == NULL) {
    printf("no memory for the task\n");
    return 1;
  }
  lv_timer_set_repeat_count(task, 3);
  const size_t with_task = heap_used();

  for (int ms_passed = 0; ms_passed < 500; ms_passed += 5) {
    lv_tick_inc(5);
    lv_timer_handler();
  }

  printf("plain LVGL C on Cortex-M4\n");
  printf("screen: %u bytes of LVGL's heap\n", (unsigned)(with_screen - before));
  printf("task:   %u bytes of LVGL's heap\n", (unsigned)(with_task - with_screen));
  printf("ticks:  %d\n", (int)lv_subject_get_int(&ticks));
  printf("dial:   %d\n", (int)lv_arc_get_value(dial));
  printf("label:  %s\n", lv_label_get_text(count));
  const int ok = lv_subject_get_int(&ticks) == 3 && lv_arc_get_value(dial) == 180;
  printf(ok ? "ok\n" : "FAILED\n");
  lv_obj_delete(column);
  lv_subject_deinit(&ticks);
  lv_display_delete(display);
  lv_deinit();
  return ok ? 0 : 1;
}
