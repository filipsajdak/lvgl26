#pragma once

// The list of watchers that a state, or a computed, keeps. A watcher is a node that its
// owner holds: the list allocates nothing and knows no type. lv::bind keeps its node on
// LVGL's heap today; the node is plain data, so a later owner can hold it in a larger
// object without any change here.
//
//   struct my_watcher : lv::detail::watch_node { ... };
//   node.changed = &on_change;
//   list.append(node);          // told after every change, in the order of append()
//   lv::detail::watch_list::remove(node);
//
// What a walk (tell) promises:
//
//   - Every watcher linked when the walk began is told once, in the order it was linked.
//   - A watcher may remove itself or any other watcher during the walk: one that was
//     removed before its turn is not told.
//   - A watcher linked during the walk is not told by it. Whoever links a watcher shows
//     the current value itself.
//   - A walk that starts while another one of the same list is in progress (a watcher that
//     changes the state) tells everyone the new value, and the walk it interrupted ends
//     when its current watcher returns: it would only repeat that one.
//   - detach_all() inside a walk ends the walk.
//
// Like all of LVGL, use a list from the UI thread only.

#include <cstddef>

namespace lv::detail {

class watch_list;

// One watcher, a node of a list. `list` is null while the node is in no list.
struct watch_node {
  watch_node* next = nullptr;
  watch_node* prev = nullptr;  // of the first node of a list: the last node (see watch_list)
  watch_list* list = nullptr;
  void (*changed)(watch_node& self) = nullptr;

  constexpr watch_node() = default;
  // A node that is moved is not in a list: the list points at the old address.
  constexpr watch_node(watch_node&& other) noexcept : changed(other.changed) {}
  watch_node(const watch_node&) = delete;
  watch_node& operator=(const watch_node&) = delete;
  watch_node& operator=(watch_node&&) = delete;

  bool linked() const { return list != nullptr; }
};

class watch_list {
  // A walk in progress. It lives on the stack of tell().
  struct walk {
    watch_node* next;   // the node to tell next, or null
    watch_node* stop;   // the last node that is to be told
    walk* outer;
    bool interrupted;   // a nested walk has told everyone
  };

  watch_node* first_ = nullptr;  // first->prev is the last node, so that append() is O(1)
  walk* walks_ = nullptr;

 public:
  constexpr watch_list() = default;
  watch_list(const watch_list&) = delete;
  watch_list& operator=(const watch_list&) = delete;

  bool empty() const { return first_ == nullptr; }

  // Adds `n` after the others. `n` must be in no list.
  void append(watch_node& n) {
    n.list = this;
    n.next = nullptr;
    if (first_ == nullptr) {
      n.prev = &n;
      first_ = &n;
    } else {
      n.prev = first_->prev;
      first_->prev->next = &n;
      first_->prev = &n;
    }
  }

  // Takes `n` out of the list it is in, if it is in one.
  static void remove(watch_node& n) {
    if (n.list != nullptr) n.list->take_out(n);
  }

  // Tells every watcher. See the top of this file.
  void tell() {
    if (first_ == nullptr) return;
    for (walk* w = walks_; w != nullptr; w = w->outer) w->interrupted = true;
    walk k{first_, first_->prev, walks_, false};
    walks_ = &k;
    while (k.next != nullptr && !k.interrupted) {
      watch_node* n = k.next;
      k.next = n == k.stop ? nullptr : n->next;
      n->changed(*n);
    }
    walks_ = k.outer;
  }

  // Takes every node out. The nodes stay where they are, with their list empty.
  void detach_all() {
    for (walk* w = walks_; w != nullptr; w = w->outer) w->next = nullptr;
    for (watch_node* n = first_; n != nullptr;) {
      watch_node* next = n->next;
      n->next = n->prev = nullptr;
      n->list = nullptr;
      n = next;
    }
    first_ = nullptr;
  }

 private:
  // Moves the walks on that were about to visit `n`, or to end with it.
  void move_walks_past(const watch_node& n) {
    for (walk* w = walks_; w != nullptr; w = w->outer) {
      if (w->next == &n)
        w->next = &n == w->stop ? nullptr : n.next;
      else if (w->stop == &n && w->next != nullptr)
        w->stop = n.prev;
    }
  }

  void take_out(watch_node& n) {
    move_walks_past(n);
    if (&n == first_) {
      first_ = n.next;
      if (first_ != nullptr) first_->prev = n.prev;
    } else {
      n.prev->next = n.next;
      (n.next != nullptr ? n.next->prev : first_->prev) = n.prev;
    }
    n.next = n.prev = nullptr;
    n.list = nullptr;
  }
};

}  // namespace lv::detail
