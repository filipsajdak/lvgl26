#pragma once

// The samples of an input device, kept in the order they came until LVGL reads them: a ring
// of a fixed size inside the device object. lv::pointer keeps its samples here, and a keypad
// and an encoder will keep theirs.
//
//   input_queue<pointer_sample, 16, merge_pointer_moves> q;
//   q.push(s);       // false when full and `Merge` cannot fold `s` into the newest sample
//   q.pop();         // the oldest sample, or nothing
//
// `Merge::merge(newest, s)` folds `s` into the newest sample when that loses nothing that
// matters, such as one move after another; it is asked only when the queue is full.

#include <array>
#include <cstddef>
#include <optional>

namespace lv::detail {

template <class Sample, std::size_t Capacity, class Merge>
  requires(Capacity > 0)
class input_queue {
  std::array<Sample, Capacity> ring_{};
  std::size_t first_ = 0;
  std::size_t count_ = 0;

 public:
  bool empty() const noexcept { return count_ == 0; }
  std::size_t size() const noexcept { return count_; }

  // The newest sample. The queue must not be empty.
  const Sample& back() const
    pre(count_ > 0)
  {
    return ring_[(first_ + count_ - 1) % Capacity];
  }

  bool push(Sample s) noexcept {
    if (count_ < Capacity) {
      ring_[(first_ + count_) % Capacity] = s;
      ++count_;
      return true;
    }
    return Merge::merge(ring_[(first_ + count_ - 1) % Capacity], s);
  }

  std::optional<Sample> pop() noexcept {
    if (count_ == 0) return std::nullopt;
    const Sample s = ring_[first_];
    first_ = (first_ + 1) % Capacity;
    --count_;
    return s;
  }
};

}  // namespace lv::detail
