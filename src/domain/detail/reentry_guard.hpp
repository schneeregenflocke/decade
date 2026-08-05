#ifndef REENTRY_GUARD_HPP
#define REENTRY_GUARD_HPP

namespace domain::detail {

// An RAII helper: it sets `flag` to true on construction and back to false on
// destruction. Stores break echo loops with it, where a slot on the published
// state runs back into `Receive*` of the same store.
class ScopedReentryFlag {
 public:
  explicit ScopedReentryFlag(bool& flag) : flag_(flag) { flag_ = true; }
  ~ScopedReentryFlag() { flag_ = false; }
  ScopedReentryFlag(const ScopedReentryFlag&) = delete;
  ScopedReentryFlag& operator=(const ScopedReentryFlag&) = delete;
  ScopedReentryFlag(ScopedReentryFlag&&) = delete;
  ScopedReentryFlag& operator=(ScopedReentryFlag&&) = delete;

 private:
  bool& flag_;
};

}  // namespace domain::detail

#endif  // REENTRY_GUARD_HPP
