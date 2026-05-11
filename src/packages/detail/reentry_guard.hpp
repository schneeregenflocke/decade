#ifndef HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_DETAIL_REENTRY_GUARD_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_DETAIL_REENTRY_GUARD_HPP

namespace packages::detail {

// RAII helper: sets `flag` to true on construction, resets to false on
// destruction. Used by stores to break echo loops where a slot connected to a
// store's outgoing signal feeds back into the store's `Receive*` (e.g. via
// the EventBus).
class ScopedReentryFlag {
 public:
  explicit ScopedReentryFlag(bool& flag) : flag_(&flag) { *flag_ = true; }
  ~ScopedReentryFlag() { *flag_ = false; }
  ScopedReentryFlag(const ScopedReentryFlag&) = delete;
  ScopedReentryFlag& operator=(const ScopedReentryFlag&) = delete;
  ScopedReentryFlag(ScopedReentryFlag&&) = delete;
  ScopedReentryFlag& operator=(ScopedReentryFlag&&) = delete;

 private:
  bool* flag_;
};

}  // namespace packages::detail

#endif  // HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_DETAIL_REENTRY_GUARD_HPP
