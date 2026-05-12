#include <gtest/gtest.h>

#include "packages/page_config.hpp"

namespace {

PageSetupConfig MakeConfig(float width, float height, int orientation) {
  PageSetupConfig config;
  config.size = {width, height};
  config.margins = {1.0F, 2.0F, 3.0F, 4.0F};
  config.orientation = orientation;
  return config;
}

}  // namespace

TEST(PageSetupStoreTest, ReceiveStoresConfigAndEmitsSignal) {
  PageSetupStore store;
  int emissions = 0;
  PageSetupConfig captured{};
  store.signal_page_setup_config.connect(
      [&](const PageSetupConfig& config) {
        ++emissions;
        captured = config;
      });

  const auto config = MakeConfig(210.0F, 297.0F, 0);
  store.ReceivePageSetup(config);

  EXPECT_EQ(emissions, 1);
  EXPECT_FLOAT_EQ(captured.size[0], 210.0F);
  EXPECT_FLOAT_EQ(captured.size[1], 297.0F);
  EXPECT_EQ(captured.orientation, 0);
  EXPECT_FLOAT_EQ(store.page_setup_config.margins[2], 3.0F);
}

TEST(PageSetupStoreTest, SendPageSetupEmitsCurrentConfig) {
  PageSetupStore store;
  store.page_setup_config = MakeConfig(100.0F, 100.0F, 1);
  int emissions = 0;
  store.signal_page_setup_config.connect(
      [&](const PageSetupConfig&) { ++emissions; });

  store.SendPageSetup();
  store.SendPageSetup();
  EXPECT_EQ(emissions, 2);
}

TEST(PageSetupStoreTest, ReentryGuardBlocksRecursiveReceive) {
  PageSetupStore store;
  int emissions = 0;
  store.signal_page_setup_config.connect([&](const PageSetupConfig&) {
    ++emissions;
    if (emissions == 1) {
      store.ReceivePageSetup(MakeConfig(0.0F, 0.0F, 9));
    }
  });

  store.ReceivePageSetup(MakeConfig(210.0F, 297.0F, 0));

  EXPECT_EQ(emissions, 1);
  EXPECT_FLOAT_EQ(store.page_setup_config.size[0], 210.0F);
  EXPECT_EQ(store.page_setup_config.orientation, 0);
}
