#ifndef REENTRY_GUARD_HPP
#define REENTRY_GUARD_HPP

namespace domain::detail {

// RAII-Helfer: setzt `flag` beim Konstruieren auf true und beim Zerstören
// zurück auf false. Stores brechen damit Echoschleifen, bei denen ein Slot am
// veröffentlichten Zustand wieder in `Receive*` desselben Stores läuft.
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
