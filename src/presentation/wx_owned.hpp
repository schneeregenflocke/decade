#ifndef WX_OWNED_HPP
#define WX_OWNED_HPP

#include <memory>
#include <utility>

// Creates a wx object and hands ownership to its wx parent, returning a raw
// (non-owning) pointer to it. This wraps the
// `std::make_unique<T>(...).release()` idiom the panels repeat for every child
// widget and sizer: the object is constructed through a unique_ptr (so a
// throwing constructor cannot leak) and then released to the wx parent, which
// owns the lifetime from there. See the ownership note in CLAUDE.md.
template <typename Widget, typename... Args>
Widget* MakeOwned(Args&&... args) {
  return std::make_unique<Widget>(std::forward<Args>(args)...).release();
}

#endif  // WX_OWNED_HPP
