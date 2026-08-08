#ifndef MAKE_OWNED_HPP
#define MAKE_OWNED_HPP

#include <memory>
#include <utility>

// Creates a Qt object and hands ownership to its Qt parent, returning a raw
// (non-owning) pointer to it. This wraps the
// `std::make_unique<T>(...).release()` idiom the panels repeat for every child
// widget and layout: the object is constructed through a unique_ptr (so a
// throwing constructor cannot leak) and then released to the parent, which owns
// the lifetime from there. See AGENTS.md, "Ownership and lifetimes".
template <typename Owned, typename... Args>
Owned* MakeOwned(Args&&... args) {
  return std::make_unique<Owned>(std::forward<Args>(args)...).release();
}

#endif  // MAKE_OWNED_HPP
