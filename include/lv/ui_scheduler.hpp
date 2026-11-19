#pragma once

// Work from other threads, run on the UI thread.
//
// LVGL is not thread-safe: only the thread that calls lv_timer_handler() (the UI
// thread) may touch a widget, a state or a task. lv::ui_scheduler is how another
// thread gets something done there:
//
//   lv::ui_scheduler ui;                    // on the UI thread, after lv_init()
//
//   // any thread:
//   ui.post([reading] { temperature = reading; });
//
//   // in a task: go on in a later turn of the LVGL loop
//   co_await ui.schedule();
//
// post() may be called from any thread, also from the UI thread. It only puts the
// work in a queue, under a mutex, and never calls LVGL. The UI thread empties the
// queue from an LVGL timer, once every `period`, in the order of posting. Work that
// is posted while the queue is emptied runs in the next period.
//
// schedule() has the shape of std::execution's: what it returns completes on the
// scheduler's thread. A task waits for it like for anything else, also in lv::first.
// A task itself always runs on the UI thread and its frame comes from LVGL's
// allocator, so another thread starts one with
//   ui.post([] { lv::spawn(work()); });
//
// Everything but post() belongs to the UI thread: creating and destroying the
// scheduler, and schedule(). Destroy it before lv_deinit(), when no other thread
// posts any more, and not from posted work. Work that is still queued then is
// destroyed without being run; a task that still waits for schedule() never goes on.
//
// This header needs a hosted C++ library (std::mutex). <lv/lv.hpp> does not
// include it.

#include <lv/detail/settings.hpp>
#include <lv/task.hpp>
#include <lv/units.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

namespace lv {

namespace detail {

// One entry of the scheduler's queue: posted work, or a task that waits.
struct queued {
  queued* next = nullptr;
  queued* previous = nullptr;
  void (*run)(queued*) noexcept = nullptr;   // on the UI thread, once it is taken out
  void (*drop)(queued*) noexcept = nullptr;  // instead of run, when the scheduler is destroyed
};

// The queue: a list of entries that any thread can add to. Every member locks.
class ui_queue {
  std::mutex mutex_;
  queued* first_ = nullptr;
  queued* last_ = nullptr;
  std::size_t size_ = 0;

 public:
  // lock() and unlock() by hand, not a std::lock_guard: with a guard, GCC 16.2's
  // analyzer loses sight of the entry and reports it as leaked. Nothing between the
  // two can throw.
  void push(queued* entry) {
    mutex_.lock();
    entry->next = nullptr;
    entry->previous = last_;
    if (last_ != nullptr)
      last_->next = entry;
    else
      first_ = entry;
    last_ = entry;
    ++size_;
    mutex_.unlock();
  }

  // Takes the oldest entry out, or returns null.
  queued* pop() {
    const std::lock_guard lock{mutex_};
    queued* entry = first_;
    if (entry != nullptr) unlink(entry);
    return entry;
  }

  // Takes out an entry that is in the queue.
  void remove(queued* entry) {
    const std::lock_guard lock{mutex_};
    unlink(entry);
  }

  std::size_t size() {
    const std::lock_guard lock{mutex_};
    return size_;
  }

 private:
  void unlink(queued* entry) {
    if (entry->previous != nullptr)
      entry->previous->next = entry->next;
    else
      first_ = entry->next;
    if (entry->next != nullptr)
      entry->next->previous = entry->previous;
    else
      last_ = entry->previous;
    --size_;
  }
};

// Posted work. It lives on the C++ heap, not LVGL's: the thread that posts must
// not call LVGL.
template <class F>
struct posted : queued {
  F work;

  explicit posted(F w) : work(std::move(w)) {
    run = &ran;
    drop = &dropped;
  }

  // noexcept: the library is built to work without exceptions; one that escapes
  // posted work ends the program.
  static void ran(queued* entry) noexcept {
    auto* self = static_cast<posted*>(entry);
    self->work();
    delete self;
  }
  static void dropped(queued* entry) noexcept { delete static_cast<posted*>(entry); }
};

// A later turn of the LVGL loop, as something to wait for: ui.schedule().
struct schedule_source : queued {
  ui_queue* queue = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  bool waiting = false;  // in the queue

  explicit schedule_source(ui_queue* q) : queue(q) {
    run = &turn_came;
    drop = &scheduler_gone;
  }
  schedule_source(schedule_source&&) = default;  // only moved before it is armed
  ~schedule_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base*) {
    done = done_fn;
    context = done_context;
    waiting = true;
    queue->push(this);
    return true;
  }

  void disarm() noexcept {
    if (waiting) queue->remove(this);
    waiting = false;
  }

  // For lv::first: what to do when the turn comes first.
  template <std::invocable F>
  handled<schedule_source, F> then(F handler) && {
    return {std::move(*this), std::move(handler)};
  }

  static void turn_came(queued* entry) noexcept {
    auto* self = static_cast<schedule_source*>(entry);
    self->waiting = false;
    self->done(self->context);  // may destroy this source: nothing after this line
  }

  // The scheduler is destroyed: this wait is never over, and must not touch the queue.
  static void scheduler_gone(queued* entry) noexcept {
    static_cast<schedule_source*>(entry)->waiting = false;
  }
};

// Work that can be posted: called once, with nothing, on the UI thread.
template <class F>
concept postable = std::invocable<F&> && std::move_constructible<F>;

}  // namespace detail

// Runs work from any thread on the UI thread. See the top of this file.
class ui_scheduler {
 public:
  // On the UI thread. The queue is emptied once every `period`.
  // O only delays reading the project's settings until a scheduler is made.
  template <class O = override>
  explicit ui_scheduler(duration period = duration{LV_DEF_REFR_PERIOD})
      : timer_(lv_timer_create(&turn, period.ms, this)) {
    detail::require<^^allow_settings::threads, O, "lv::ui_scheduler">();
  }

  ui_scheduler(const ui_scheduler&) = delete;
  ui_scheduler& operator=(const ui_scheduler&) = delete;

  // On the UI thread. What is still queued is destroyed without being run.
  ~ui_scheduler() {
    if (timer_ != nullptr) lv_timer_delete(timer_);
    while (detail::queued* entry = queue_.pop()) entry->drop(entry);
  }

  // False when LVGL had no memory for the timer: nothing that is posted would run.
  explicit operator bool() const noexcept { return timer_ != nullptr; }

  // From any thread: runs `work` on the UI thread, in a later turn of the LVGL loop.
  // Returns false if there is no memory to queue it; `work` is then destroyed.
  template <detail::postable F>
  bool post(F work) {
    detail::require<^^allow_settings::allocation, typename detail::settings<F>::tag, "lv::ui_scheduler::post",
                    " It keeps each piece of posted work on the heap until the UI thread runs it.">();
    // Owned here until it is in the queue: locking the mutex can throw.
    std::unique_ptr<detail::posted<F>> entry{new (std::nothrow) detail::posted<F>{std::move(work)}};
    if (entry == nullptr) return false;
    queue_.push(entry.get());
    static_cast<void>(entry.release());
    return true;
  }

  // On the UI thread, in a task: co_await ui.schedule() goes on in a later turn of
  // the LVGL loop, after the work that was posted before it.
  detail::schedule_source schedule() noexcept { return detail::schedule_source{&queue_}; }

 private:
  detail::ui_queue queue_;
  lv_timer_t* timer_;

  // Runs what is queued now. Entries that are added meanwhile wait for the next
  // turn, so that work which posts more work cannot keep the UI thread here.
  void run_pending() {
    for (std::size_t left = queue_.size(); left > 0; --left) {
      detail::queued* entry = queue_.pop();
      if (entry == nullptr) return;  // a task that waited was cancelled meanwhile
      entry->run(entry);
    }
  }

  static void turn(lv_timer_t* t) {
    static_cast<ui_scheduler*>(lv_timer_get_user_data(t))->run_pending();
  }
};

}  // namespace lv
