#include "reentry_guard.hpp"

namespace domain::detail {

ScopedReentryFlag::ScopedReentryFlag(bool& flag) : flag_(flag) { flag_ = true; }

ScopedReentryFlag::~ScopedReentryFlag() { flag_ = false; }

}  // namespace domain::detail
