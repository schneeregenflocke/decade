#include <gtest/gtest.h>

#include "domain/detail/reentry_guard.hpp"

using domain::detail::ScopedReentryFlag;

TEST(ReentryGuardTest, SetsFlagWhileAliveResetsOnDestruction) {
  bool flag = false;
  {
    const ScopedReentryFlag guard(flag);
    EXPECT_TRUE(flag);
  }
  EXPECT_FALSE(flag);
}

TEST(ReentryGuardTest, NestedScopesResetOuterFlagToFalse) {
  bool flag = false;
  {
    const ScopedReentryFlag outer(flag);
    EXPECT_TRUE(flag);
    {
      const ScopedReentryFlag inner(flag);
      EXPECT_TRUE(flag);
    }
    // Inner destructor resets the flag — that is the documented behaviour.
    // Stores rely on this only for top-level guarding; the inner scope here
    // exists to document the contract.
    EXPECT_FALSE(flag);
  }
  EXPECT_FALSE(flag);
}
