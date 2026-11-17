#pragma once

// LVGL's heap with no room left, for tests of what the library does when LVGL has no
// memory. Changes nothing in LVGL: it takes every free block with lv_malloc.

#include <lvgl.h>

#include <cstddef>
#include <vector>

// Takes all of LVGL's heap except one block of `room` bytes, so that the next
// allocation of at most that size succeeds and the one after it fails. Gives the
// heap back when it goes.
class full_heap {
  // The first size tried is above the largest block the test heap has; each round
  // halves it, down to the smallest block LVGL's allocator hands out.
  static constexpr std::size_t largest_block = std::size_t{1} << 23;
  static constexpr std::size_t smallest_block = sizeof(void*);

  std::vector<void*> blocks_;

 public:
  explicit full_heap(std::size_t room) {
    blocks_.reserve(largest_block / smallest_block / smallest_block);
    void* kept = room > 0 ? lv_malloc(room) : nullptr;
    for (std::size_t size = largest_block; size >= smallest_block; size /= 2)
      while (void* block = lv_malloc(size)) blocks_.push_back(block);
    lv_free(kept);
  }
  full_heap(const full_heap&) = delete;
  full_heap& operator=(const full_heap&) = delete;
  ~full_heap() { release(); }

  // Gives the heap back now.
  void release() {
    for (void* block : blocks_) lv_free(block);
    blocks_.clear();
  }
};
